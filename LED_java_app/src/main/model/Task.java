package main.model;

import java.time.LocalDateTime;

public class Task {
    private String description;
    private boolean completed;
    private LocalDateTime completedAt;
    private int id;

    public Task(int id, String description) {
        this.id = id;
        this.description = description;
        this.completed = false;
    }

    public void setCompleted(boolean completed) {
        this.completed = completed;
        if (completed) {
            this.completedAt = LocalDateTime.now();
        } else {
            this.completedAt = null;
        }
    }

    // Getters and Setters
    public String getDescription() { return description; }
    public void setDescription(String description) { this.description = description; }
    public boolean isCompleted() { return completed; }
    public LocalDateTime getCompletedAt() { return completedAt; }
    public int getId() { return id; }

    // For JSON serialization
    public org.json.JSONObject toJson() {
        org.json.JSONObject json = new org.json.JSONObject();
        json.put("id", id);
        json.put("description", description);
        json.put("completed", completed);
        if (completedAt != null) {
            json.put("completedAt", completedAt.toString());
        }
        return json;
    }

    // For JSON deserialization
    public static Task fromJson(org.json.JSONObject json) {
        Task task = new Task(
                json.getInt("id"),
                json.getString("description")
        );
        task.setCompleted(json.getBoolean("completed"));
        if (json.has("completedAt") && !json.isNull("completedAt")) {
            task.completedAt = LocalDateTime.parse(json.getString("completedAt"));
        }
        return task;
    }
}