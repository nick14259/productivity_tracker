package main.util;

import java.awt.Color;

public class ColorUtils {
    // Progress colors for task completion (0 to 5 tasks)
    public static final Color[] PROGRESS_COLORS = {
            new Color(0, 0, 0),      // 0 tasks - Off
            new Color(18, 54, 0),    // 1 task  - Red
            new Color(37, 107, 0),  // 2 tasks - Orange
            new Color(67, 161, 0),  // 3 tasks - Yellow
            new Color(76, 199, 0),    // 4 tasks - Green
            new Color(92, 255, 0)     // 5 tasks - Blue
    };

    public static Color getColorForTaskCount(int completedTasks) {
        if (completedTasks < 0) return PROGRESS_COLORS[0];
        if (completedTasks >= PROGRESS_COLORS.length) return PROGRESS_COLORS[PROGRESS_COLORS.length - 1];
        return PROGRESS_COLORS[completedTasks];
    }

    public static String colorToHex(Color color) {
        return String.format("#%02x%02x%02x",
                color.getRed(), color.getGreen(), color.getBlue());
    }

    public static String getColorDescription(Color color) {
        for (int i = 0; i < PROGRESS_COLORS.length; i++) {
            if (PROGRESS_COLORS[i].equals(color)) {
                return i + " tasks completed";
            }
        }
        return "Custom color";
    }
}