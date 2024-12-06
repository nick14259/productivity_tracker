package main.controller;

import main.model.LEDState;
import java.net.URI;
import java.net.http.*;
import java.time.Duration;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;
import java.util.concurrent.CompletableFuture;

public class ESP32Controller {
    private final HttpClient httpClient;
    private String ipAddress;

    public ESP32Controller() {
        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(5))
                .build();
    }

    public void setIpAddress(String ipAddress) {
        this.ipAddress = ipAddress;
    }

    public CompletableFuture<Boolean> testConnection() {
        if (ipAddress == null || ipAddress.isEmpty()) {
            return CompletableFuture.completedFuture(false);
        }

        return httpClient.sendAsync(
                        HttpRequest.newBuilder()
                                .uri(URI.create("http://" + ipAddress + "/status"))
                                .GET()
                                .build(),
                        HttpResponse.BodyHandlers.ofString())
                .thenApply(response -> response.statusCode() == 200);
    }

    public CompletableFuture<Boolean> updateLED(LEDState ledState) {
        JSONObject json = new JSONObject();
        json.put("index", ledState.getIndex());
        json.put("red", ledState.getColor().getRed());
        json.put("green", ledState.getColor().getGreen());
        json.put("blue", ledState.getColor().getBlue());
        json.put("brightness", 255);

        return sendRequest("/led", json);
    }

    public CompletableFuture<Boolean> clearAllLEDs() {
        return sendRequest("/clear", new JSONObject());
    }

    private CompletableFuture<Boolean> sendRequest(String endpoint, JSONObject json) {
        if (ipAddress == null || ipAddress.isEmpty()) {
            return CompletableFuture.completedFuture(false);
        }

        return httpClient.sendAsync(
                        HttpRequest.newBuilder()
                                .uri(URI.create("http://" + ipAddress + endpoint))
                                .header("Content-Type", "application/json")
                                .POST(HttpRequest.BodyPublishers.ofString(json.toString()))
                                .build(),
                        HttpResponse.BodyHandlers.ofString())
                .thenApply(response -> response.statusCode() == 200)
                .exceptionally(e -> {
                    System.err.println("Error sending request: " + e.getMessage());
                    return false;
                });
    }

    public CompletableFuture<Boolean> updateAllLEDs(List<LEDState> states) {
        JSONObject json = new JSONObject();
        JSONArray ledsArray = new JSONArray();

        for (LEDState state : states) {
            JSONObject ledJson = new JSONObject();
            ledJson.put("index", state.getIndex());
            ledJson.put("red", state.getColor().getRed());
            ledJson.put("green", state.getColor().getGreen());
            ledJson.put("blue", state.getColor().getBlue());
            ledsArray.put(ledJson);
        }

        json.put("leds", ledsArray);
        return sendRequest("/all", json);
    }

    public CompletableFuture<Boolean> sendBatchUpdate(List<LEDState> states) {
        JSONObject json = new JSONObject();
        JSONArray ledsArray = new JSONArray();

        for (LEDState state : states) {
            JSONObject ledJson = new JSONObject();
            ledJson.put("index", state.getIndex());
            ledJson.put("red", state.getColor().getRed());
            ledJson.put("green", state.getColor().getGreen());
            ledJson.put("blue", state.getColor().getBlue());
            ledJson.put("brightness", 255);  // Using max brightness
            ledsArray.put(ledJson);
        }

        json.put("leds", ledsArray);
        return sendRequest("/batch", json);
    }
}