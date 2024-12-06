// RestoreDialog.java
package main.ui.dialogs;

import main.controller.SaveManager;
import javax.swing.*;
import java.awt.*;
import java.io.IOException;
import java.nio.file.*;
import java.util.List;
import java.util.function.Consumer;

public class RestoreDialog extends JDialog {
    private final JList<Path> savesList;
    private final DefaultListModel<Path> listModel;
    private final Consumer<Path> onRestore;

    public RestoreDialog(Frame owner, SaveManager saveManager, Consumer<Path> onRestore) {
        super(owner, "Restore from Save", true);
        this.onRestore = onRestore;

        setLayout(new BorderLayout(10, 10));
        listModel = new DefaultListModel<>();
        savesList = new JList<>(listModel);

        loadSaveFiles(saveManager);
        createUI();

        pack();
        setLocationRelativeTo(owner);
    }

    private void createUI() {
        // List of saves
        savesList.setCellRenderer(new SaveFileRenderer());
        savesList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        JScrollPane scrollPane = new JScrollPane(savesList);
        scrollPane.setBorder(BorderFactory.createTitledBorder("Available Saves"));
        scrollPane.setPreferredSize(new Dimension(400, 300));

        add(scrollPane, BorderLayout.CENTER);

        // Buttons
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));

        JButton restoreButton = new JButton("Restore");
        restoreButton.addActionListener(e -> handleRestore());

        JButton cancelButton = new JButton("Cancel");
        cancelButton.addActionListener(e -> dispose());

        buttonPanel.add(restoreButton);
        buttonPanel.add(cancelButton);

        add(buttonPanel, BorderLayout.SOUTH);
    }

    private void loadSaveFiles(SaveManager saveManager) {
        try {
            List<Path> saves = saveManager.listSaveFiles();
            listModel.clear();
            saves.forEach(listModel::addElement);
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this,
                    "Error loading save files: " + e.getMessage(),
                    "Error",
                    JOptionPane.ERROR_MESSAGE);
        }
    }

    private void handleRestore() {
        Path selected = savesList.getSelectedValue();
        if (selected == null) {
            JOptionPane.showMessageDialog(this,
                    "Please select a save file to restore.",
                    "No Selection",
                    JOptionPane.WARNING_MESSAGE);
            return;
        }

        int result = JOptionPane.showConfirmDialog(this,
                "Are you sure you want to restore from this save?\n" +
                        "Current progress will be lost.",
                "Confirm Restore",
                JOptionPane.YES_NO_OPTION,
                JOptionPane.WARNING_MESSAGE);

        if (result == JOptionPane.YES_OPTION) {
            onRestore.accept(selected);
            dispose();
        }
    }

    private static class SaveFileRenderer extends DefaultListCellRenderer {
        @Override
        public Component getListCellRendererComponent(
                JList<?> list, Object value, int index,
                boolean isSelected, boolean cellHasFocus) {

            super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);

            if (value instanceof Path) {
                Path path = (Path) value;
                setText(path.getFileName().toString());
                setToolTipText(path.toString());
            }

            return this;
        }
    }
}