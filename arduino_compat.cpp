#include "arduino_compat.h"
#include <deque>
#include <mutex>
#include <condition_variable>

// Один набор объектов — тут реальные определения
std::deque<std::string> g_serialQueue;
std::mutex g_serialMutex;
std::condition_variable g_serialCv;

// Define the global output callback and Serial instance
std::function<void(const std::string&)> g_outputCallback = nullptr;
SerialClass Serial;
