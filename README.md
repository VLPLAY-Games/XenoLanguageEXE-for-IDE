# Xeno Language EXE for Windows IDE

## 🏗️ Project Overview

This repository provides the Windows executable version of the Xeno programming language, specifically designed for use with the **Xeno IDE**. It creates a compatibility bridge that allows Arduino-based Xeno code to run natively on Windows systems.

## 🌉 Bridge Architecture

### Three-Layer Bridge System

#### 1. **Arduino-Windows Compatibility Layer** (`arduino_compat.h/cpp`)
- **Purpose**: Emulates Arduino API on Windows
- **Components**:
  - `XenoString` class replacing Arduino `String`
  - Serial emulation with input/output queues
  - GPIO function stubs (`pinMode`, `digitalWrite`, etc.)
  - Timing functions (`delay`, `millis`)

#### 2. **Language Source Adaptation** (`auto_xeno_bridge.py`)
- **Purpose**: Automatically converts ESP32 Xeno code to Windows-compatible code
- **Features**:
  - Replaces `Arduino.h` includes with `arduino_compat.h`
  - Converts `String` to `XenoString` using macro definitions
  - Handles function conflicts (e.g., `isInteger`)
  - Processes entire directory trees

#### 3. **Windows-IDE Communication Layer** (`xeno_host.cpp`)
- **Purpose**: Provides the executable interface for Xeno IDE
- **Capabilities**:
  - Standard input/output protocol with IDE
  - Background VM execution
  - Compilation and runtime management
  - Version information reporting

## 📁 File Structure
```
XenoLanguageEXE-for-IDE/
├── arduino_compat.h # Arduino API emulation header
├── arduino_compat.cpp # Arduino API implementation
├── auto_xeno_bridge.py # Automatic code conversion script
├── xeno_host.cpp # Main executable source
├── CMakeLists.txt # Build configuration
├── version.rc # Version resource file
└── Xeno Language Main Files
```


## 🔧 Building from Source

### Prerequisites
- CMake 3.10+
- Visual Studio 2017+ or MinGW-w64
- C++17 compatible compiler

### Build Steps
```bash
Configure project:
cmake -B build -G "Visual Studio 17 2022" -A x64

Build release version:
cmake --build build --config Release

Output: build/Release/xeno_host.exe
```
## 🔄 Integration with Xeno IDE

Xeno IDE automatically downloads and uses `xeno_host.exe` from this repository's releases. The integration process:

1. **IDE Detection**: IDE checks `xeno/xeno_info.txt` for version compatibility
2. **Communication Protocol**: IDE communicates via standard input/output
3. **Execution Flow**:
   - IDE sends `COMPILE` command with source code
   - `xeno_host.exe` compiles using Xeno language core
   - IDE sends `RUN` command to execute in background thread
   - Real-time I/O through Serial emulation

## 🎯 Main Features

### Cross-Platform Compatibility
- **Arduino API Emulation**: Full Serial, String, and basic GPIO support
- **ESP32 to Windows**: Seamless code migration between platforms
- **Real-time Execution**: Background VM with thread management

### IDE Integration
- **Standard Protocol**: Simple line-based communication
- **Version Management**: Automatic update notifications
- **Error Handling**: Comprehensive exception reporting
- **State Management**: Start/stop/step execution control

### Performance & Security
- **Instruction Limits**: Configurable maximum execution steps
- **Memory Safety**: Bounded string operations and stack limits
- **Process Isolation**: Separate VM process from IDE

## 🔗 Related Projects

- **Main Xeno Language** (ESP32): [Xeno-Language Repository](https://github.com/VLPLAY-Games/Xeno-Language)
  - Core language implementation for ESP32 microcontrollers
  - Source basis for this Windows adaptation

- **Xeno IDE**: [XenoIDE Repository](https://github.com/VLPLAY-Games/XenoLanguageIDE)
  - Windows Integrated Development Environment
  - Primary consumer of this executable

## 📥 Installation for IDE Users

### Automatic Installation (Recommended)
1. Open Xeno IDE
2. Go to Settings → Xeno tab  
3. Click "Check Updates"
4. Select version and click "Install"

### Manual Installation
1. Download `xeno_host.exe` from Releases
2. Place in `xeno/` directory relative to IDE
3. Important: Run `xeno_host.exe` once to generate configuration
4. Restart IDE

## 🛠️ Development Usage

### Running the Conversion Script
Convert Xeno language source files:
```
python3 auto_xeno_bridge.py
```

### Source Files Included
The build includes all necessary Xeno language core files:
- xeno_common.cpp - Common utilities and data structures
- xeno_compiler.cpp - Code compilation and parsing
- xeno_vm.cpp - Virtual machine execution engine
- xeno_security.cpp - Security limits and validation
- XenoLanguage.cpp - Main API interface

## 📋 Supported Commands

| Command | Parameters | Description |
|---------|------------|-------------|
| COMPILE | Source code + length | Compile Xeno program |
| RUN | None | Execute in background |
| STOP | None | Stop execution |
| STEP | None | Execute single instruction |
| STDIN <data> | Input string | Send to Serial queue |
| SET_MAX_INSTRUCTIONS | Number | Change execution limit |

## 🔄 Version Compatibility

| Xeno Language | Bridge Version | IDE Version | IDE Support Level |
|---------------|----------------|-------------|---------------|
| 0.1.4+ | 0.1.4.1+ | 1.0.0+ | Full support |
| 0.1.3 | 0.1.3.1 | 1.0.0+ | Limited support with restrictions |
| < 0.1.3 | - | - | Not supported |

## 🐛 Troubleshooting

### Common Issues

**"xeno_host.exe not found"**
- Run manual installation steps
- Ensure file is in `xeno/` directory
- Check Windows Defender hasn't quarantined file

**"Installation error" after manual setup**
- Run `xeno_host.exe` directly once
- Verify `xeno_info.txt` is generated
- Check directory permissions

**Serial input not working**
- Ensure code waits for `INPUT` prompt
- Use `Serial.readString()` in Xeno code
- Check VM is running before sending input

**Build failures**
- Verify CMake version 3.10+
- Check Visual Studio installation
- Ensure Windows SDK is installed

## 📄 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

We welcome contributions to improve the Windows compatibility layer:

1. Report Arduino API coverage gaps
2. Improve Serial emulation performance  
3. Add Windows-specific optimizations
4. Enhance error handling and diagnostics

## 📞 Support

For issues with the Windows executable:
1. Check the [main language repository](https://github.com/VLPLAY-Games/Xeno-Language) for language-specific issues
2. Create an issue in this repository for Windows compatibility problems
3. Check IDE console for detailed error messages
4. Verify version compatibility between all components