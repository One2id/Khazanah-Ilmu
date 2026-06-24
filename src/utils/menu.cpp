#include "menu.h"
#include <iostream>
#include <limits>
#include <stdexcept>

int getMenuChoice(int min, int max) {
    int choice;
    while (true) {
        std::cout << "Enter your choice: ";
        if (std::cin >> choice && choice >= min && choice <= max) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a number between " << min << " and " << max << ".\n";
    }
}

std::string getStringInput(const std::string& prompt, bool allowEmpty) {
    std::string input;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, input);
        if (allowEmpty || !input.empty()) return input;
        std::cout << "This field cannot be empty. Please try again.\n";
    }
}

int getIntInput(const std::string& prompt, bool allowEmpty, int defaultVal) {
    while (true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        if (allowEmpty && line.empty()) return defaultVal;
        try {
            size_t pos;
            int val = std::stoi(line, &pos);
            if (pos == line.size()) return val;
        } catch (...) {}
        std::cout << "Invalid input. Please enter a valid integer.\n";
    }
}

double getDoubleInput(const std::string& prompt, bool allowEmpty, double defaultVal) {
    while (true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        if (allowEmpty && line.empty()) return defaultVal;
        try {
            size_t pos;
            double val = std::stod(line, &pos);
            if (pos == line.size()) return val;
        } catch (...) {}
        std::cout << "Invalid input. Please enter a valid number.\n";
    }
}

std::string getDateInput(const std::string& prompt, bool allowEmpty) {
    while (true) {
        std::cout << prompt;
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) {
            if (allowEmpty) return "";
            std::cout << "  Date is required. Please enter in YYYY-MM-DD format.\n";
            continue;
        }

        // Must be exactly 10 chars: YYYY-MM-DD
        bool valid = (input.size() == 10 &&
                      isdigit(input[0]) && isdigit(input[1]) &&
                      isdigit(input[2]) && isdigit(input[3]) &&
                      input[4] == '-' &&
                      isdigit(input[5]) && isdigit(input[6]) &&
                      input[7] == '-' &&
                      isdigit(input[8]) && isdigit(input[9]));

        if (valid) {
            int month = std::stoi(input.substr(5, 2));
            int day   = std::stoi(input.substr(8, 2));
            if (month < 1 || month > 12) valid = false;
            if (day   < 1 || day   > 31) valid = false;
        }

        if (valid) return input;
        std::cout << "  Invalid date format. Please use YYYY-MM-DD (e.g. 2024-03-15).\n";
    }
}

std::string getEnumInput(const std::string& prompt, const std::vector<std::string>& allowed,
                         bool allowEmpty, const std::string& defaultVal) {
    while (true) {
        std::cout << prompt;
        std::string input;
        std::getline(std::cin, input);
        if (input.empty() && allowEmpty) return defaultVal;
        for (const auto& v : allowed)
            if (input == v) return input;
        std::cout << "  Invalid. Allowed values: ";
        for (size_t i = 0; i < allowed.size(); i++) {
            std::cout << "'" << allowed[i] << "'";
            if (i < allowed.size() - 1) std::cout << " / ";
        }
        std::cout << ".\n";
    }
}

bool getConfirmation(const std::string& prompt) {
    while (true) {
        std::cout << prompt << " (y/n): ";
        std::string input;
        std::getline(std::cin, input);
        if (input == "y" || input == "Y") return true;
        if (input == "n" || input == "N") return false;
        std::cout << "Please enter 'y' or 'n'.\n";
    }
}
