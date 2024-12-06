package main.ui;

import main.controller.HistoryManager;
import main.model.Task;

import javax.swing.*;
import java.awt.*;
import java.util.*;
import java.util.List;

public class TaskPanel extends JPanel {
    private final List<Task> tasks;
    private final List<JCheckBox> checkboxes;
    private final List<JTextField> textFields;
    private final HistoryManager historyManager;
    private Runnable saveCallback;
    private JButton newDayButton;

    public TaskPanel(HistoryManager historyManager) {
        this.historyManager = historyManager;
        this.tasks = new ArrayList<>();
        this.checkboxes = new ArrayList<>();
        this.textFields = new ArrayList<>();

        setLayout(new BorderLayout());
        initializeComponents();
    }

    private void initializeComponents() {
        JPanel taskListPanel = new JPanel();
        taskListPanel.setLayout(new GridLayout(5, 2, 5, 5));

        // Create 5 tasks with checkboxes and text fields
        for (int i = 0; i < 5; i++) {
            Task task = new Task(i, "Task " + (i + 1));
            tasks.add(task);

            JCheckBox checkbox = new JCheckBox();
            checkboxes.add(checkbox);

            JTextField textField = new JTextField(20);
            textField.setText(task.getDescription());
            textFields.add(textField);

            checkbox.addActionListener(e -> onTaskUpdated());

            taskListPanel.add(checkbox);
            taskListPanel.add(textField);
        }

        // Add New Day button
        newDayButton = new JButton("New Day");
        newDayButton.addActionListener(e -> onNewDay());

        // Layout
        add(taskListPanel, BorderLayout.CENTER);
        add(newDayButton, BorderLayout.SOUTH);
    }

    private void onTaskUpdated() {
        // Get current task states
        for (int i = 0; i < tasks.size(); i++) {
            tasks.get(i).setCompleted(checkboxes.get(i).isSelected());
            tasks.get(i).setDescription(textFields.get(i).getText());
        }

        // Update LED based on completed tasks
        historyManager.updateTaskProgress(tasks);

        // Trigger save if needed
        if (saveCallback != null) {
            saveCallback.run();
        }
    }

    private void onNewDay() {
        int option = JOptionPane.showConfirmDialog(
                this,
                "Start a new day? This will shift the LED history.",
                "New Day",
                JOptionPane.YES_NO_OPTION
        );

        if (option == JOptionPane.YES_OPTION) {
            historyManager.shiftHistory(tasks);
            // Reset checkboxes
            checkboxes.forEach(cb -> cb.setSelected(false));
            // Update tasks
            onTaskUpdated();
        }
    }

    public void setSaveCallback(Runnable callback) {
        this.saveCallback = callback;
    }

    public List<Task> getTasks() {
        return new ArrayList<>(tasks);
    }

    public void setTasks(List<Task> newTasks) {
        tasks.clear();
        tasks.addAll(newTasks);

        for (int i = 0; i < Math.min(newTasks.size(), checkboxes.size()); i++) {
            checkboxes.get(i).setSelected(newTasks.get(i).isCompleted());
            textFields.get(i).setText(newTasks.get(i).getDescription());
        }

        onTaskUpdated();
    }
}