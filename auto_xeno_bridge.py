import os
import re
import sys

def process_all_files(root_dir):
    """Process all .h and .cpp files under the project's src/ directory.

    This script requires that a `src/` directory exists inside the provided
    `root_dir`. If `src/` is missing the script will print an error and
    return False (exit code 1 when run as __main__).
    """
    src_root = os.path.join(root_dir, 'src')
    if not os.path.isdir(src_root):
        print(f"ERROR: Required directory not found: {os.path.abspath(src_root)}")
        print("Please make sure your project contains a 'src/' directory and run again.")
        return False

    walk_root = src_root
    print(f"Processing files under: {os.path.abspath(walk_root)}")

    processed_count = 0
    error_count = 0

    for root, dirs, files in os.walk(walk_root):
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
    """Process a single file - convert Arduino includes to arduino_compat.h and handle String defines.

    This version:
      - removes any `#include <Arduino.h>` lines,
      - does NOT strip directories from #include "..." (keeps xeno/...),
      - removes isInteger declarations and definitions completely,
      - replaces XenoVM::handleINPUT implementation if present,
      - adds/maintains arduino_compat.h and String define/undef as before.
    """
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

        # Remove isInteger declarations and definitions entirely
        content = remove_isInteger_declarations(content)
        content = remove_isInteger_definitions(content)

        # Replace VM handleINPUT implementation if processing xeno_vm.cpp
        if filepath.endswith('xeno_vm.cpp'):
            content = replace_handleINPUT(content)

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

def remove_isInteger_declarations(content):
    """Remove declaration lines like:
        bool isInteger(const String& str);
    that may appear inside class declarations or as standalone prototypes.
    """
    decl_pattern = re.compile(
        r'^\s*(?:static\s+|inline\s+|virtual\s+|)?'  # optional qualifiers
        r'bool\s+isInteger\s*\(\s*const\s+String\s*&\s*\w*\s*\)\s*;\s*$',
        flags=re.MULTILINE
    )
    new_content, n = decl_pattern.subn('', content)
    if n:
        # Collapse multiple blank lines to at most one empty line
        new_content = re.sub(r'\n{3,}', '\n\n', new_content)
    return new_content

def remove_isInteger_definitions(content):
    """Remove full function definitions for:
        bool XenoVM::isInteger(...){...}
        bool XenoCompiler::isInteger(...){...}

    We locate signature with a regex, then scan forward counting braces to find
    the true end of the function body (to handle nested braces).
    """
    patterns = [
        r'bool\s+XenoVM::isInteger\s*\(',
        r'bool\s+XenoCompiler::isInteger\s*\('
    ]

    for p in patterns:
        start_search_pos = 0
        while True:
            m = re.search(p, content[start_search_pos:])
            if not m:
                break
            abs_start = start_search_pos + m.start()
            # find the '(' and then the opening '{'
            paren_pos = content.find('(', abs_start)
            if paren_pos == -1:
                start_search_pos = abs_start + 1
                continue
            brace_pos = content.find('{', paren_pos)
            if brace_pos == -1:
                start_search_pos = abs_start + 1
                continue

            # scan for matching closing brace
            depth = 0
            i = brace_pos
            end_pos = None
            while i < len(content):
                c = content[i]
                if c == '{':
                    depth += 1
                elif c == '}':
                    depth -= 1
                    if depth == 0:
                        end_pos = i
                        break
                i += 1

            if end_pos is None:
                # unbalanced - stop attempting further on this pattern
                break

            # removal_start: try to remove full function signature line(s)
            line_start = content.rfind('\n', 0, abs_start)
            removal_start = line_start + 1 if line_start != -1 else abs_start
            removal_end = end_pos + 1  # include closing brace

            # Remove the function definition
            content = content[:removal_start] + content[removal_end:]

            # continue searching from removal_start (content has changed)
            start_search_pos = removal_start
    return content

def replace_handleINPUT(content):
    """Replace implementation of:
       void XenoVM::handleINPUT(const XenoInstruction& instr)
    with the provided new implementation.
    """
    pattern = r'void\s+XenoVM::handleINPUT\s*\(\s*const\s+XenoInstruction\s*&\s*instr\s*\)\s*\{'
    m = re.search(pattern, content)
    if not m:
        return content

    abs_start = m.start()
    brace_pos = content.find('{', m.end()-1)
    if brace_pos == -1:
        return content

    # find matching closing brace
    depth = 0
    i = brace_pos
    end_pos = None
    while i < len(content):
        if content[i] == '{':
            depth += 1
        elif content[i] == '}':
            depth -= 1
            if depth == 0:
                end_pos = i
                break
        i += 1

    if end_pos is None:
        return content

    # Include whole signature line(s)
    line_start = content.rfind('\n', 0, abs_start)
    removal_start = line_start + 1 if line_start != -1 else abs_start

    replacement = '''void XenoVM::handleINPUT(const XenoInstruction& instr) {
    if (instr.arg1 >= string_table.size()) {
        Serial.println("ERROR: Invalid variable name index in INPUT");
        running = false;
        return;
    }
    String var_name = string_table[instr.arg1];
    Serial.print("INPUT ");
    Serial.print(var_name);
    Serial.println(":");
    const unsigned long TIMEOUT_MS = 30000;
    XenoString raw = Serial.readStringTimeout(TIMEOUT_MS);
    String input_str = raw;
    input_str.trim();

    if (input_str.isEmpty()) {
        Serial.println("TIMEOUT - using default value 0");
        variables[var_name] = XenoValue::makeInt(0);
        return;
    }
    XenoString temp = input_str;
    temp.trim();
    XenoString lowered = temp.toLower();
    XenoValue input_value;
    if (isInteger(temp)) {
        input_value = XenoValue::makeInt(temp.toInt());
    } else if (isFloat(temp)) {
        input_value = XenoValue::makeFloat(temp.toFloat());
    } else if (lowered == "true" || lowered == "false") {
        input_value = XenoValue::makeBool(lowered == "true");
    } else {
        input_value = XenoValue::makeString(addString(temp));
    }
    variables[var_name] = input_value;
    Serial.print("-> ");
    Serial.println(input_str);
}'''

    new_content = content[:removal_start] + replacement + content[end_pos+1:]
    return new_content

def process_header_file(content):
    """Process .h file - add arduino_compat.h include and String defines."""
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

    print("Starting Arduino to arduino_compat.h conversion (src/ required)...")
    print(f"Project root: {os.path.abspath(project_path)}")

    success = process_all_files(project_path)

    if success:
        print("\nAll files processed successfully!")
        sys.exit(0)
    else:
        print("\nProcessing failed (see messages above). Ensure 'src/' exists and fix any file errors.")
        sys.exit(1)
