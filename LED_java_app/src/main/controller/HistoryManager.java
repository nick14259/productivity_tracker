package main.controller;

import main.model.LEDState;
import main.model.Task;
import java.awt.*;
import java.time.LocalDate;
import java.util.*;
import java.util.List;
import java.util.concurrent.TimeUnit;
import main.util.ColorUtils;
import org.json.JSONObject;
import org.json.JSONArray;

public class HistoryManager {
    private static final int TOTAL_LEDS = 84;
    private final Map<Integer, LEDState> ledStates;
    private LocalDate currentDate;
    private final ESP32Controller esp32Controller;

    public HistoryManager(ESP32Controller esp32Controller) {
        this.ledStates = new HashMap<>();
        this.currentDate = LocalDate.now();
        this.esp32Controller = esp32Controller;
    }

    public void shiftHistory(List<Task> tasks) {
        Color currentColor = ColorUtils.getColorForTaskCount(
                (int) tasks.stream().filter(Task::isCompleted).count()
        );

        // Multiple clear attempts for LED 84
        for (int i = 0; i < 3; i++) {
            esp32Controller.updateLED(new LEDState(TOTAL_LEDS - 1, Color.BLACK, currentDate));
            try {
                TimeUnit.MILLISECONDS.sleep(20);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }

        Map<Integer, LEDState> oldStates = new HashMap<>(ledStates);
        ledStates.clear();

        List<LEDState> updates = new ArrayList<>();

        // Shift all existing states down
        for (int i = 0; i < TOTAL_LEDS - 1; i++) {
            if (oldStates.containsKey(i + 1)) {
                LEDState oldState = oldStates.get(i + 1);
                LEDState newState = new LEDState(i, oldState.getColor(), oldState.getDate());
                ledStates.put(i, newState);
                updates.add(newState);
            }
        }

        // Send batch update
        esp32Controller.sendBatchUpdate(updates);

        try {
            TimeUnit.MILLISECONDS.sleep(50);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        // Set LED 84 to new color with multiple attempts
        for (int i = 0; i < 2; i++) {
            LEDState newState = new LEDState(TOTAL_LEDS - 1, currentColor, currentDate);
            ledStates.put(TOTAL_LEDS - 1, newState);
            esp32Controller.updateLED(newState);
            try {
                TimeUnit.MILLISECONDS.sleep(20);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }

        currentDate = currentDate.plusDays(1);
    }

    public void updateTaskProgress(List<Task> tasks) {
        Color currentColor = ColorUtils.getColorForTaskCount(
                (int) tasks.stream().filter(Task::isCompleted).count()
        );
        LEDState state = new LEDState(TOTAL_LEDS - 1, currentColor, currentDate);
        ledStates.put(TOTAL_LEDS - 1, state);
        esp32Controller.updateLED(state);
    }





    public void updateAllLEDs() {
        List<LEDState> states = new ArrayList<>();
        for (int i = 0; i < TOTAL_LEDS; i++) {
            if (ledStates.containsKey(i)) {
                states.add(ledStates.get(i));
            } else {
                states.add(new LEDState(i, Color.BLACK, currentDate));
            }
        }
        esp32Controller.updateAllLEDs(states);
    }
    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        json.put("currentDate", currentDate.toString());

        JSONArray ledsArray = new JSONArray();
        for (LEDState state : ledStates.values()) {
            ledsArray.put(state.toJson());
        }
        json.put("ledStates", ledsArray);

        return json;
    }

    public void fromJson(JSONObject json) {
        ledStates.clear();
        currentDate = LocalDate.parse(json.getString("currentDate"));

        JSONArray ledsArray = json.getJSONArray("ledStates");
        for (int i = 0; i < ledsArray.length(); i++) {
            LEDState state = LEDState.fromJson(ledsArray.getJSONObject(i));
            ledStates.put(state.getIndex(), state);
        }

        updateAllLEDs();
    }

    public List<HistoryEntry> getHistoryEntries() {
        List<HistoryEntry> entries = new ArrayList<>();
        for (int i = 0; i < TOTAL_LEDS; i++) {
            if (ledStates.containsKey(i)) {
                LEDState state = ledStates.get(i);
                entries.add(new HistoryEntry(
                        state.getDate(),
                        ColorUtils.getColorDescription(state.getColor()),
                        state.getColor()
                ));
            }
        }
        return entries;
    }

    public static class HistoryEntry {
        public final LocalDate date;
        public final String description;
        public final Color color;

        public HistoryEntry(LocalDate date, String description, Color color) {
            this.date = date;
            this.description = description;
            this.color = color;
        }
    }
}