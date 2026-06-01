#pragma once
#include <string>

int getMenuChoice(int min, int max);
std::string getStringInput(const std::string& prompt, bool allowEmpty = false);
int getIntInput(const std::string& prompt, bool allowEmpty = false, int defaultVal = 0);
double getDoubleInput(const std::string& prompt, bool allowEmpty = false, double defaultVal = 0.0);
bool getConfirmation(const std::string& prompt);
