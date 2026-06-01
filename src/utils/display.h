#pragma once
#include <string>
#include <vector>

void printAppHeader();
void printSeparator(const std::vector<int>& widths);
void printRow(const std::vector<std::string>& values, const std::vector<int>& widths);
void printTableHeader(const std::vector<std::string>& headers, const std::vector<int>& widths);
void pressEnterToContinue();
