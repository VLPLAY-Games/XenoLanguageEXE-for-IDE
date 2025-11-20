// xeno_host.cpp
// cmake .. -G "Visual Studio 17 2022" -A x64
// cmake --build . --config Release


#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <exception>
#include <cstring>
#include <fstream>
#include <filesystem>
#include "XenoLanguage.h"

namespace fs = std::filesystem;
static uint32_t g_max_instructions = 100000;

static const char* bridge_version = "v0.1.3.1";
static const char* bridge_date = "14.11.2025";

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

    std::thread vmThread;     // поток для VM
    std::mutex vmThreadMutex; // защита для vmThread


    std::ofstream infoFile("xeno/xeno_info.txt", std::ios::trunc);
    if (infoFile.is_open()) {
        infoFile << "Language: " << (engine.getLanguageName() ? engine.getLanguageName() : "Unknown") << "\n";
        infoFile << "LanguageVersion: " << (engine.getLanguageVersion() ? engine.getLanguageVersion() : "Unknown") << "\n";
        infoFile << "LanguageDate: " << (engine.getLanguageDate() ? engine.getLanguageDate() : "Unknown") << "\n";

        infoFile << "BridgeVersion: " << bridge_version << "\n";
        infoFile << "BridgeDate: " << bridge_date << "\n\n";

        infoFile << "[API_SETTINGS]\n";
        infoFile << "SUPPORT_PRINT_COMPILED_CODE\n";
        infoFile << "SUPPORT_DISASSEMBLE\n";
        infoFile << "SUPPORT_DUMP_STATE\n";
        infoFile << "SUPPORT_SET_MAX_INSTRUCTIONS\n";

        infoFile.close();
        
        try {
            fs::permissions("xeno/xeno_info.txt",
                            fs::perms::owner_read |
                            fs::perms::group_read |
                            fs::perms::others_read,
                            fs::perm_options::replace);
        } catch (const std::exception& e) {
            std::cerr << "Failed to set read-only permissions: " << e.what() << std::endl;
        }
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
                if (vm_running.load()) {
                    send_line("VM already running");
                } else {
                    vm_running = true;

                    {
                        std::lock_guard<std::mutex> lk(vmThreadMutex);
                        if (vmThread.joinable()) {
                            try { vmThread.join(); } catch(...) {}
                        }
                        // УБРАЛИ &g_max_instructions из списка захвата.
                        vmThread = std::thread([&engine, &vm_running, &send_line]() {
                            try {
                                // глоабльная g_max_instructions доступна здесь без захвата
                                engine.setMaxInstructions(g_max_instructions);
                                bool ok = engine.run();
                                if (!ok) {
                                    send_line("Failed to start virtual machine");
                                }
                            } catch (const std::exception& ex) {
                                send_line(std::string("Runtime error: ") + ex.what());
                            } catch (...) {
                                send_line("Unknown runtime error occurred in VM thread");
                            }
                            vm_running = false;
                            send_line("VM thread finished");
                        });
                    }
                    send_line("VM started (background)");
                }
            } catch (const std::exception& ex) {
                send_line(std::string("Runtime error starting VM: ") + ex.what());
            } catch (...) {
                send_line("Unknown runtime error occurred starting VM");
            }
        }

        else if (cmd == "STOP") {
            try {
                // Остановим VM; engine.stop() должен корректно разбудить/прервать engine.run()
                engine.stop();
                vm_running = false;

                // Подождём завершения фонового потока
                {
                    std::lock_guard<std::mutex> lk(vmThreadMutex);
                    if (vmThread.joinable()) {
                        try { vmThread.join(); } catch(...) {}
                    }
                }
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
        else if (cmd == "GET_BRIDGE_VERSION") {
            try {
                send_line(std::string("Language version: ") + bridge_version);
            } catch (...) {
                send_line("Could not retrieve language version");
            }
        }
        else if (cmd == "GET_BRIDGE_DATE") {
            try {
                send_line(std::string("Language date: ") + bridge_date);
            } catch (...) {
                send_line("Could not retrieve language date");
            }
        }

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
        else if (cmd == "EXIT") {
            send_line("Exiting");
            // остановим VM, если запущена
            try {
                engine.stop();
            } catch(...) {}
            // дождёмся потока
            {
                std::lock_guard<std::mutex> lk(vmThreadMutex);
                if (vmThread.joinable()) {
                    try { vmThread.join(); } catch(...) {}
                }
            }
            running = false;
            break;
        }
        else if (cmd.rfind("STDIN ", 0) == 0) { // starts_with "STDIN "
            std::string payload = cmd.substr(6); // всё после "STDIN "
            SerialPushInput(payload); // кладём в очередь Serial (см arduino_compat)
            send_line("STDIN_OK");
        }
        else {
            send_line(std::string("Unknown command: ") + cmd);
        }

    }

    try { engine.stop(); } catch(...) {}
    {
        std::lock_guard<std::mutex> lk(vmThreadMutex);
        if (vmThread.joinable()) {
            try { vmThread.join(); } catch(...) {}
        }
    }
    return 0;
}