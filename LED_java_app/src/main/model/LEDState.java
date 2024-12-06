package main.model;

import java.awt.Color;
import java.time.LocalDate;
import org.json.JSONObject;

public class LEDState {
    private int index;
    private Color color;
    private LocalDate date;

    public LEDState(int index, Color color, LocalDate date) {
        this.index = index;
        this.color = color;
        this.date = date;
    }

    public int getIndex() { return index; }
    public void setIndex(int index) { this.index = index; }
    public Color getColor() { return color; }
    public void setColor(Color color) { this.color = color; }
    public LocalDate getDate() { return date; }
    public void setDate(LocalDate date) { this.date = date; }

    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        json.put("index", index);
        json.put("red", color.getRed());
        json.put("green", color.getGreen());
        json.put("blue", color.getBlue());
        json.put("date", date.toString());
        return json;
    }

    public static LEDState fromJson(JSONObject json) {
        Color color = new Color(
                json.getInt("red"),
                json.getInt("green"),
                json.getInt("blue")
        );
        return new LEDState(
                json.getInt("index"),
                color,
                LocalDate.parse(json.getString("date"))
        );
    }

    @Override
    public String toString() {
        return String.format("LED %d: RGB(%d,%d,%d) on %s",
                index, color.getRed(), color.getGreen(), color.getBlue(), date);
    }
}