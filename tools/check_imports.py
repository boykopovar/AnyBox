"""Compare static PE imports with the exports of built AnyBox DLLs."""

import argparse
from collections import defaultdict
from pathlib import Path
import sys

try:
    import pefile
except ModuleNotFoundError:
    sys.exit("Install the optional dependency with python -m pip install pefile")


LIBRARY_SOURCE = Path(__file__).resolve().parents[1] / "core" / "libs"
IMPORT_TABLES = ("DIRECTORY_ENTRY_IMPORT", "DIRECTORY_ENTRY_DELAY_IMPORT")


def read_imports(path):
    imports = defaultdict(set)
    with pefile.PE(str(path), fast_load=True) as image:
        image.parse_data_directories(
            directories=[pefile.DIRECTORY_ENTRY[f"IMAGE_{table}"] for table in IMPORT_TABLES]
        )
        for table in IMPORT_TABLES:
            for module in getattr(image, table, ()):
                dll = module.dll.decode("ascii", errors="backslashreplace").lower()
                imports[dll].update(
                    entry.name if entry.name is not None else entry.ordinal
                    for entry in module.imports
                )
    return imports


def read_exports(path):
    with pefile.PE(str(path), fast_load=True) as image:
        image.parse_data_directories(
            directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"]]
        )
        table = getattr(image, "DIRECTORY_ENTRY_EXPORT", None)
        if table is None:
            return set()
        symbols = {entry.name for entry in table.symbols if entry.name is not None}
        symbols.update(entry.ordinal for entry in table.symbols)
        return symbols


def format_symbol(symbol):
    if isinstance(symbol, int):
        return f"ordinal #{symbol}"
    return symbol.decode("ascii", errors="backslashreplace")


def check_imports(path, anybox_dlls, built_dlls, export_cache):
    print(path)
    passed = True
    other_modules = []
    checked_modules = 0
    for dll, imports in sorted(read_imports(path).items()):
        if dll not in anybox_dlls:
            other_modules.append(dll)
            continue
        checked_modules += 1
        if dll not in built_dlls:
            print(f"  {dll} missing from the build")
            passed = False
            continue
        if dll not in export_cache:
            export_cache[dll] = read_exports(built_dlls[dll])
        missing = sorted(map(format_symbol, imports - export_cache[dll]))
        if missing:
            print(f"  {dll} missing {', '.join(missing)}")
            passed = False
        else:
            print(f"  {dll} provides all {len(imports)} imported symbols")
    if not checked_modules:
        print("  No AnyBox imports found")
    if other_modules:
        print(f"  Other imported modules: {', '.join(other_modules)}")
    return passed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime_dir", type=Path, help="directory containing the built AnyBox DLLs")
    parser.add_argument("images", nargs="+", type=Path, help="PE executables or DLLs to inspect")
    args = parser.parse_args()

    anybox_dlls = {f"{definition.stem.lower()}.dll" for definition in LIBRARY_SOURCE.glob("*/*.def")}
    export_cache = {}
    passed = True
    try:
        built_dlls = {
            file.name.lower(): file
            for file in args.runtime_dir.iterdir()
            if file.is_file() and file.name.lower() in anybox_dlls
        }
        for path in args.images:
            if not check_imports(path, anybox_dlls, built_dlls, export_cache):
                passed = False
    except (OSError, pefile.PEFormatError) as error:
        print(f"Could not inspect PE files: {error}", file=sys.stderr)
        return 2
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
