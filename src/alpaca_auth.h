#ifndef ALPACA_AUTH_H
#define ALPACA_AUTH_H

#include <esp_err.h>
#include <string>
#include <esp_http_server.h>

class AlpacaAuth {
public:
    // Initialize authentication with default settings
    static esp_err_t init();
    
    // Check if authentication is enabled
    static bool isEnabled();
    
    // Enable/disable authentication
    static esp_err_t setEnabled(bool enabled);
    
    // Set username and password
    static esp_err_t setCredentials(const std::string& username, const std::string& password);
    
    // Get username
    static std::string getUsername();
    
    // Verify request authentication
    static bool verifyRequest(httpd_req_t* req);
    
    // Add authentication headers to a response
    static void addAuthHeaders(httpd_req_t* req);

private:
    static bool _enabled;
    static std::string _username;
    static std::string _password;
    static const char* NVS_NAMESPACE;
    
    // Load authentication settings from NVS
    static esp_err_t loadSettings();
    
    // Save authentication settings to NVS
    static esp_err_t saveSettings();
};

#endif // ALPACA_AUTH_H