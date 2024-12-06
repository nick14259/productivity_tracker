package main.ui;

import main.controller.*;
import main.model.*;
import main.util.*;
import javax.swing.*;
import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.event.*;
import java.nio.file.*;
import java.util.prefs.Preferences;
import java.util.*;
import java.io.*;
import java.time.LocalDateTime;
import org.json.*;

public class MainFrame extends JFrame {
    private final ESP32Controller esp32Controller;
    private final SaveManager saveManager;
    private final HistoryManager historyManager;
    private final SaveConfig saveConfig;

    private TaskPanel taskPanel;
    private JTextField ipAddressField;
    private JTextArea responseArea;
    private JLabel statusLabel;
    private final Preferences prefs;

    public MainFrame() {
        setTitle("LED Task Tracker");
        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        setSize(800, 600);

        // Initialize controllers
        this.esp32Controller = new ESP32Controller();
        this.saveConfig = new SaveConfig();
        this.saveManager = new SaveManager(saveConfig);
        this.historyManager = new HistoryManager(esp32Controller);
        this.prefs = Preferences.userNodeForPackage(MainFrame.class);

        // Initialize task panel before creating main panel
        this.taskPanel = new TaskPanel(historyManager);
        taskPanel.setSaveCallback(() -> saveManager.scheduleAutoSave(() -> saveCurrentState(null)));
        taskPanel.setEnabled(false);  // Disabled until ESP32 connection

        // Create UI components after taskPanel initialization
        createMainPanel();
        createMenuBar();
        loadPreferences();

        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                handleClosing();
            }
        });

        pack();
        setLocationRelativeTo(null);
    }

    private void createMainPanel() {
        JPanel mainPanel = new JPanel(new BorderLayout(5, 5));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));

        // IP Address panel
        JPanel ipPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        ipAddressField = new JTextField(15);
        JButton connectButton = new JButton("Connect");
        connectButton.addActionListener(e -> testConnection());
        ipPanel.add(new JLabel("ESP32 IP:"));
        ipPanel.add(ipAddressField);
        ipPanel.add(connectButton);

        // Status panel
        JPanel statusPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        statusLabel = new JLabel("Not connected");
        statusPanel.add(statusLabel);

        // Response area
        responseArea = new JTextArea(5, 40);
        responseArea.setEditable(false);
        JScrollPane scrollPane = new JScrollPane(responseArea);

        // Layout
        JPanel topPanel = new JPanel(new BorderLayout());
        topPanel.add(ipPanel, BorderLayout.NORTH);
        topPanel.add(statusPanel, BorderLayout.SOUTH);

        mainPanel.add(topPanel, BorderLayout.NORTH);
        mainPanel.add(taskPanel, BorderLayout.CENTER);
        mainPanel.add(scrollPane, BorderLayout.SOUTH);

        add(mainPanel);
    }

    private void createMenuBar() {
        JMenuBar menuBar = new JMenuBar();

        // File Menu
        JMenu fileMenu = new JMenu("File");
        fileMenu.setMnemonic(KeyEvent.VK_F);

        JMenuItem saveDir = new JMenuItem("Set Save Directory...", KeyEvent.VK_D);
        saveDir.addActionListener(e -> chooseSaveDirectory());

        JMenuItem saveNow = new JMenuItem("Save Now", KeyEvent.VK_S);
        saveNow.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_S, InputEvent.CTRL_DOWN_MASK));
        saveNow.addActionListener(e -> saveCurrentState(null));

        JMenuItem loadState = new JMenuItem("Load State...", KeyEvent.VK_L);
        loadState.addActionListener(e -> {
            JFileChooser chooser = new JFileChooser(saveConfig.getSaveDirPath().toFile());
            if (chooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
                loadState(chooser.getSelectedFile().toPath());
            }
        });

        JMenuItem exit = new JMenuItem("Exit", KeyEvent.VK_X);
        exit.addActionListener(e -> handleClosing());

        fileMenu.add(saveDir);
        fileMenu.add(saveNow);
        fileMenu.add(loadState);
        fileMenu.addSeparator();
        fileMenu.add(exit);

        menuBar.add(fileMenu);
        setJMenuBar(menuBar);
    }

    private void chooseSaveDirectory() {
        JFileChooser chooser = new JFileChooser();
        chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
        if (saveConfig.getSaveDirPath() != null) {
            chooser.setCurrentDirectory(saveConfig.getSaveDirPath().toFile());
        }

        if (chooser.showDialog(this, "Select Save Directory") == JFileChooser.APPROVE_OPTION) {
            Path selectedPath = chooser.getSelectedFile().toPath();
            saveConfig.setSaveDirPath(selectedPath);
            prefs.put("save_dir", selectedPath.toString());
        }
    }

    private void loadPreferences() {
        String savedIp = prefs.get("esp32_ip", "");
        if (!savedIp.isEmpty()) {
            ipAddressField.setText(savedIp);
            esp32Controller.setIpAddress(savedIp);
        }

        String saveDir = prefs.get("save_dir", "");
        if (!saveDir.isEmpty()) {
            saveConfig.setSaveDirPath(Path.of(saveDir));
        }
    }

    private void saveCurrentState(Path path) {
        if (path == null && saveConfig.getSaveDirPath() == null) {
            chooseSaveDirectory();
            if (saveConfig.getSaveDirPath() == null) {
                return;
            }
            path = saveConfig.getSaveDirPath().resolve("led_progress_auto_" +
                    DateUtils.formatForFilename(LocalDateTime.now()) + ".json");
        }

        JSONObject saveData = new JSONObject();
        saveData.put("tasks", new JSONArray(taskPanel.getTasks()));
        saveData.put("history", historyManager.toJson());

        try {
            Files.writeString(path, saveData.toString(2));
            responseArea.append("State saved to: " + path + "\n");
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this,
                    "Error saving state: " + e.getMessage(),
                    "Save Error",
                    JOptionPane.ERROR_MESSAGE);
        }
    }

    private void loadState(Path path) {
        try {
            String jsonStr = Files.readString(path);
            JSONObject saveData = new JSONObject(jsonStr);

            JSONArray tasksArray = saveData.getJSONArray("tasks");
            List<Task> tasks = new ArrayList<>();
            for (int i = 0; i < tasksArray.length(); i++) {
                tasks.add(Task.fromJson(tasksArray.getJSONObject(i)));
            }
            taskPanel.setTasks(tasks);

            historyManager.fromJson(saveData.getJSONObject("history"));
            responseArea.append("State loaded from: " + path + "\n");

        } catch (IOException | JSONException e) {
            JOptionPane.showMessageDialog(this,
                    "Error loading state: " + e.getMessage(),
                    "Load Error",
                    JOptionPane.ERROR_MESSAGE);
        }
    }

    private void testConnection() {
        String ip = ipAddressField.getText().trim();
        esp32Controller.setIpAddress(ip);
        prefs.put("esp32_ip", ip);

        statusLabel.setText("Testing connection...");
        esp32Controller.testConnection().thenAccept(success -> {
            SwingUtilities.invokeLater(() -> {
                if (success) {
                    statusLabel.setText("Connected");
                    taskPanel.setEnabled(true);
                    responseArea.append("Connected to ESP32 at " + ip + "\n");
                } else {
                    statusLabel.setText("Connection failed");
                    taskPanel.setEnabled(false);
                    responseArea.append("Failed to connect to ESP32 at " + ip + "\n");
                }
            });
        });
    }

    private void handleClosing() {
        if (saveConfig.isAutoSaveOnClose()) {
            saveCurrentState(null);
            dispose();
            System.exit(0);
        } else {
            int result = JOptionPane.showConfirmDialog(this,
                    "Save before exiting?",
                    "Exit",
                    JOptionPane.YES_NO_CANCEL_OPTION);

            if (result == JOptionPane.YES_OPTION) {
                saveCurrentState(null);
                dispose();
                System.exit(0);
            } else if (result == JOptionPane.NO_OPTION) {
                dispose();
                System.exit(0);
            }
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            try {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } catch (Exception e) {
                e.printStackTrace();
            }
            new MainFrame().setVisible(true);
        });
    }
}