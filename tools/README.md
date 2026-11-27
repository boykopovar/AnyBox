# Checking PE imports

Build AnyBox first. Install the optional `pefile` dependency with `python -m pip install pefile`, then pass the desired build's DLL directory followed by the executable and any companion DLLs you want to inspect:

```powershell
python tools/check_imports.py build/bin/Release title.exe companion.dll
```

The checker reads the files without loading them. It compares regular and delay-load imports by name or ordinal against the actual PE export tables of the AnyBox DLLs. It lists imports of other modules separately for investigation.

Exit code `0` means every checked AnyBox import resolved. Exit code `1` means an AnyBox library or export is missing. Exit code `2` means a file could not be inspected. An exported function may still be a stub. Imports resolved dynamically by name at runtime are outside the scan.
