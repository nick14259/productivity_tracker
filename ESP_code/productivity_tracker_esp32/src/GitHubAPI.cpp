#include "GitHubAPI.h"

bool GitHubAPI::begin() {
    lastError.clear();
    return true;
}

bool GitHubAPI::isConfigured() const {
    const auto& config = Config::getInstance().getGitHubConfig();
    return !config.username.isEmpty() && !config.token.isEmpty();
}

void GitHubAPI::setError(const String& error) {
    lastError = error;
    Serial.println("[GitHubAPI] Error: " + error);
}

void GitHubAPI::updateRateLimits(HTTPClient& http) {
    String remaining = http.header("X-RateLimit-Remaining");
    if (!remaining.isEmpty()) {
        remainingCalls = remaining.toInt();
        Serial.printf("[GitHubAPI] Remaining API calls: %d\n", remainingCalls);
    }
}

String GitHubAPI::buildGraphQLQuery() const {
    const auto& config = Config::getInstance().getGitHubConfig();
    
    // Calculate date for 12 weeks ago
    time_t now = time(nullptr);
    time_t startDate = now - (12 * 7 * 24 * 60 * 60);
    
    // Format date in ISO 8601 format with time (required by GitHub GraphQL API)
    char fromDate[25];
    struct tm* timeinfo = gmtime(&startDate);
    strftime(fromDate, sizeof(fromDate), "%Y-%m-%dT00:00:00Z", timeinfo);
    
    // Build the GraphQL query with proper date format
    String query = "{\"query\": \"query {\\n" 
                  "  user(login: \\\"" + config.username + "\\\") {\\n"
                  "    contributionsCollection(from: \\\"" + String(fromDate) + "\\\") {\\n"
                  "      contributionCalendar {\\n"
                  "        totalContributions\\n"
                  "        weeks {\\n"
                  "          contributionDays {\\n"
                  "            contributionCount\\n"
                  "            date\\n"
                  "          }\\n"
                  "        }\\n"
                  "      }\\n"
                  "    }\\n"
                  "  }\\n"
                  "}\"}";
    
    Serial.println("[GitHubAPI] GraphQL query: " + query);
    return query;
}

bool GitHubAPI::parseGraphQLResponse(const String& response, std::vector<ContributionData>& contributions) {
    DynamicJsonDocument doc(32768);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        setError("JSON parse error: " + String(error.c_str()));
        return false;
    }
    
    try {
        contributions.clear();
        contributions.reserve(84);  // 12 weeks * 7 days

        Serial.println("[GitHubAPI] Raw response: " + response);

        JsonObject data = doc["data"];
        if (!data.isNull()) {
            JsonObject user = data["user"];
            if (!user.isNull()) {
                JsonObject contributionsCollection = user["contributionsCollection"];
                if (!contributionsCollection.isNull()) {
                    JsonObject calendar = contributionsCollection["contributionCalendar"];
                    if (!calendar.isNull()) {
                        JsonArray weeks = calendar["weeks"];
                        if (!weeks.isNull()) {
                            Serial.println("\n[GitHubAPI] Raw Contribution Data:");
                            Serial.println("Date\t\tCount");
                            Serial.println("------------------------");

                            for (JsonObject week : weeks) {
                                JsonArray days = week["contributionDays"];
                                for (JsonObject day : days) {
                                    ContributionData data;
                                    data.date = day["date"].as<String>();
                                    data.count = day["contributionCount"].as<int>();
                                    if (data.count > 0) {
                                        contributions.push_back(data);
                                        Serial.printf("%s\t%d\n", data.date.c_str(), data.count);
                                    }
                                }
                            }
                        } else {
                            Serial.println("[GitHubAPI] No weeks data found in response");
                        }
                    } else {
                        Serial.println("[GitHubAPI] No calendar data found in response");
                    }
                } else {
                    Serial.println("[GitHubAPI] No contributions collection found in response");
                }
            } else {
                Serial.println("[GitHubAPI] No user data found in response");
            }
        } else {
            Serial.println("[GitHubAPI] No data found in response");
        }

        // Sort by date (newest first)
        std::sort(contributions.begin(), contributions.end(),
            [](const ContributionData& a, const ContributionData& b) {
                return a.date > b.date;
            });
            
        Serial.println("\n[GitHubAPI] Processed Contributions (newest first):");
        Serial.println("Date\t\tCount");
        Serial.println("------------------------");
        for (const auto& contrib : contributions) {
            Serial.printf("%s\t%d\n", contrib.date.c_str(), contrib.count);
        }
        
        return true;
    }
    catch (const std::exception& e) {
        setError("Data processing error: " + String(e.what()));
        return false;
    }
}

GitHubAPI::ApiResponse GitHubAPI::fetchContributions() {
    ApiResponse response = {false, "", {}, remainingCalls};
    
    if (!isConfigured()) {
        response.error = "GitHub username or token not configured";
        return response;
    }
    
    Serial.printf("[GitHubAPI] Free heap before API call: %u bytes\n", ESP.getFreeHeap());
    
    const auto& config = Config::getInstance().getGitHubConfig();
    
    WiFiClientSecure *client = new WiFiClientSecure;
    if(!client) {
        response.error = "Failed to create SSL client";
        Serial.println("[GitHubAPI] Error: Failed to create SSL client");
        return response;
    }

    client->setInsecure();
    HTTPClient http;
    
    if (!http.begin(*client, "https://api.github.com/graphql")) {
        response.error = "Failed to connect to GitHub API";
        delete client;
        return response;
    }

    http.addHeader("User-Agent", "ESP32-GitHub-LED-Tracker");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + config.token);
    
    String query = buildGraphQLQuery();
    Serial.println("[GitHubAPI] Sending GraphQL request...");
    int httpCode = http.POST(query);
    
    if (httpCode > 0) {
        String payload = http.getString();
        Serial.printf("[GitHubAPI] Response length: %d bytes\n", payload.length());
        
        if (httpCode == HTTP_CODE_OK) {
            if (parseGraphQLResponse(payload, response.contributions)) {
                response.success = true;
                updateRateLimits(http);
                lastUpdateTime = millis();
            } else {
                response.error = lastError.isEmpty() ? "Failed to parse GraphQL data" : lastError;
            }
        } else {
            response.error = "HTTP Error " + String(httpCode) + ": " + payload;
        }
    } else {
        response.error = "Connection failed: " + http.errorToString(httpCode);
    }
    
    http.end();
    delete client;
    
    Serial.printf("[GitHubAPI] Free heap after API call: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("[GitHubAPI] Final status - Success: %d, Error: %s\n", 
                 response.success, response.error.c_str());
    
    return response;
}