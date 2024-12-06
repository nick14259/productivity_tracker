package main.controller;

import main.model.*;
import main.util.DateUtils;

import java.io.IOException;
import java.nio.file.*;
import java.time.Duration;
import java.time.LocalDateTime;
import java.util.*;
import java.util.concurrent.*;
import java.util.stream.Collectors;
import org.json.JSONObject;
import org.json.JSONArray;

public class SaveManager {
    private final SaveConfig config;
    private final ScheduledExecutorService autoSaveExecutor;
    private ScheduledFuture<?> currentAutoSaveTask;
    private final Queue<Path> autoSaveFiles;

    public SaveManager(SaveConfig config) {
        this.config = config;
        this.autoSaveExecutor = Executors.newSingleThreadScheduledExecutor();
        this.autoSaveFiles = new LinkedList<>();
    }

    public Path createSavePath(String prefix) throws IOException {
        if (config.getSaveDirPath() == null) {
            // Use default directory if none is set (user's home directory)
            String userHome = System.getProperty("user.home");
            Path defaultSavePath = Paths.get(userHome, "LEDTaskTracker");
            config.setSaveDirPath(defaultSavePath);

            // Create directory if it doesn't exist
            Files.createDirectories(defaultSavePath);
        }

        // Ensure save directory exists
        Files.createDirectories(config.getSaveDirPath());

        return config.getSaveDirPath().resolve(
                prefix + "_" + DateUtils.formatForFilename(LocalDateTime.now()) + ".json"
        );
    }

    public void scheduleAutoSave(Runnable saveTask) {
        if (currentAutoSaveTask != null) {
            currentAutoSaveTask.cancel(false);
        }

        if (config.getFrequency() == SaveConfig.SaveFrequency.MANUAL_ONLY) {
            return;
        }

        if (config.getFrequency() == SaveConfig.SaveFrequency.IMMEDIATE) {
            saveTask.run();
            return;
        }

        Duration interval = config.getFrequency().getDuration();
        currentAutoSaveTask = autoSaveExecutor.scheduleAtFixedRate(
                saveTask,
                interval.toMinutes(),
                interval.toMinutes(),
                TimeUnit.MINUTES
        );
    }

    public void manageAutoSaves() throws IOException {
        if (config.getSaveDirPath() == null) return;

        // Get all auto-save files
        List<Path> files = Files.list(config.getSaveDirPath())
                .filter(p -> p.getFileName().toString().startsWith("led_progress_auto_"))
                .sorted()
                .collect(Collectors.toList());

        // Keep only the most recent files
        while (files.size() >= config.getMaxAutoSaves()) {
            Files.delete(files.remove(0));
        }
    }

    public List<Path> listSaveFiles() throws IOException {
        if (config.getSaveDirPath() == null) {
            return Collections.emptyList();
        }

        Path saveDir = config.getSaveDirPath();
        if (!Files.exists(saveDir)) {
            Files.createDirectories(saveDir);
            return Collections.emptyList();
        }

        return Files.list(saveDir)
                .filter(p -> p.getFileName().toString().contains("led_progress"))
                .sorted()
                .collect(Collectors.toList());
    }

    public void shutdown() {
        autoSaveExecutor.shutdown();
        try {
            if (!autoSaveExecutor.awaitTermination(800, TimeUnit.MILLISECONDS)) {
                autoSaveExecutor.shutdownNow();
            }
        } catch (InterruptedException e) {
            autoSaveExecutor.shutdownNow();
            Thread.currentThread().interrupt();
        }
    }
}