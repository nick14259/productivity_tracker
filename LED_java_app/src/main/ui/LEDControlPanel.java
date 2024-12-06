// LEDControlPanel.java
package main.ui;

import main.model.LEDState;
import javax.swing.*;
import java.awt.*;
import java.util.function.Consumer;

public class LEDControlPanel extends JPanel {
    private static final int NUM_COLUMNS = 12;  // 84 LEDs = 7 rows of 12
    private static final int NUM_ROWS = 7;
    private final JButton[] ledButtons;
    private final Consumer<LEDState> onLedSelected;

    public LEDControlPanel(Consumer<LEDState> onLedSelected) {
        this.onLedSelected = onLedSelected;
        this.ledButtons = new JButton[84];

        setLayout(new GridLayout(NUM_ROWS, NUM_COLUMNS, 2, 2));
        setBorder(BorderFactory.createTitledBorder("LED Array"));

        createLEDGrid();
    }

    private void createLEDGrid() {
        for (int i = 0; i < ledButtons.length; i++) {
            final int index = i;
            JButton button = new JButton(String.valueOf(i));
            button.setPreferredSize(new Dimension(40, 40));
            button.setMargin(new Insets(0, 0, 0, 0));
            button.addActionListener(e -> handleLedClick(index));

            ledButtons[i] = button;
            add(button);
        }
    }

    private void handleLedClick(int index) {
        Color color = JColorChooser.showDialog(
                this,
                "Choose LED Color",
                ledButtons[index].getBackground()
        );

        if (color != null) {
            updateLedColor(index, color);
        }
    }

    public void updateLedColor(int index, Color color) {
        if (index >= 0 && index < ledButtons.length) {
            ledButtons[index].setBackground(color);
            ledButtons[index].setForeground(
                    (color.getRed() + color.getGreen() + color.getBlue()) / 3 > 128
                            ? Color.BLACK
                            : Color.WHITE
            );

            if (onLedSelected != null) {
                onLedSelected.accept(new LEDState(index, color, java.time.LocalDate.now()));
            }
        }
    }

    public void clearAllLeds() {
        for (int i = 0; i < ledButtons.length; i++) {
            updateLedColor(i, Color.BLACK);
        }
    }
}