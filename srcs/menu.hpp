#pragma once
#include <string>
#include <chrono>

namespace ui {

enum class ModeChoice {
    Cancel = 0,
    Project = 1,
    Evaluation = 2,
    Sandbox = 3
};

class Menu {
public:
    void startupAnimation() const;
    void showDashboard(const std::string& modeName, long sessionSeconds) const;
    ModeChoice pickMode() const;

    // Optional: higher-level sections similar to legacy menus
    int piscineMenu() const; // returns 0 to go back, 1..2 choices
    int studentMenu() const; // returns 0 to go back, 2..6 choices
    void settingsMenu() const; // placeholder to plug real settings
};

} // namespace ui
