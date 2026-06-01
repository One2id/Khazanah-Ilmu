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
