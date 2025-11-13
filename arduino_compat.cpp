#include "arduino_compat.h"

// Define the global output callback and Serial instance
std::function<void(const std::string&)> g_outputCallback = nullptr;
SerialClass Serial;