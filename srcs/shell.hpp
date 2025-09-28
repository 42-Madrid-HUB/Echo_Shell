#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

class Shell {
public:
    Shell();
    void run();

    enum class Mode {
        Menu,
        Project,     // Evaluate projects
        Evaluation,  // Evaluate evaluations
        Sandbox      // Practice / custom tests
    };

private:
    void backupEnvironment();
    void restoreEnvironment();
    void trackFileChange(const std::string& path);
    void trackEnvChange(const std::string& key, const std::string& value);
    void persistChanges();

    // Built-in framework
    using Handler = std::function<void(const std::vector<std::string>&)>;
    void setupBuiltins();
    void handleTokens(const std::vector<std::string>& tokens);
    std::vector<std::string> split(const std::string& line) const;

    // UI
    std::string buildPrompt() const;
    void showDashboard() const;
    void modeMenu();

    // State
    std::map<std::string, std::string> originalEnv;
    std::vector<std::string> changedFiles;
    std::map<std::string, std::string> changedEnv;
    std::string originalCwd;

    // New state
    Mode currentMode = Mode::Menu;
    std::chrono::system_clock::time_point sessionStart;
    std::map<std::string, Handler> commands;
    std::map<std::string, std::string> helpTexts;
    bool running = true;
};
