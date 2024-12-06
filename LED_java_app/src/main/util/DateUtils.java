package main.util;

import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class DateUtils {
    private static final DateTimeFormatter DATE_FORMATTER =
            DateTimeFormatter.ofPattern("MMMM d, yyyy");
    private static final DateTimeFormatter TIME_FORMATTER =
            DateTimeFormatter.ofPattern("HH:mm:ss");
    private static final DateTimeFormatter DATETIME_FORMATTER =
            DateTimeFormatter.ofPattern("MMM d, yyyy HH:mm:ss");
    private static final DateTimeFormatter FILENAME_FORMATTER =
            DateTimeFormatter.ofPattern("yyyy-MM-dd_HH-mm");

    public static String formatDate(LocalDate date) {
        return date.format(DATE_FORMATTER);
    }

    public static String formatTime(LocalDateTime time) {
        return time.format(TIME_FORMATTER);
    }

    public static String formatDateTime(LocalDateTime dateTime) {
        return dateTime.format(DATETIME_FORMATTER);
    }

    public static String formatForFilename(LocalDateTime dateTime) {
        return dateTime.format(FILENAME_FORMATTER);
    }

    public static String getRelativeDateDescription(LocalDate date) {
        LocalDate now = LocalDate.now();
        if (date.equals(now)) return "Today";
        if (date.equals(now.minusDays(1))) return "Yesterday";
        if (date.equals(now.plusDays(1))) return "Tomorrow";
        return formatDate(date);
    }
}