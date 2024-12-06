// HistoryDialog.java
package main.ui.dialogs;

import main.controller.HistoryManager.HistoryEntry;
import main.util.DateUtils;

import javax.swing.*;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.util.List;

public class HistoryDialog extends JDialog {
    private final DefaultTableModel tableModel;

    public HistoryDialog(Frame owner, List<HistoryEntry> history) {
        super(owner, "Task History", true);
        setLayout(new BorderLayout(10, 10));

        // Create table model
        tableModel = new DefaultTableModel(
                new String[]{"Date", "Progress", "Color"}, 0) {
            @Override
            public boolean isCellEditable(int row, int column) {
                return false;
            }
        };

        createUI();
        loadHistory(history);

        pack();
        setLocationRelativeTo(owner);
        setMinimumSize(new Dimension(400, 300));
    }

    private void createUI() {
        JTable table = new JTable(tableModel);
        table.setRowHeight(25);
        table.getColumnModel().getColumn(2).setCellRenderer(new ColorRenderer());

        JScrollPane scrollPane = new JScrollPane(table);
        scrollPane.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));

        add(scrollPane, BorderLayout.CENTER);

        JButton closeButton = new JButton("Close");
        closeButton.addActionListener(e -> dispose());

        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        buttonPanel.add(closeButton);
        add(buttonPanel, BorderLayout.SOUTH);
    }

    private void loadHistory(List<HistoryEntry> history) {
        tableModel.setRowCount(0);
        for (HistoryEntry entry : history) {
            tableModel.addRow(new Object[]{
                    DateUtils.getRelativeDateDescription(entry.date),
                    entry.description,
                    entry.color
            });
        }
    }

    private static class ColorRenderer extends DefaultTableCellRenderer {
        @Override
        public Component getTableCellRendererComponent(
                JTable table, Object value, boolean isSelected, boolean hasFocus,
                int row, int column) {

            super.getTableCellRendererComponent(table, "", isSelected, hasFocus, row, column);

            if (value instanceof Color) {
                Color color = (Color) value;
                setBackground(color);
                setForeground(
                        (color.getRed() + color.getGreen() + color.getBlue()) / 3 > 128
                                ? Color.BLACK
                                : Color.WHITE
                );
                setBorder(BorderFactory.createLineBorder(Color.GRAY));
            }

            return this;
        }
    }
}