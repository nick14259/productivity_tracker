#ifndef GITHUB_API_H
#define GITHUB_API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <vector>
#include <map>
#include <algorithm>
#include "Config.h"

class GitHubAPI {
public:
    struct ContributionData {
        String date;
        int count;
    };

    struct ApiResponse {
        bool success;
        String error;
        std::vector<ContributionData> contributions;
        int remainingCalls;
    };

    static GitHubAPI& getInstance() {
        static GitHubAPI instance;
        return instance;
    }

    bool begin();
    ApiResponse fetchContributions();
    bool isConfigured() const;
    String getLastError() const { return lastError; }
    bool hasError() const { return !lastError.isEmpty(); }
    void clearError() { lastError.clear(); }
    int getRemainingCalls() const { return remainingCalls; }
    unsigned long getLastUpdateTime() const { return lastUpdateTime; }

private:
    GitHubAPI() : remainingCalls(0), lastUpdateTime(0) {}
    GitHubAPI(const GitHubAPI&) = delete;
    GitHubAPI& operator=(const GitHubAPI&) = delete;
    
    String lastError;
    int remainingCalls;
    unsigned long lastUpdateTime;
    
    void setError(const String& error);
    bool validateResponse(const String& response);
    void updateRateLimits(HTTPClient& http);
    bool parseGraphQLResponse(const String& response, std::vector<ContributionData>& contributions);
    String buildGraphQLQuery() const;
};

#endif // GITHUB_API_H