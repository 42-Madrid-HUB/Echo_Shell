#include "shell.hpp"
#include "utils.hpp"
#include "log.hpp"
#include "menu.hpp"
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <map>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <functional>
#include <chrono>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>

// Forward declare template renderer from utils.cpp
std::string render_template(const std::string& tpl, const std::map<std::string, std::string>& vars);

Shell::Shell() {
    backupEnvironment();
}

void Shell::backupEnvironment() {
    originalCwd = get_current_dir();
    originalEnv = get_env_map();
}

void Shell::restoreEnvironment() {
    chdir(originalCwd.c_str());
    for (const auto& kv : originalEnv) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }
}

void Shell::trackFileChange(const std::string& path) {
    changedFiles.push_back(path);
}

void Shell::trackEnvChange(const std::string& key, const std::string& value) {
    changedEnv[key] = value;
}

void Shell::persistChanges() {
    std::ofstream audit(".shell_audit", std::ios::app);
    audit << "Changed files:\n";
    for (const auto& f : changedFiles) {
        audit << f << "\n";
    }
    audit << "Changed env:\n";
    for (const auto& kv : changedEnv) {
        audit << kv.first << "=" << kv.second << "\n";
    }
    audit.close();
}

static void print_welcome() {
    // Build dynamic context
    const char* user = std::getenv("USER");
    if (!user) user = "student";

    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef __linux__
    localtime_r(&t, &tm);
#else
    tm = *std::localtime(&t);
#endif
    char ts[64];
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);

    std::map<std::string, std::string> ctx = {
        {"user", user},
        {"start_time", ts},
        {"product", "Exam Shell"}
    };

    std::string tpl =
        "\033[1;36m========================================\033[0m\n"
        "\033[1;32m  Welcome {{user}} to the {{product}}!\033[0m\n"
        "\033[1;36m========================================\033[0m\n"
        "Session start: {{start_time}}\n"
        "Use 'mode' to select evaluation mode, 'help' for commands.\n\n";

    std::cout << render_template(tpl, ctx);
    ESH_LOG_INFO() << "Welcome banner displayed for user=" << user;
}

// Replace previous SIGINT handler with a safe one (no prompt print here)
static volatile sig_atomic_t sigint_received = 0;
static void handle_sigint(int) {
    sigint_received = 1;
    // async-signal-safe: just write a newline
    write(STDOUT_FILENO, "\n", 1);
}

// Build a nice prompt with mode and time
std::string Shell::buildPrompt() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef __linux__
    localtime_r(&t, &tm);
#else
    tm = *std::localtime(&t);
#endif
    std::ostringstream ts;
    ts << std::put_time(&tm, "%H:%M:%S");

    const char* modeName = "MENU";
    switch (currentMode) {
        case Mode::Project: modeName = "PROJECT"; break;
        case Mode::Evaluation: modeName = "EVALUATION"; break;
        case Mode::Sandbox: modeName = "SANDBOX"; break;
        default: modeName = "MENU"; break;
    }

    std::ostringstream prompt;
    prompt << "\033[1;90m[" << ts.str() << "]\033[0m "
           << "\033[1;94m" << modeName << "\033[0m"
           << " \033[1;93mexamshell\033[0m$ ";
    return prompt.str();
}

// Replace old dashboard with delegation to Menu
void Shell::showDashboard() const {
    ui::Menu menu;
    // Build current mode name and elapsed time
    const char* modeName = "MENU";
    switch (currentMode) {
        case Mode::Project: modeName = "PROJECT"; break;
        case Mode::Evaluation: modeName = "EVALUATION"; break;
        case Mode::Sandbox: modeName = "SANDBOX"; break;
        default: modeName = "MENU"; break;
    }
    auto now = std::chrono::system_clock::now();
    long secs = std::chrono::duration_cast<std::chrono::seconds>(now - sessionStart).count();
    if (secs < 0) secs = 0;
    menu.showDashboard(modeName, secs);
}

// Replace old modeMenu with delegation to Menu
void Shell::modeMenu() {
    ui::Menu menu;
    while (true) {
        ui::ModeChoice c = menu.pickMode();
        if (c == ui::ModeChoice::Cancel) {
            ESH_LOG_INFO() << "Mode selection cancelled";
            currentMode = Mode::Menu;
            return;
        }
        if (c == ui::ModeChoice::Project)    { currentMode = Mode::Project;    ESH_LOG_INFO() << "Mode set to PROJECT";    break; }
        if (c == ui::ModeChoice::Evaluation) { currentMode = Mode::Evaluation; ESH_LOG_INFO() << "Mode set to EVALUATION"; break; }
        if (c == ui::ModeChoice::Sandbox)    { currentMode = Mode::Sandbox;    ESH_LOG_INFO() << "Mode set to SANDBOX";    break; }
    }
    sessionStart = std::chrono::system_clock::now();
    showDashboard();
}

// Tokenize input by spaces (simple)
std::vector<std::string> Shell::split(const std::string& line) const {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

// Register built-in commands
void Shell::setupBuiltins() {
    helpTexts.clear();
    commands.clear();

    commands["help"] = [this](const std::vector<std::string>&) {
        std::cout << "\033[1;34mAvailable commands:\033[0m\n";
        for (const auto& kv : helpTexts) {
            std::cout << "  \033[1;33m" << kv.first << "\033[0m  - " << kv.second << "\n";
        }
        ESH_LOG_DEBUG() << "Displayed help";
    };
    helpTexts["help"] = "Show this help message";

    commands["finish"] = [this](const std::vector<std::string>&) {
        std::cout << "Are you sure you want to \033[1;31mexit\033[0m the exam?\n";
        std::cout << "All your progress will be \033[1;31mlost\033[0m.\n";
        std::cout << "Type '\033[1;32myes\033[0m' to confirm: ";
        std::string confirm;
        std::getline(std::cin, confirm);
        if (confirm == "yes") {
            ESH_LOG_INFO() << "User confirmed exit";
            running = false;
        } else {
            std::cout << " ** Abort ** \n";
            ESH_LOG_DEBUG() << "User aborted exit";
        }
    };
    helpTexts["finish"] = "Exit the exam shell";

    commands["clock"] = [this](const std::vector<std::string>&) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef __linux__
        localtime_r(&t, &tm);
#else
        tm = *std::localtime(&t);
#endif
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        std::cout << "Current time: " << buf << "\n";
        ESH_LOG_DEBUG() << "Clock requested";
    };
    helpTexts["clock"] = "Show current time";

    commands["grademe"] = [this](const std::vector<std::string>&) {
        std::cout << "\033[1;32mGrading in progress...\033[0m\n";
        std::cout << "Mode: ";
        switch (currentMode) {
            case Mode::Project: std::cout << "PROJECT\n"; break;
            case Mode::Evaluation: std::cout << "EVALUATION\n"; break;
            case Mode::Sandbox: std::cout << "SANDBOX\n"; break;
            default: std::cout << "MENU\n"; break;
        }
        std::cout << "This is a placeholder. Plug your grading logic here.\n";
        ESH_LOG_INFO() << "Grademe invoked in mode=" << (currentMode == Mode::Project ? "PROJECT" :
                                                       currentMode == Mode::Evaluation ? "EVALUATION" :
                                                       currentMode == Mode::Sandbox ? "SANDBOX" : "MENU");
    };
    helpTexts["grademe"] = "Simulate grading (placeholder)";

    commands["mode"] = [this](const std::vector<std::string>&) { modeMenu(); };
    helpTexts["mode"] = "Switch mode (Project/Evaluation/Sandbox)";

    commands["status"] = [this](const std::vector<std::string>&) { showDashboard(); ESH_LOG_DEBUG() << "Status displayed"; };
    helpTexts["status"] = "Show dashboard";

    commands["clear"] = [](const std::vector<std::string>&) { std::cout << "\033[2J\033[H"; };
    helpTexts["clear"] = "Clear the screen";
}

// Dispatch tokens to a handler
void Shell::handleTokens(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return;
    ESH_LOG_DEBUG() << "Dispatch command=" << tokens[0] << " argc=" << (tokens.size() - 1);
    auto it = commands.find(tokens[0]);
    if (it != commands.end()) {
        it->second(tokens);
    } else {
        std::cout << "           **Unknown command**     type \033[1;33mhelp\033[0m for more help\n";
        ESH_LOG_WARN() << "Unknown command: " << tokens[0];
    }
}

void Shell::run() {
    ESH_LOG_INFO() << "Shell run() entered";
    print_welcome();

    // SIGINT handler (Ctrl+C -> newline + fresh prompt)
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // let readline return on SIGINT
    sigaction(SIGINT, &sa, nullptr);

    setupBuiltins();
    sessionStart = std::chrono::system_clock::now();

    // Fancy startup
    ui::Menu menu;
    menu.startupAnimation();
    showDashboard();
    modeMenu();

    while (running) {
        sigint_received = 0;
        std::string prompt = buildPrompt();
        char* input = readline(prompt.c_str());
        if (!input) {
            if (sigint_received) {
                ESH_LOG_DEBUG() << "Input interrupted by SIGINT";
                continue;
            }
            ESH_LOG_INFO() << "EOF received, exiting loop";
            break;
        }
        std::string line = input;
        free(input);

        line = trim(line);
        if (line.empty()) continue;

        add_history(line.c_str());
        auto tokens = split(line);
        handleTokens(tokens);
    }
    persistChanges();
    restoreEnvironment();
    ESH_LOG_INFO() << "Shell run() exited";
}