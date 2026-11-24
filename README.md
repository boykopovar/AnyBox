# About

AnyBox is a compatibility layer for running Xbox Series X|S executables natively on Windows. Part of [AnyPS5](https://github.com/boykopovar/AnyPS5) ecosystem. The [shader recompiler from AnyPS5 core](https://github.com/boykopovar/AnyPS5/tree/main/core/shader/recompiler) is planned to be used.

## Status

Initial development. No playable titles confirmed.

Seven system libraries with a total of 39 functions required by the test title and its bundled libraries have been added and build successfully. Launch has been tested with the title's executable.

Unimplemented functions throw a C++ exception. Cross-library stack unwinding and destructor execution are verified by tests. The title reaches an unimplemented function and terminates with an unhandled MSVC-compatible C++ exception; its call stack is visible in the debugger. Destructor execution during unwinding through the title's code has not been verified.

APU memory uses ordinary host memory with cached and non-cached partitions, synthetic 32-bit APU addresses, and lazy default heap creation. This models the memory API; it does not emulate audio-processing hardware or host cache behavior.

## Building

AnyBox uses standard C++20 and can be built with any conforming compiler targeting 64-bit Windows. MSVC or `clang-cl` with the MSVC runtime is recommended for ABI compatibility and correct stack unwinding of diagnostic C++ exceptions through title code.

To build and test with Visual Studio:

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

To build and test with MinGW-w64:

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-mingw --parallel
ctest --test-dir build-mingw -C Debug --output-on-failure
```

## Disclaimer

The project is intended for interoperability, research and preservation. Game files, firmware, cryptographic keys and proprietary SDK libraries are not distributed with the project.

AnyBox is an independent project and is not affiliated with, authorized or endorsed by Microsoft. Xbox is a trademark of Microsoft Corporation.

## License

This project is licensed under the [GNU General Public License version 2 only](LICENSE).
