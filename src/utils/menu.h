#pragma once
#include <string>

int getMenuChoice(int min, int max);
std::string getStringInput(const std::string& prompt, bool allowEmpty = false);
int getIntInput(const std::string& prompt, bool allowEmpty = false, int defaultVal = 0);
double getDoubleInput(const std::string& prompt, bool allowEmpty = false, double defaultVal = 0.0);
bool getConfirmation(const std::string& prompt);

// Validates YYYY-MM-DD format (month 01-12, day 01-31).
// allowEmpty=true means blank is accepted and returns "".
std::string getDateInput(const std::string& prompt, bool allowEmpty = false);

// Loops until user enters one of the allowed values.
// allowEmpty=true means blank returns defaultVal.
#include <vector>
std::string getEnumInput(const std::string& prompt, const std::vector<std::string>& allowed,
                         bool allowEmpty = false, const std::string& defaultVal = "");
