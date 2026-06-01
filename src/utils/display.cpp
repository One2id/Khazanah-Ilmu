#include "display.h"
#include <iostream>
#include <iomanip>
#include <limits>

void printAppHeader() {
    std::cout << "\n";
    std::cout << "==============================================\n";
    std::cout << "     KHAZANAH ILMU  |  خزانة علم\n";
    std::cout << "   Historical & Multilingual Library System\n";
    std::cout << "==============================================\n";
}

void printSeparator(const std::vector<int>& widths) {
    std::cout << "+";
    for (int w : widths) {
        for (int i = 0; i < w + 2; i++) std::cout << "-";
        std::cout << "+";
    }
    std::cout << "\n";
}

void printRow(const std::vector<std::string>& values, const std::vector<int>& widths) {
    std::cout << "|";
    for (size_t i = 0; i < widths.size(); i++) {
        std::string val = (i < values.size()) ? values[i] : "";
        if ((int)val.size() > widths[i]) {
            val = val.substr(0, widths[i] - 3) + "...";
        }
        std::cout << " " << std::left << std::setw(widths[i]) << val << " |";
    }
    std::cout << "\n";
}

void printTableHeader(const std::vector<std::string>& headers, const std::vector<int>& widths) {
    printSeparator(widths);
    printRow(headers, widths);
    printSeparator(widths);
}

void pressEnterToContinue() {
    std::cout << "\nPress Enter to continue...";
    std::cin.get();
}
