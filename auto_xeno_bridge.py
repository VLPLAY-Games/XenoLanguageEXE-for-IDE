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

    Important: this version removes any isInteger declaration/definition
    (for both XenoVM and XenoCompiler) instead of commenting it out.
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

        # Keep include paths intact (do NOT strip directory components from #include "...")

        # Remove isInteger declarations/definitions entirely
        content = remove_isInteger_declarations(content)
        content = remove_isInteger_definitions(content)

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

    This uses a multiline regex to remove the full line. It tolerates an
    optional leading qualifier such as `static` or `inline`.
    """
    decl_pattern = re.compile(
        r'^\s*(?:static\s+|inline\s+|virtual\s+|)?'  # optional qualifiers
        r'bool\s+isInteger\s*\(\s*const\s+String\s*&\s*\w*\s*\)\s*;\s*$',
        flags=re.MULTILINE
    )
    new_content, n = decl_pattern.subn('', content)
    if n:
        # Remove possible empty lines left behind
        new_content = re.sub(r'\n{2,}', '\n\n', new_content)
    return new_content

def remove_isInteger_definitions(content):
    """Remove full function definitions for:
        bool XenoVM::isInteger(...){...}
        bool XenoCompiler::isInteger(...){...}

    Regex alone is fragile for nested braces, so we find the start with a
    regex and then scan forward matching braces to find the real end.
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
            # Calculate absolute start index of the match
            abs_start = start_search_pos + m.start()
            # Find the opening parenthesis from abs_start
            paren_pos = content.find('(', abs_start)
            if paren_pos == -1:
                # malformed, bail out for this match
                start_search_pos = abs_start + 1
                continue

            # Find the opening brace '{' that starts the function body
            brace_pos = content.find('{', paren_pos)
            if brace_pos == -1:
                start_search_pos = abs_start + 1
                continue

            # Scan forward to find the matching closing brace
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
                # Unbalanced braces; stop trying to remove further occurrences
                break

            # Remove from start of function return type to end_pos (inclusive)
            # Heuristic: find the previous line break before abs_start
            line_start = content.rfind('\n', 0, abs_start)
            removal_start = line_start + 1 if line_start != -1 else abs_start
            removal_end = end_pos + 1  # include the closing brace

            # Perform the removal
            content = content[:removal_start] + content[removal_end:]

            # Continue searching after removal_start to find further matches
            start_search_pos = removal_start
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

    print("Starting Arduino to arduino_compat.h conversion (src/ required)...")
    print(f"Project root: {os.path.abspath(project_path)}")

    success = process_all_files(project_path)

    if success:
        print("\nAll files processed successfully!")
        sys.exit(0)
    else:
        print("\nProcessing failed (see messages above). Ensure 'src/' exists and fix any file errors.")
        sys.exit(1)
