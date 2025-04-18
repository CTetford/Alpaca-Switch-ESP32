#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <esp_err.h>
#include <string>

class OtaUpdater {
private:
    static const size_t BUFFER_SIZE = 1024;
    static std::string _firmware_url;
    static bool _updateInProgress;
    static int _updateProgress;
    static std::string _lastStatusMessage;
    static void updateTask(void* pvParameter);

public:
    // Start OTA update from a URL
    static esp_err_t startUpdate(const std::string& url);
    
    // Check if an update is in progress
    static bool isUpdateInProgress();
    
    // Get update progress (0-100)
    static int getUpdateProgress();
    
    // Get last update status message
    static std::string getLastStatusMessage();
    
    // Get firmware version
    static std::string getFirmwareVersion();
};

#endif // OTA_UPDATER_H