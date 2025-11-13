// xeno_host.cpp
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <exception>
#include <atomic>
#include <cstring>
#include <fstream>

#include "XenoLanguage.h"

// Глобальные настройки
static uint32_t g_max_instructions = 10000;
static bool g_print_disassembly = false;
static bool g_print_dump_state = false;
static bool g_print_compiled_code = false;

static bool read_exact(std::istream& in, std::string& out, size_t n) {
    out.clear();
    out.resize(n);
    size_t read = 0;
    while (read < n) {
        in.read(&out[read], n - read);
        std::streamsize got = in.gcount();
        if (got <= 0) return false;
        read += static_cast<size_t>(got);
    }
    return true;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    XenoLanguage engine;
    std::mutex ioMutex;
    std::atomic<bool> running{true};
    std::atomic<bool> vm_running{false};

    std::ofstream infoFile("xeno_info.txt");
    if (infoFile.is_open()) {
        infoFile << "Language: " << (engine.getLanguageName() ? engine.getLanguageName() : "Unknown") << "\n";
        infoFile << "LanguageVersion: " << (engine.getLanguageVersion() ? engine.getLanguageVersion() : "Unknown") << "\n";
        infoFile << "LanguageDate: " << (engine.getLanguageDate() ? engine.getLanguageDate() : "Unknown") << "\n";
        infoFile << "VMVersion: " << (engine.getVMVersion() ? engine.getVMVersion() : "Unknown") << "\n";
        infoFile << "VMDate: " << (engine.getVMDate() ? engine.getVMDate() : "Unknown") << "\n";
        infoFile << "CompilerVersion: " << (engine.getCompilerVersion() ? engine.getCompilerVersion() : "Unknown") << "\n";
        infoFile << "CompilerDate: " << (engine.getCompilerDate() ? engine.getCompilerDate() : "Unknown") << "\n";
        infoFile.close();
    }

    auto send_line = [](const std::string& s) {
        std::cout << s << std::endl;
        std::cout.flush();
    };

    while (running.load()) {
        std::string cmd;
        if (!std::getline(std::cin, cmd)) {
            break;
        }

        if (cmd == "COMPILE") {
            std::string lenLine;
            if (!std::getline(std::cin, lenLine)) {
                send_line("Missing source code length");
                continue;
            }
            size_t N = 0;
            try {
                N = std::stoull(lenLine);
            } catch (...) {
                send_line("Invalid length format");
                continue;
            }

            std::string src;
            if (!read_exact(std::cin, src, N)) {
                send_line("Could not read source code");
                continue;
            }
            if (std::cin.peek() == '\n') std::cin.get();

            try {
                // Применяем текущие настройки перед компиляцией
                engine.setMaxInstructions(g_max_instructions);
                
                bool ok = engine.compile(src);
                if (ok) {
                    send_line("Compilation successful!");
                } else {
                    send_line("Compilation failed - check your code for errors");
                }
            } catch (const std::exception& ex) {
                send_line(std::string("Compilation error: ") + ex.what());
            } catch (...) {
                send_line("Unknown compilation error occurred");
            }
        }
        else if (cmd == "RUN") {
            try {
                // Применяем текущие настройки перед запуском
                engine.setMaxInstructions(g_max_instructions);
                
                bool ok = engine.run();
                if (ok) {
                    vm_running = true;
                    send_line("");
                } else {
                    send_line("Failed to start virtual machine");
                }
            } catch (const std::exception& ex) {
                send_line(std::string("Runtime error: ") + ex.what());
            } catch (...) {
                send_line("Unknown runtime error occurred");
            }
        }
        else if (cmd == "STOP") {
            try {
                engine.stop();
                vm_running = false;
                send_line("Virtual machine stopped");
            } catch (...) {
                send_line("Error while stopping virtual machine");
            }
        }
        else if (cmd == "GET_LANGUAGE_NAME") {
            try {
                const char* name = engine.getLanguageName();
                send_line(std::string("Language: ") + (name ? name : "Unknown"));
            } catch (...) {
                send_line("Could not retrieve language name");
            }
        }
        else if (cmd == "GET_LANGUAGE_VERSION") {
            try {
                const char* ver = engine.getLanguageVersion();
                send_line(std::string("Language version: ") + (ver ? ver : "Unknown"));
            } catch (...) {
                send_line("Could not retrieve language version");
            }
        }
        else if (cmd == "GET_LANGUAGE_DATE") {
            try {
                const char* date = engine.getLanguageDate();
                send_line(std::string("Language date: ") + (date ? date : "Unknown"));
            } catch (...) {
                send_line("Could not retrieve language date");
            }
        }
        else if (cmd == "GET_VM_VERSION") {
            try {
                const char* vm_ver = engine.getVMVersion();
                send_line(std::string("VM version: ") + (vm_ver ? vm_ver : "Unknown"));
            } catch (...) {
                send_line("Could not retrieve VM version");
            }
        }
        else if (cmd == "GET_VM_DATE") {
            try {
                const char* vm_date = engine.getVMDate();
                send_line(std::string("VM date: ") + (vm_date ? vm_date : "Unknown"));
            } catch (...) {
                send_line("Could not retrieve VM date");
            }
        }
        else if (cmd == "GET_COMPILER_VERSION") {
            try {
                const char* comp_ver = engine.getCompilerVersion();
                send_line(std::string("Compiler version: ") + (comp_ver ? comp_ver : "Unknown"));
            } catch (...) {
                send_line("Could not retrieve compiler version");
            }
        }
        else if (cmd == "GET_COMPILER_DATE") {
            try {
                const char* comp_date = engine.getCompilerDate();
                send_line(std::string("Compiler date: ") + (comp_date ? comp_date : "Unknown"));
            } catch (...) {
                send_line("Could not retrieve compiler date");
            }
        }
        else if (cmd == "GET_VERSION") {
            try {
                const char* name = engine.getLanguageName();
                const char* ver = engine.getLanguageVersion();
                const char* date = engine.getLanguageDate();
                const char* vm_ver = engine.getVMVersion();
                const char* vm_date = engine.getVMDate();
                const char* comp_ver = engine.getCompilerVersion();
                const char* comp_date = engine.getCompilerDate();
                std::string out = "Version Information:\n" +
                    std::string("  Language: ") + (name ? name : "Unknown") + " "+ (ver ? ver : "Unknown") + " (" + (date ? date : "Unknown") + ")\n" +
                    "  Virtual Machine: " + (vm_ver ? vm_ver : "Unknown") + " (" + (vm_date ? vm_date : "Unknown") + ")\n" +
                    "  Compiler: " + (comp_ver ? comp_ver : "Unknown") + " (" + (comp_date ? comp_date : "Unknown") + ")";
                send_line(out);
            } catch (...) {
                send_line("Could not retrieve version information");
            }
        }
        // Новые команды для вызова методов XenoLanguage напрямую
        else if (cmd == "PRINT_COMPILED_CODE") {
            try {
                engine.printCompiledCode();
                send_line("Compiled code printed");
            } catch (const std::exception& ex) {
                send_line(std::string("Error printing compiled code: ") + ex.what());
            } catch (...) {
                send_line("Unknown error while printing compiled code");
            }
        }
        else if (cmd == "DISASSEMBLE") {
            try {
                engine.disassemble();
                send_line("Disassembly completed");
            } catch (const std::exception& ex) {
                send_line(std::string("Error during disassembly: ") + ex.what());
            } catch (...) {
                send_line("Unknown error during disassembly");
            }
        }
        else if (cmd == "DUMP_STATE") {
            try {
                engine.dumpState();
                send_line("VM state dumped");
            } catch (const std::exception& ex) {
                send_line(std::string("Error dumping VM state: ") + ex.what());
            } catch (...) {
                send_line("Unknown error while dumping VM state");
            }
        }
        else if (cmd == "STEP") {
            try {
                engine.step();
                send_line("Executed one step");
            } catch (const std::exception& ex) {
                send_line(std::string("Error executing step: ") + ex.what());
            } catch (...) {
                send_line("Unknown error while executing step");
            }
        }
        else if (cmd == "IS_RUNNING") {
            try {
                bool is_running = engine.isRunning();
                send_line(is_running ? "VM is running" : "VM is not running");
            } catch (const std::exception& ex) {
                send_line(std::string("Error checking VM status: ") + ex.what());
            } catch (...) {
                send_line("Unknown error while checking VM status");
            }
        }
        // Команды для настроек
        else if (cmd == "SET_MAX_INSTRUCTIONS") {
            std::string value;
            if (std::getline(std::cin, value)) {
                try {
                    g_max_instructions = std::stoul(value);
                    engine.setMaxInstructions(g_max_instructions);
                    send_line("Max instructions set to " + value);
                } catch (...) {
                    send_line("Invalid value for max instructions");
                }
            } else {
                send_line("Missing value for max instructions");
            }
        }
        else if (cmd == "GET_SETTINGS") {
            std::string settings = "⚙️  Current Settings:\n" +
                std::string("  Max Instructions: ") + std::to_string(g_max_instructions) + "\n" +
                "  Print Disassembly: " + (g_print_disassembly ? "Yes" : "No") + "\n" +
                "  Print VM State: " + (g_print_dump_state ? "Yes" : "No") + "\n" +
                "  Print Compiled Code: " + (g_print_compiled_code ? "Yes" : "No");
            send_line(settings);
        }
        else if (cmd == "EXIT") {
            send_line("Exiting");
            running = false;
            break;
        }
        else {
            send_line(std::string("Unknown command: ") + cmd);
        }
    }

    try {
        engine.stop();
    } catch(...) {}
    return 0;
}