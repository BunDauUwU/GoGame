"""Print physical lines and Unicode characters in the current server source."""
from pathlib import Path


def measure(path):
    text = path.read_bytes().decode("utf-8")
    return len(text.splitlines()), len(text)


def report(label, paths):
    print(f"\n{label}")
    lines = characters = 0
    for path in paths:
        file_lines, file_characters = measure(path)
        lines += file_lines
        characters += file_characters
        print(f"{path.relative_to(ROOT)}: {file_lines} lines, {file_characters} characters")
    print(f"TOTAL: {lines} lines, {characters} characters")


ROOT = Path(__file__).resolve().parents[1]
if __name__ == "__main__":
    runtime = sorted(path for path in (ROOT / "server").iterdir()
                     if path.suffix in {".cpp", ".h"})
    tests = sorted((ROOT / "server/tests").glob("*.cpp"))
    report("Server runtime (.cpp/.h)", runtime)
    report("Server tests (.cpp)", tests)
    report("Runtime + tests + server/CMakeLists.txt",
           runtime + tests + [ROOT / "server/CMakeLists.txt"])
