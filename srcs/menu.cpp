#include "menu.hpp"
#include "utils.hpp"
#include "log.hpp"
#include <iostream>
#include <ctime>

using namespace std::chrono_literals;

namespace ui {

static std::string username() {
    const char* u = std::getenv("USER");
    return u ? std::string(u) : std::string("student");
}

void Menu::startupAnimation() const {
    clear_screen();
    std::string title = "examshell";
    for (size_t i = 0; i < title.size(); ++i) {
        std::cout << title[i] << std::flush;
        sleep_ms(70);
    }
    std::cout << std::endl;
    sleep_ms(600);
    clear_screen();
    std::cout << "\033[1m\033[4mExamShell\033[0m v2.1\n\n";
    std::cout << "\033[1mlogin:\033[0m " << std::flush;
    std::string login = username();
    for (size_t i = 0; i < login.size(); ++i) {
        std::cout << login[i] << std::flush;
        sleep_ms(100);
    }
    std::cout << "\n\033[1mpassword:\033[0m ";
    int password_size = 12;
    for (int i = 0; i < password_size; ++i) {
        std::cout << "*" << std::flush;
        sleep_ms(150);
    }
    std::cout << "\n\n";
    esh::Logger::instance().log(esh::Logger::Level::Info, "Startup animation shown", __FILE__, __LINE__, __func__);
}

void Menu::showDashboard(const std::string& modeName, long sessionSeconds) const {
    clear_screen();
    std::cout << "\033[1;96m╔══════════════════════════════════════════════════╗\033[0m\n";
    std::cout << "\033[1;96m║\033[0m            \033[1;32mExam Shell Dashboard\033[0m               \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m╠══════════════════════════════════════════════════╣\033[0m\n";
    std::cout << "\033[1;96m║\033[0m Current user: \033[1;94m" << username() << "\033[0m";
    std::cout << "                             \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m║\033[0m Current mode: \033[1;94m" << modeName << "\033[0m";
    std::cout << "                             \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m║\033[0m Session time: \033[1;90m" << sessionSeconds << "s\033[0m";
    std::cout << "                            \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m╠══════════════════════════════════════════════════╣\033[0m\n";
    std::cout << "\033[1;96m║\033[0m Quick actions:                                  \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m║\033[0m  - help     Show help                            \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m║\033[0m  - mode     Switch mode                          \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m║\033[0m  - status   Show this dashboard                  \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m║\033[0m  - clock    Show current time                    \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m║\033[0m  - grademe  Simulate grading                     \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m║\033[0m  - finish   Exit the shell                       \033[1;96m║\033[0m\n";
    std::cout << "\033[1;96m╚══════════════════════════════════════════════════╝\033[0m\n\n";
}

ModeChoice Menu::pickMode() const {
    while (true) {
        std::cout << "\n\033[1;95mSelect a mode:\033[0m\n";
        std::cout << "  1) Project evaluation\n";
        std::cout << "  2) Exam evaluation\n";
        std::cout << "  3) Sandbox / practice\n";
        std::cout << "  4) Back to menu / cancel\n";
        std::string choice;
        if (!read_line("Choice [1-4]: ", choice)) {
            std::cout << "\n";
            esh::Logger::instance().log(esh::Logger::Level::Info, "Mode selection cancelled (EOF)", __FILE__, __LINE__, __func__);
            return ModeChoice::Cancel;
        }
        choice = trim(choice);
        if (choice == "1") return ModeChoice::Project;
        if (choice == "2") return ModeChoice::Evaluation;
        if (choice == "3") return ModeChoice::Sandbox;
        if (choice == "4") return ModeChoice::Cancel;
        std::cout << "Invalid choice. Try again.\n";
        esh::Logger::instance().log(esh::Logger::Level::Warn, "Invalid mode choice: " + choice, __FILE__, __LINE__, __func__);
    }
}

int Menu::piscineMenu() const {
    std::string choice = "-2";
    while (choice == "-1" || choice == "-2") {
        clear_screen();
        std::cout << "\033[1m         42EXAM \n\033[31m   BACK\033[0m\033[1m to menu with \033[31m0\033[0m\n";
        std::cout << "\033[32m            \033[0m\n\n\033[32m            •           \033[0m\n\033[1m    |  Piscine PART  |\033[0m\n\n";
        std::cout << "\033[32m            1\033[0m\033[1m\n       EXAM WEEK 01\033[0m\n\n";
        std::cout << "\033[32m            2\033[0m\033[1m\n       EXAM WEEK 02\033[0m\n\n";
        std::cout << "\033[1m     \\ ------------ /\033[0m\n\n";
        std::cout << "    Enter your choice:\n            ";
        if (!std::getline(std::cin, choice)) return 0;
        if (choice != "1" && choice != "2" && choice != "0") choice = "-1";
    }
    return std::atoi(choice.c_str());
}

int Menu::studentMenu() const {
    std::string choice = "-2";
    while (choice == "-1" || choice == "-2") {
        clear_screen();
        std::cout << "\033[1m         42EXAM \n\033[31m   BACK\033[0m\033[1m to menu with \033[31m0\033[0m\n";
        std::cout << "\033[32m            \033[0m\n\n\033[1m    |  Student PART  |\033[0m\n\n";
        std::cout << "\033[32m            2\033[0m\033[1m\n       EXAM RANK 02\033[0m\n\n";
        std::cout << "\033[32m            3\033[0m\033[1m\n       EXAM RANK 03\033[0m\n\n";
        std::cout << "\033[32m            4\033[0m\033[1m\n       EXAM RANK 04\033[0m\n\n";
        std::cout << "\033[32m            5\033[0m\033[1m\n       EXAM RANK 05\033[0m\n\n";
        std::cout << "\033[32m            6\033[0m\033[1m\n       EXAM RANK 06\033[0m\n";
        std::cout << "\033[1m     \\ ------------ /\033[0m\n\n";
        std::cout << "    Enter your choice:\n            ";
        if (!std::getline(std::cin, choice)) return 0;
        if (choice != "2" && choice != "3" && choice != "4" && choice != "5" && choice != "6" && choice != "0")
            choice = "-1";
    }
    return std::atoi(choice.c_str());
}

void Menu::settingsMenu() const {
    clear_screen();
    std::cout << "\033[1m     === SETTINGS MENU ===\033[0m\n\033[31m          BACK\033[0m with \033[31m0\033[0m\n\n";
    std::cout << "This is a placeholder. Plug your settings here.\n";
    std::cout << "Press Enter to go back...";
    std::string tmp;
    std::getline(std::cin, tmp);
}

} // namespace ui
