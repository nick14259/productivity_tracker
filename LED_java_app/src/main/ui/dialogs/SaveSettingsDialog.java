// SaveSettingsDialog.java
package main.ui.dialogs;

import main.model.SaveConfig;
import javax.swing.*;
import java.awt.*;

public class SaveSettingsDialog extends JDialog {
    private final SaveConfig config;
    private boolean configChanged = false;
    private final JComboBox<SaveConfig.SaveFrequency> frequencyCombo;
    private final JSpinner maxAutoSavesSpinner;
    private final JCheckBox autoSaveOnCloseCheck;

    public SaveSettingsDialog(Frame owner, SaveConfig config) {
        super(owner, "Auto-save Settings", true);
        this.config = config;

        setLayout(new BorderLayout(10, 10));

        frequencyCombo = new JComboBox<>(SaveConfig.SaveFrequency.values());
        maxAutoSavesSpinner = new JSpinner(new SpinnerNumberModel(3, 1, 10, 1));
        autoSaveOnCloseCheck = new JCheckBox("Auto-save on close");

        initializeUI();
        loadCurrentSettings();

        pack();
        setLocationRelativeTo(owner);
        setResizable(false);
    }

    private void initializeUI() {
        JPanel settingsPanel = new JPanel(new GridBagLayout());
        settingsPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets(5, 5, 5, 5);

        // Frequency settings
        gbc.gridx = 0; gbc.gridy = 0;
        settingsPanel.add(new JLabel("Save Frequency:"), gbc);

        gbc.gridx = 1;
        settingsPanel.add(frequencyCombo, gbc);

        // Max auto-saves
        gbc.gridx = 0; gbc.gridy = 1;
        settingsPanel.add(new JLabel("Keep last:"), gbc);

        gbc.gridx = 1;
        JPanel spinnerPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 5, 0));
        spinnerPanel.add(maxAutoSavesSpinner);
        spinnerPanel.add(new JLabel("auto-saves"));
        settingsPanel.add(spinnerPanel, gbc);

        // Auto-save on close
        gbc.gridx = 0; gbc.gridy = 2;
        gbc.gridwidth = 2;
        settingsPanel.add(autoSaveOnCloseCheck, gbc);

        add(settingsPanel, BorderLayout.CENTER);

        // Buttons
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));

        JButton saveButton = new JButton("Save");
        saveButton.addActionListener(e -> saveSettings());

        JButton cancelButton = new JButton("Cancel");
        cancelButton.addActionListener(e -> dispose());

        buttonPanel.add(saveButton);
        buttonPanel.add(cancelButton);

        add(buttonPanel, BorderLayout.SOUTH);
    }

    private void loadCurrentSettings() {
        frequencyCombo.setSelectedItem(config.getFrequency());
        maxAutoSavesSpinner.setValue(config.getMaxAutoSaves());
        autoSaveOnCloseCheck.setSelected(config.isAutoSaveOnClose());
    }

    private void saveSettings() {
        SaveConfig.SaveFrequency newFrequency =
                (SaveConfig.SaveFrequency) frequencyCombo.getSelectedItem();
        int newMaxAutoSaves = (Integer) maxAutoSavesSpinner.getValue();
        boolean newAutoSaveOnClose = autoSaveOnCloseCheck.isSelected();

        boolean changed =
                newFrequency != config.getFrequency() ||
                        newMaxAutoSaves != config.getMaxAutoSaves() ||
                        newAutoSaveOnClose != config.isAutoSaveOnClose();

        if (changed) {
            config.setFrequency(newFrequency);
            config.setMaxAutoSaves(newMaxAutoSaves);
            config.setAutoSaveOnClose(newAutoSaveOnClose);
            configChanged = true;
        }

        dispose();
    }

    public boolean isConfigChanged() {
        return configChanged;
    }
}