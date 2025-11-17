import os
import re
import sys

def process_all_files(root_dir):
    """Process all .h and .cpp files in the directory tree."""
    processed_count = 0
    error_count = 0
    
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith(('.h', '.cpp')) and file != 'arduino_compat.h':
                filepath = os.path.join(root, file)
                try:
                    if process_file(filepath):
                        processed_count += 1
                    else:
                        error_count += 1
                except Exception as e:
                    print(f"ERROR processing {filepath}: {str(e)}")
                    error_count += 1
    
    print(f"\nProcessing completed:")
    print(f"  Successfully processed: {processed_count} files")
    print(f"  Errors: {error_count} files")
    
    return error_count == 0

def process_file(filepath):
    """Process a single file - convert Arduino includes to arduino_compat.h and handle String defines."""
    # Skip these files completely
    if filepath.endswith(('arduino_compat.cpp', 'xeno_host.cpp')):
        print(f"SKIPPED: {filepath}")
        return True
        
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        
        # Remove Arduino.h includes
        content = re.sub(r'#include\s*<Arduino\.h>\s*\n', '', content)
        
        # Remove paths from includes, keep only filename
        content = re.sub(
            r'#include\s*"([^"/]*/)*([^"]+)"',
            r'#include "\2"',
            content
        )
        
        # Comment out isInteger functions in specific files
        if filepath.endswith(('xeno_compiler.h', 'xeno_vm.h')):
            content = comment_isInteger_declaration(content)
        elif filepath.endswith(('xeno_compiler.cpp', 'xeno_vm.cpp')):
            content = comment_isInteger_definition(content)
        
        if filepath.endswith('.h'):
            content = process_header_file(content)
        elif filepath.endswith('.cpp'):
            content = process_cpp_file(content)
        
        # Only write if content changed
        if content != original_content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"UPDATED: {filepath}")
            return True
        else:
            print(f"NO CHANGES: {filepath}")
            return True
            
    except Exception as e:
        print(f"ERROR: Failed to process {filepath} - {str(e)}")
        return False

def comment_isInteger_declaration(content):
    """Comment out isInteger function declaration in header files."""
    # Pattern to find bool isInteger(const String& str);
    pattern = r'bool\s+isInteger\s*\(\s*const\s+String\s*&\s*str\s*\)\s*;'
    
    # Replace with commented version
    replacement = r'// bool isInteger(const String& str);'
    content = re.sub(pattern, replacement, content)
    
    return content

def comment_isInteger_definition(content):
    """Comment out isInteger function definition in cpp files."""
    # Improved patterns to capture the complete function definitions
    vm_pattern = r'bool\s+XenoVM::isInteger\s*\(\s*const\s+String\s*&\s*str\s*\)\s*\{[^}]+\}'
    compiler_pattern = r'bool\s+XenoCompiler::isInteger\s*\(\s*const\s+String\s*&\s*str\s*\)\s*\{[^}]+\}[^}]*\}'
    
    # More specific patterns that capture everything between the braces
    vm_complete_pattern = r'(bool\s+XenoVM::isInteger\s*\(\s*const\s+String\s*&\s*str\s*\)\s*\{)(.*?)(\n\s*\})'
    compiler_complete_pattern = r'(bool\s+XenoCompiler::isInteger\s*\(\s*const\s+String\s*&\s*str\s*\)\s*\{)(.*?)(\n\s*\})'
    
    # Try the complete patterns first
    vm_replacement = '''// bool XenoVM::isInteger(const String& str) {
//     if (str.isEmpty()) return false;
//     const char* cstr = str.c_str();
//     size_t start = 0;
//     if (cstr[0] == '-') start = 1;
//     for (size_t i = start; i < str.length(); ++i) {
//         if (!isdigit(cstr[i])) return false;
//     }
//     return true;
// }'''
    
    compiler_replacement = '''// bool XenoCompiler::isInteger(const String& str) {
//     if (str.isEmpty()) return false;
//     const char* cstr = str.c_str();
//     size_t start = 0;
//     if (cstr[0] == '-') start = 1;
//     for (size_t i = start; i < str.length(); ++i) {
//         if (!isdigit(cstr[i])) return false;
//     }
//     long long_val = str.toInt();
//     return !(long_val > 2147483647L || long_val < -2147483648L);
// }'''
    
    # Apply replacements using the complete patterns
    content = re.sub(compiler_complete_pattern, compiler_replacement, content, flags=re.DOTALL)
    content = re.sub(vm_complete_pattern, vm_replacement, content, flags=re.DOTALL)
    
    return content

def process_header_file(content):
    """Process .h file - add arduino_compat.h include and String defines."""
    # Add arduino_compat.h and String define after all includes within guard macros
    if '#include "arduino_compat.h"' not in content:
        lines = content.splitlines()
        in_guard = False
        last_include_line = -1
        
        for i, line in enumerate(lines):
            if line.strip().startswith('#ifndef') or line.strip().startswith('#define'):
                in_guard = True
            elif in_guard and line.strip().startswith('#include'):
                last_include_line = i
            elif in_guard and line.strip() and not line.strip().startswith('#'):
                break
        
        if last_include_line != -1:
            lines.insert(last_include_line + 1, '#include "arduino_compat.h"')
            lines.insert(last_include_line + 2, '#define String XenoString')
            lines.insert(last_include_line + 3, '')
        else:
            ifndef_pattern = r'(#ifndef\s+[A-Za-z0-9_]+\s*\n#define\s+[A-Za-z0-9_]+\s*\n)'
            replacement = r'\1#include "arduino_compat.h"\n#define String XenoString\n\n'
            content = re.sub(ifndef_pattern, replacement, content)
            return content
        
        content = '\n'.join(lines)
    
    # Add #undef String before last #endif
    if '#undef String' not in content:
        lines = content.splitlines()
        for i in range(len(lines)-1, -1, -1):
            if lines[i].strip().startswith('#endif'):
                lines.insert(i, '#undef String')
                content = '\n'.join(lines)
                break
    
    return content

def process_cpp_file(content):
    """Process .cpp file - add String defines after includes."""
    # Add String define after all includes
    if '#define String XenoString' not in content:
        includes_pattern = r'((?:#include[^\n]+\n)+)'
        match = re.search(includes_pattern, content)
        
        if match:
            end_pos = match.end()
            content = content[:end_pos] + '#define String XenoString\n' + content[end_pos:]
        else:
            license_end = content.find('*/')
            if license_end != -1:
                insert_pos = content.find('\n', license_end) + 1
                if insert_pos > 0:
                    content = content[:insert_pos] + '#define String XenoString\n' + content[insert_pos:]
    
    # Add #undef String at end of file
    if '#undef String' not in content:
        content = content.rstrip() + '\n#undef String\n'
    
    return content

if __name__ == "__main__":
    if len(sys.argv) > 1:
        project_path = sys.argv[1]
    else:
        project_path = "."
    
    print("Starting Arduino to arduino_compat.h conversion...")
    print(f"Processing directory: {os.path.abspath(project_path)}")
    
    success = process_all_files(project_path)
    
    if success:
        print("\nAll files processed successfully!")
        sys.exit(0)
    else:
        print("\nSome files had errors during processing!")
        sys.exit(1)