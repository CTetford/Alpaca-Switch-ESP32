#include "alpaca_auth.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <mbedtls/base64.h>
#include <string.h>

static const char* TAG = "alpaca_auth";

bool AlpacaAuth::_enabled = false;
std::string AlpacaAuth::_username = "admin";
std::string AlpacaAuth::_password = "admin";
const char* AlpacaAuth::NVS_NAMESPACE = "alpaca_auth";

esp_err_t AlpacaAuth::init() {
    return loadSettings();
}

bool AlpacaAuth::isEnabled() {
    return _enabled;
}

esp_err_t AlpacaAuth::setEnabled(bool enabled) {
    _enabled = enabled;
    return saveSettings();
}

esp_err_t AlpacaAuth::setCredentials(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        ESP_LOGW(TAG, "Username and password cannot be empty");
        return ESP_ERR_INVALID_ARG;
    }
    
    _username = username;
    _password = password;
    return saveSettings();
}

std::string AlpacaAuth::getUsername() {
    return _username;
}

esp_err_t AlpacaAuth::loadSettings() {
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "Authentication settings not found, using defaults");
            return ESP_OK;
        }
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read authentication status
    uint8_t enabled = 0;
    err = nvs_get_u8(handle, "enabled", &enabled);
    if (err == ESP_OK) {
        _enabled = (enabled != 0);
    }
    
    // Read username
    size_t required_size = 0;
    err = nvs_get_str(handle, "username", nullptr, &required_size);
    if (err == ESP_OK && required_size > 0) {
        char* buffer = new char[required_size];
        err = nvs_get_str(handle, "username", buffer, &required_size);
        if (err == ESP_OK) {
            _username = std::string(buffer);
        }
        delete[] buffer;
    }
    
    // Read password
    required_size = 0;
    err = nvs_get_str(handle, "password", nullptr, &required_size);
    if (err == ESP_OK && required_size > 0) {
        char* buffer = new char[required_size];
        err = nvs_get_str(handle, "password", buffer, &required_size);
        if (err == ESP_OK) {
            _password = std::string(buffer);
        }
        delete[] buffer;
    }
    
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Loaded authentication settings, auth enabled: %d", _enabled);
    return ESP_OK;
}

esp_err_t AlpacaAuth::saveSettings() {
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    // Save authentication status
    err = nvs_set_u8(handle, "enabled", _enabled ? 1 : 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write enabled status: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    // Save username
    err = nvs_set_str(handle, "username", _username.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write username: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    // Save password
    err = nvs_set_str(handle, "password", _password.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write password: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit settings: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Saved authentication settings");
    return ESP_OK;
}

bool AlpacaAuth::verifyRequest(httpd_req_t* req) {
    if (!_enabled) {
        return true;  // Authentication disabled
    }
    
    // Get Authorization header
    size_t auth_header_len = httpd_req_get_hdr_value_len(req, "Authorization");
    if (auth_header_len == 0) {
        ESP_LOGW(TAG, "No Authorization header in request");
        return false;
    }
    
    char* auth_header = new char[auth_header_len + 1];
    if (auth_header == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for auth header");
        return false;
    }
    
    esp_err_t err = httpd_req_get_hdr_value_str(req, "Authorization", auth_header, auth_header_len + 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get Authorization header: %s", esp_err_to_name(err));
        delete[] auth_header;
        return false;
    }
    
    // Check if it's a Basic auth header
    if (strncmp(auth_header, "Basic ", 6) != 0) {
        ESP_LOGW(TAG, "Not a Basic auth header");
        delete[] auth_header;
        return false;
    }
    
    // Decode base64
    size_t out_len = 0;
    unsigned char* decoded = new unsigned char[auth_header_len];
    if (decoded == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for decoded auth");
        delete[] auth_header;
        return false;
    }
    
    mbedtls_base64_decode(decoded, auth_header_len, &out_len, 
                          (const unsigned char*)auth_header + 6, auth_header_len - 6);
    delete[] auth_header;
    
    if (out_len == 0) {
        ESP_LOGW(TAG, "Failed to decode base64 auth");
        delete[] decoded;
        return false;
    }
    
    // Add null terminator
    decoded[out_len] = '\0';
    
    // Split username:password
    char* colon = strchr((char*)decoded, ':');
    if (colon == NULL) {
        ESP_LOGW(TAG, "Invalid auth format, no colon separator");
        delete[] decoded;
        return false;
    }
    
    *colon = '\0';
    const char* auth_username = (const char*)decoded;
    const char* auth_password = colon + 1;
    
    // Verify credentials
    bool result = (_username == auth_username && _password == auth_password);
    
    if (!result) {
        ESP_LOGW(TAG, "Authentication failed for user: %s", auth_username);
    }
    
    delete[] decoded;
    return result;
}

void AlpacaAuth::addAuthHeaders(httpd_req_t* req) {
    if (!_enabled) {
        return;  // Authentication disabled
    }
    
    // Add WWW-Authenticate header
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"ASCOM Alpaca\"");
}