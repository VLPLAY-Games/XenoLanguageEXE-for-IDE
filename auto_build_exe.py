"""
Performs:
  - cmake .. -G "Visual Studio 17 2022" -A x64
  - cmake --build . --config Release
Then:
  - copies build/Release/xeno_host.exe -> build/xeno_host.exe
  - removes all other files and directories inside build/

Usage examples:
  python auto_build_exe.py
  python auto_build_exe.py C:\path\to\project
  python auto_build_exe.py . --cmake "C:\Program Files\CMake\bin\cmake.exe"
  python auto_build_exe.py . --vsdevcmd "C:\...VsDevCmd.bat"
"""
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

DEFAULT_GENERATOR = "Visual Studio 17 2022"
DEFAULT_ARCH = "x64"
DEFAULT_CONFIG = "Release"
DEFAULT_ARTIFACT = "xeno_host.exe"

def find_cmake(explicit_path=None):
    if explicit_path:
        p = Path(explicit_path)
        if p.is_file():
            return str(p)
        return None
    # try PATH first
    cm = shutil.which("cmake")
    if cm:
        return cm
    # common install locations
    common = [
        r"C:\Program Files\CMake\bin\cmake.exe",
        r"C:\Program Files (x86)\CMake\bin\cmake.exe",
        r"C:\soft\cmake\bin\cmake.exe"
    ]
    for c in common:
        if Path(c).is_file():
            return c
    return None

def run_direct(cmd_list, cwd=None):
    print("Running:", " ".join(cmd_list))
    p = subprocess.run(cmd_list, cwd=cwd)
    if p.returncode != 0:
        raise RuntimeError(f"Command failed with exit code {p.returncode}: {' '.join(cmd_list)}")

def run_via_vsdevcmd(vsdevcmd, commands, cwd=None):
    """
    Run commands in a cmd session that first calls vsdevcmd.
    `commands` is a list of command strings (shell form); they will be joined with &&.
    """
    safe_vs = f'"{vsdevcmd}"'
    joined = " && ".join(commands)
    full = f'call {safe_vs} && {joined}'
    print("Running in cmd via VsDevCmd:", full)
    # Use shell=True to run full command string under cmd.exe
    p = subprocess.run(full, shell=True, cwd=cwd)
    if p.returncode != 0:
        raise RuntimeError(f"Command chain failed with exit code {p.returncode}")

def cleanup_build_dir_keep_artifact(build_dir: Path, artifact_name: str):
    """
    Keep only build/<artifact_name>; delete all other files & directories inside build_dir.
    If artifact is in a subfolder (e.g. build/Release/<artifact>), caller should copy it to build_dir first.
    """
    if not build_dir.exists() or not build_dir.is_dir():
        raise RuntimeError(f"Build directory does not exist: {build_dir}")

    kept = build_dir / artifact_name
    if not kept.exists():
        raise RuntimeError(f"Expected artifact not found in build root: {kept}")

    for entry in list(build_dir.iterdir()):
        # Skip the kept artifact
        try:
            if entry.is_file():
                if entry.name == artifact_name:
                    continue
                print(f"Removing file: {entry}")
                entry.unlink()
            elif entry.is_dir():
                # do not try to rmtree if this is the artifact (artifact is expected as file)
                print(f"Removing directory tree: {entry}")
                shutil.rmtree(entry)
            else:
                # other filesystem objects
                print(f"Removing: {entry}")
                try:
                    entry.unlink()
                except Exception:
                    # best-effort
                    pass
        except Exception as e:
            print(f"Warning: failed to remove {entry}: {e}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("project", nargs="?", default=".", help="Path to project root (contains top-level CMakeLists.txt)")
    parser.add_argument("--cmake", help="Explicit path to cmake executable")
    parser.add_argument("--vsdevcmd", help="Optional path to VsDevCmd.bat to initialize VS build environment")
    parser.add_argument("--generator", default=DEFAULT_GENERATOR, help="CMake generator (default: Visual Studio 17 2022)")
    parser.add_argument("--arch", default=DEFAULT_ARCH, help="Architecture (default: x64)")
    parser.add_argument("--config", default=DEFAULT_CONFIG, help="Build configuration (default: Release)")
    parser.add_argument("--build-dir", default="build", help="Build directory (default: build)")
    parser.add_argument("--artifact", default=DEFAULT_ARTIFACT, help="Final executable name to keep (default: xeno_host.exe)")
    args = parser.parse_args()

    project = Path(args.project).resolve()
    if not project.exists():
        print("ERROR: project path does not exist:", project)
        return 2

    build_dir = (project / args.build_dir).resolve()
    build_dir.mkdir(parents=True, exist_ok=True)

    cmake_exe = find_cmake(args.cmake)
    if not cmake_exe:
        print("ERROR: cmake not found.")
        print(" - Install CMake and add it to PATH, or pass its path via --cmake.")
        return 3

    # Prepare commands (string form for vsdevcmd path; list form for direct execution)
    cfg_cmd_str = f'"{cmake_exe}" .. -G "{args.generator}" -A {args.arch}'
    build_cmd_str = f'"{cmake_exe}" --build . --config {args.config}'

    try:
        if args.vsdevcmd:
            # run in VsDevCmd environment
            run_via_vsdevcmd(args.vsdevcmd, [cfg_cmd_str, build_cmd_str], cwd=str(build_dir))
        else:
            # run direct (expects cmake in PATH and environment prepared if needed)
            run_direct([cmake_exe, "..", "-G", args.generator, "-A", args.arch], cwd=str(build_dir))
            run_direct([cmake_exe, "--build", ".", "--config", args.config], cwd=str(build_dir))
    except FileNotFoundError as fe:
        print("FileNotFoundError:", fe)
        print("This usually means the executable wasn't found in PATH.")
        return 4
    except RuntimeError as re:
        print("Error:", re)
        return 5

    # After successful build: copy artifact from build/<Config>/<artifact> -> build/<artifact>
    artifact_name = args.artifact
    artifact_src = build_dir / args.config / artifact_name
    artifact_dst = build_dir / artifact_name

    if not artifact_src.exists():
        print(f"ERROR: Built artifact not found at expected location: {artifact_src}")
        print("Possible reasons: different project name, output path changed, or build failed.")
        return 6

    try:
        print(f"Copying artifact: {artifact_src} -> {artifact_dst}")
        shutil.copy2(artifact_src, artifact_dst)
    except Exception as e:
        print("ERROR copying artifact:", e)
        return 7

    # Now remove everything else inside build_dir except artifact_dst
    try:
        print("Cleaning up build directory, keeping only:", artifact_dst.name)
        cleanup_build_dir_keep_artifact(build_dir, artifact_name)
    except Exception as e:
        print("ERROR during cleanup:", e)
        return 8

    print("Build+artifact handling completed successfully.")
    return 0

if __name__ == "__main__":
    rc = main()
    sys.exit(rc)
