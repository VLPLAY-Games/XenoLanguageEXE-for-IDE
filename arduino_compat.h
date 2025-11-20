#pragma once

#ifndef ARDUINO_BRIDGE
#define ARDUINO_BRIDGE

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <cctype>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <iomanip>
#include <limits>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <deque>

// Arduino compatibility layer
#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define SERIAL_BUFFER_SIZE 64
#define LED_BUILTIN 13

typedef bool boolean;
typedef uint8_t byte;

// Forward declarations
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
void delay(unsigned long ms);
unsigned long millis(void);

// Global output callback for redirecting output
extern std::function<void(const std::string&)> g_outputCallback;

// Serial input queue (for VM INPUT)
extern std::deque<std::string> g_serialQueue;
extern std::mutex g_serialMutex;
extern std::condition_variable g_serialCv;

// Функция для помещения строки в очередь - inline можно оставить
inline void SerialPushInput(const std::string& s) {
    std::lock_guard<std::mutex> lk(g_serialMutex);
    std::string copy = s;
    while (!copy.empty() && (copy.back() == '\r' || copy.back() == '\n')) copy.pop_back();
    g_serialQueue.push_back(copy);
    g_serialCv.notify_one();
}


class XenoString {
private:
    std::string str;

public:
    XenoString() : str("") {}
    XenoString(const char* s) : str(s ? s : "") {}
    XenoString(const std::string& s) : str(s) {}
    XenoString(char c) : str(1, c) {}
    XenoString(int value) : str(std::to_string(value)) {}
    XenoString(unsigned int value) : str(std::to_string(value)) {}
    XenoString(long value) : str(std::to_string(value)) {}
    XenoString(unsigned long value) : str(std::to_string(value)) {}
    XenoString(float value) : str(std::to_string(value)) {}
    XenoString(double value) : str(std::to_string(value)) {}
    XenoString(const XenoString& other) : str(other.str) {}
    XenoString(uint16_t value) : str(std::to_string(value)) {}
    XenoString(int16_t value) : str(std::to_string(value)) {}
    XenoString(size_t value) : str(std::to_string(value)) {}
    XenoString(float value, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        str = ss.str();
    }
    XenoString(double value, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        str = ss.str();
    }
    XenoString(long long value) : str(std::to_string(value)) {}

    // Conversion operators
    operator std::string() const { return str; }
    const char* c_str() const { return str.c_str(); }
    std::string toString() const { return str; }

    // Capacity
    size_t length() const { return str.length(); }
    size_t size() const { return str.size(); }
    bool isEmpty() const { return str.empty(); }
    void clear() { str.clear(); }
    void reserve(size_t size) { str.reserve(size); }
    size_t capacity() const { return str.capacity(); }

    // Access
    char charAt(size_t index) const {
        return index < str.length() ? str[index] : '\0';
    }
    char operator[](size_t index) const { return charAt(index); }
    char& operator[](size_t index) {
        if (index >= str.length()) {
            static char null_char = '\0';
            return null_char;
        }
        return str[index];
    }

    void setCharAt(size_t index, char c) {
        if (index < str.length()) str[index] = c;
    }

    // Comparison
    bool equals(const XenoString& s) const { return str == s.str; }
    bool equals(const char* s) const { return str == (s ? s : ""); }
    bool equalsIgnoreCase(const XenoString& s) const {
        if (str.length() != s.str.length()) return false;
        for (size_t i = 0; i < str.length(); ++i) {
            if (std::tolower(static_cast<unsigned char>(str[i])) !=
                std::tolower(static_cast<unsigned char>(s.str[i]))) return false;
        }
        return true;
    }

    // Comparison operators
    bool operator==(const XenoString& other) const { return str == other.str; }
    bool operator!=(const XenoString& other) const { return str != other.str; }
    bool operator==(const char* other) const { return str == (other ? other : ""); }
    bool operator!=(const char* other) const { return str != (other ? other : ""); }
    bool operator<(const XenoString& other) const { return str < other.str; }
    bool operator>(const XenoString& other) const { return str > other.str; }
    bool operator<=(const XenoString& other) const { return str <= other.str; }
    bool operator>=(const XenoString& other) const { return str >= other.str; }

    int compareTo(const XenoString& s) const {
        return str.compare(s.str);
    }

    int compareToIgnoreCase(const XenoString& s) const {
        std::string str1 = str, str2 = s.str;
        for (char& c : str1) c = std::tolower(static_cast<unsigned char>(c));
        for (char& c : str2) c = std::tolower(static_cast<unsigned char>(c));
        return str1.compare(str2);
    }

    // String operations
    bool startsWith(const XenoString& prefix) const {
        if (prefix.length() > str.length()) return false;
        return str.compare(0, prefix.length(), prefix.str) == 0;
    }

    bool endsWith(const XenoString& suffix) const {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix.str) == 0;
    }

    bool startsWith(const char* prefix) const {
        if (!prefix) return false;
        size_t prefix_len = strlen(prefix);
        if (prefix_len > str.length()) return false;
        return str.compare(0, prefix_len, prefix) == 0;
    }

    bool endsWith(const char* suffix) const {
        if (!suffix) return false;
        size_t suffix_len = strlen(suffix);
        if (suffix_len > str.length()) return false;
        return str.compare(str.length() - suffix_len, suffix_len, suffix) == 0;
    }

    XenoString& toLowerCase() {
        for (char& c : str) c = std::tolower(static_cast<unsigned char>(c));
        return *this;
    }

    XenoString& toUpperCase() {
        for (char& c : str) c = std::toupper(static_cast<unsigned char>(c));
        return *this;
    }

    // Search
    int indexOf(char c, size_t fromIndex = 0) const {
        if (fromIndex >= str.length()) return -1;
        size_t pos = str.find(c, fromIndex);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }

    int indexOf(const XenoString& s, size_t fromIndex = 0) const {
        if (fromIndex >= str.length()) return -1;
        size_t pos = str.find(s.str, fromIndex);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }

    int lastIndexOf(char c, size_t fromIndex = std::string::npos) const {
        size_t pos = str.rfind(c, fromIndex);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }

    int lastIndexOf(const XenoString& s, size_t fromIndex = std::string::npos) const {
        size_t pos = str.rfind(s.str, fromIndex);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }

    bool contains(char c) const {
        return str.find(c) != std::string::npos;
    }

    bool contains(const XenoString& s) const {
        return str.find(s.str) != std::string::npos;
    }

    // Substring
    XenoString substring(size_t beginIndex, size_t endIndex = std::string::npos) const {
        if (beginIndex > str.length()) return XenoString("");
        if (endIndex == std::string::npos) endIndex = str.length();
        if (endIndex < beginIndex) return XenoString("");
        return XenoString(str.substr(beginIndex, endIndex - beginIndex));
    }

    // Modification
    XenoString& trim() {
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) {
            str.clear();
            return *this;
        }
        size_t end = str.find_last_not_of(" \t\n\r\f\v");
        str = str.substr(start, end - start + 1);
        return *this;
    }

    XenoString& trimLeft() {
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) {
            str.clear();
        }
        else {
            str = str.substr(start);
        }
        return *this;
    }

    XenoString& trimRight() {
        size_t end = str.find_last_not_of(" \t\n\r\f\v");
        if (end == std::string::npos) {
            str.clear();
        }
        else {
            str = str.substr(0, end + 1);
        }
        return *this;
    }

    XenoString& replace(char findChar, char replaceChar) {
        for (char& c : str) {
            if (c == findChar) c = replaceChar;
        }
        return *this;
    }

    XenoString& replace(const XenoString& findStr, const XenoString& replaceStr) {
        size_t pos = 0;
        while ((pos = str.find(findStr.str, pos)) != std::string::npos) {
            str.replace(pos, findStr.length(), replaceStr.str);
            pos += replaceStr.length();
        }
        return *this;
    }

    XenoString& remove(size_t index, size_t count = std::string::npos) {
        if (index >= str.length()) return *this;
        if (count == std::string::npos || index + count > str.length()) {
            count = str.length() - index;
        }
        str.erase(index, count);
        return *this;
    }

    XenoString& insert(size_t index, const XenoString& s) {
        if (index <= str.length()) {
            str.insert(index, s.str);
        }
        return *this;
    }

    // Conversion
    int toInt() const {
        try {
            return std::stoi(str);
        }
        catch (...) {
            return 0;
        }
    }

    long toLong() const {
        try {
            return std::stol(str);
        }
        catch (...) {
            return 0;
        }
    }

    float toFloat() const {
        try {
            return std::stof(str);
        }
        catch (...) {
            return 0.0f;
        }
    }

    double toDouble() const {
        try {
            return std::stod(str);
        }
        catch (...) {
            return 0.0;
        }
    }

    bool toBoolean() const {
        if (equalsIgnoreCase("true") || equals("1")) return true;
        if (equalsIgnoreCase("false") || equals("0")) return false;
        return toInt() != 0;
    }

    // Concatenation operators
    XenoString& operator+=(const XenoString& s) { str += s.str; return *this; }
    XenoString& operator+=(const char* s) { str += (s ? s : ""); return *this; }
    XenoString& operator+=(char c) { str += c; return *this; }

    XenoString& concat(const XenoString& s) { return *this += s; }
    XenoString& concat(const char* s) { return *this += s; }
    XenoString& concat(char c) { return *this += c; }
    XenoString& concat(int num) { return *this += XenoString(num); }
    XenoString& concat(unsigned int num) { return *this += XenoString(num); }
    XenoString& concat(long num) { return *this += XenoString(num); }
    XenoString& concat(float num) { return *this += XenoString(num); }
    XenoString& concat(double num) { return *this += XenoString(num); }

    // Utility functions
    XenoString toLower() const {
        return XenoString(*this).toLowerCase();
    }

    XenoString toUpper() const {
        return XenoString(*this).toUpperCase();
    }

    std::vector<XenoString> split(char delimiter) const {
        std::vector<XenoString> result;
        size_t start = 0;
        size_t end = str.find(delimiter);

        while (end != std::string::npos) {
            result.push_back(substring(start, end));
            start = end + 1;
            end = str.find(delimiter, start);
        }
        result.push_back(substring(start));
        return result;
    }

    std::vector<XenoString> split(const XenoString& delimiter) const {
        std::vector<XenoString> result;
        size_t start = 0;
        size_t end = str.find(delimiter.str);

        while (end != std::string::npos) {
            result.push_back(substring(start, end));
            start = end + delimiter.length();
            end = str.find(delimiter.str, start);
        }
        result.push_back(substring(start));
        return result;
    }

    bool isDigit() const {
        if (str.empty()) return false;
        for (char c : str) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }

    bool isAlpha() const {
        if (str.empty()) return false;
        for (char c : str) {
            if (!std::isalpha(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }

    bool isAlphaNumeric() const {
        if (str.empty()) return false;
        for (char c : str) {
            if (!std::isalnum(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }

    // Friend operators for concatenation
    friend XenoString operator+(const XenoString& lhs, const XenoString& rhs) {
        return XenoString(lhs.str + rhs.str);
    }

    friend XenoString operator+(const XenoString& lhs, const char* rhs) {
        return XenoString(lhs.str + (rhs ? rhs : ""));
    }

    friend XenoString operator+(const char* lhs, const XenoString& rhs) {
        return XenoString((lhs ? lhs : "") + rhs.str);
    }

    friend XenoString operator+(const XenoString& lhs, char rhs) {
        return XenoString(lhs.str + rhs);
    }

    friend XenoString operator+(char lhs, const XenoString& rhs) {
        return XenoString(std::string(1, lhs) + rhs.str);
    }

    // Stream operators
    friend std::ostream& operator<<(std::ostream& os, const XenoString& s) {
        return os << s.str;
    }

    friend std::istream& operator>>(std::istream& is, XenoString& s) {
        std::string temp;
        is >> temp;
        s.str = temp;
        return is;
    }

    // Get internal string reference
    const std::string& getStdString() const { return str; }
};

// Hash support for std::unordered_map etc.
namespace std {
    template<>
    struct hash<XenoString> {
        size_t operator()(const XenoString& s) const {
            return hash<std::string>()(s.getStdString());
        }
    };
}
// Simple Serial emulation
class SerialClass {
public:
    void begin(unsigned long) {}
    void end() {}

    int available() {
        std::lock_guard<std::mutex> lk(g_serialMutex);
        if (g_serialQueue.empty()) return 0;
        return static_cast<int>(g_serialQueue.front().size());
    }

    XenoString readString() {
        std::lock_guard<std::mutex> lk(g_serialMutex);
        if (g_serialQueue.empty()) return XenoString("");
        std::string s = std::move(g_serialQueue.front());
        g_serialQueue.pop_front();
        return XenoString(s);
    }

    // Blocking read with timeout - if you added it earlier, оставьте её
    XenoString readStringTimeout(unsigned long timeout_ms) {
        std::unique_lock<std::mutex> lk(g_serialMutex);
        if (g_serialQueue.empty()) {
            if (timeout_ms == 0) {
                g_serialCv.wait(lk, [] { return !g_serialQueue.empty(); });
            } else {
                auto dur = std::chrono::milliseconds(timeout_ms);
                if (!g_serialCv.wait_for(lk, dur, [] { return !g_serialQueue.empty(); })) {
                    return XenoString("");
                }
            }
        }
        std::string s = std::move(g_serialQueue.front());
        g_serialQueue.pop_front();
        return XenoString(s);
    }

    // Универсальные методы через шаблоны
    template<typename T>
    size_t print(const T& value) {
        std::stringstream ss;
        ss << value;
        std::string output = ss.str();

        // Use global callback if set, otherwise fall back to cout.
        if (g_outputCallback) {
            g_outputCallback(output);
        } else {
            std::cout << output;
            std::cout.flush(); // <- важное: сразу сбрасываем буфер stdout
        }
        return output.length();
    }

    template<typename T>
    size_t println(const T& value) {
        size_t result = print(value);
        // ensuring newline and flush
        if (g_outputCallback) {
            g_outputCallback("\n");
        } else {
            std::cout << std::endl; // std::endl вставит '\n' и flush
        }
        return result + 1;
    }

    // println() без аргументов
    size_t println() {
        if (g_outputCallback) {
            g_outputCallback("\n");
        } else {
            std::cout << std::endl;
        }
        return 1;
    }
    // Явные специализации для устранения неоднозначности
    size_t print(size_t n) {
        return print(std::to_string(n));
    }

    size_t println(size_t n) {
        return println(std::to_string(n));
    }

    // Специализированные методы для float/double с precision
    size_t print(double n, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << n;
        return print(ss.str());
    }

    size_t println(double n, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << n;
        return println(ss.str());
    }
};

// Объявляем Serial как extern, а определение будет в одном .cpp файле
extern SerialClass Serial;

// Inline функции
inline void pinMode(uint8_t pin, uint8_t mode) {
    // Implementation for Windows
}

inline void digitalWrite(uint8_t pin, uint8_t val) {
    // Implementation for Windows  
}

inline int digitalRead(uint8_t pin) {
    return 0;
}

inline int analogRead(uint8_t pin) {
    return 0;
}

inline void analogReference(uint8_t mode) {
}

inline void analogWrite(uint8_t pin, int val) {
}

inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void delayMicroseconds(unsigned int us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

inline unsigned long millis() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

inline unsigned long micros() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

inline unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout) {
    return 0;
}

inline unsigned long pulseInLong(uint8_t pin, uint8_t state, unsigned long timeout) {
    return 0;
}

inline void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
}

inline uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) {
    return 0;
}

inline void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode) {
}

inline void detachInterrupt(uint8_t interruptNum) {
}

inline bool isInteger(const XenoString& str) {
    if (str.isEmpty()) return false;

    // Убираем все пробелы
    XenoString trimmed = str;
    trimmed.trim();
    if (trimmed.isEmpty()) return false;

    const char* cstr = trimmed.c_str();
    size_t len = trimmed.length();
    size_t start = 0;

    // Разрешаем знак в начале
    if (cstr[0] == '-' || cstr[0] == '+') {
        start = 1;
        if (len == 1) return false; // Только знак без цифр
    }

    // Проверяем что все символы - цифры
    for (size_t i = start; i < len; ++i) {
        if (cstr[i] < '0' || cstr[i] > '9') {
            return false;
        }
    }

    // Простая проверка - если дошли сюда, это целое число
    return true;
}


// Utility functions
template<typename T>
inline T min(T a, T b) {
    return (a < b) ? a : b;
}

template<typename T>
inline T max(T a, T b) {
    return (a > b) ? a : b;
}
#endif