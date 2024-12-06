package main.model;

import org.json.JSONObject;

import java.nio.file.Path;
import java.time.Duration;

public class SaveConfig {
    public enum SaveFrequency {
        IMMEDIATE(Duration.ZERO, "Immediate"),
        FIVE_MINUTES(Duration.ofMinutes(5), "Every 5 minutes"),
        FIFTEEN_MINUTES(Duration.ofMinutes(15), "Every 15 minutes"),
        THIRTY_MINUTES(Duration.ofMinutes(30), "Every 30 minutes"),
        MANUAL_ONLY(Duration.ofDays(365), "Manual only");

        private final Duration duration;
        private final String display;

        SaveFrequency(Duration duration, String display) {
            this.duration = duration;
            this.display = display;
        }

        public Duration getDuration() { return duration; }
        public String toString() { return display; }
    }

    private Path saveDirPath;
    private SaveFrequency frequency;
    private int maxAutoSaves;
    private boolean autoSaveOnClose;

    public SaveConfig() {
        this.frequency = SaveFrequency.FIFTEEN_MINUTES;
        this.maxAutoSaves = 3;
        this.autoSaveOnClose = true;
    }

    // Getters and Setters
    public Path getSaveDirPath() { return saveDirPath; }
    public void setSaveDirPath(Path saveDirPath) { this.saveDirPath = saveDirPath; }
    public SaveFrequency getFrequency() { return frequency; }
    public void setFrequency(SaveFrequency frequency) { this.frequency = frequency; }
    public int getMaxAutoSaves() { return maxAutoSaves; }
    public void setMaxAutoSaves(int maxAutoSaves) { this.maxAutoSaves = maxAutoSaves; }
    public boolean isAutoSaveOnClose() { return autoSaveOnClose; }
    public void setAutoSaveOnClose(boolean autoSaveOnClose) { this.autoSaveOnClose = autoSaveOnClose; }

    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        if (saveDirPath != null) {
            json.put("saveDirPath", saveDirPath.toString());
        }
        json.put("frequency", frequency.name());
        json.put("maxAutoSaves", maxAutoSaves);
        json.put("autoSaveOnClose", autoSaveOnClose);
        return json;
    }

    public static SaveConfig fromJson(JSONObject json) {
        SaveConfig config = new SaveConfig();
        if (json.has("saveDirPath")) {
            config.setSaveDirPath(Path.of(json.getString("saveDirPath")));
        }
        config.setFrequency(SaveFrequency.valueOf(json.getString("frequency")));
        config.setMaxAutoSaves(json.getInt("maxAutoSaves"));
        config.setAutoSaveOnClose(json.getBoolean("autoSaveOnClose"));
        return config;
    }
}