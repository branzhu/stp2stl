# stp2stl

A minimal STEP → STL converter built on Open CASCADE Technology (OCCT) 7.9.3.
It provides:

- a small C API (single file `stp2stl` shared library)
- a CLI tool (`stp2stl` executable)

OCCT is statically linked into stp2stl, so redistribution is as simple as shipping a single `stp2stl` dynamic library (~11.3MB on windows), instead of dozens of OCCT/third-party libraries and dependencies totaling hundreds of MB.

## Requirements

- CMake 3.23+
- A C++17 compiler
- OCCT 7.9.3 (provided as a Git submodule by default)

## Get the source

This project expects OCCT 7.9.3 at `third_party/occt`.

```sh
git submodule update --init --recursive
cd third_party/occt
git checkout V7_9_3
cd ../..
```

## Build

### Option 1: Superbuild (default)

Superbuild (`-DSTP2STL_SUPERBUILD=ON`) first builds a trimmed static OCCT under `build/_deps/`, then builds `stp2stl` against that install tree.

**Windows (Visual Studio 2022 x64)**

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Artifacts (Superbuild):

- `build\\_deps\\stp2stl-build\\bin\\Release\\stp2stl.exe`
- `build\\_deps\\stp2stl-build\\bin\\Release\\stp2stl.dll`

**Linux/macOS (Ninja example)**

```sh
cmake -S . -B build -G Ninja -DSTP2STL_SUPERBUILD=ON -DSTP2STL_SUPERBUILD_CONFIG=Release
cmake --build build
```

Artifacts (typical single-config output):

- `build/_deps/stp2stl-build/bin/stp2stl`
- `build/_deps/stp2stl-build/bin/libstp2stl.so` (Linux) / `libstp2stl.dylib` (macOS)

### Option 2: Use an existing OCCT install (no Superbuild)

If you already have OCCT 7.9.3 installed, disable Superbuild and point CMake to it:

```sh
cmake -S . -B build -DSTP2STL_SUPERBUILD=OFF -DCMAKE_PREFIX_PATH=<occt-install-prefix>
cmake --build build
```

Artifacts (no Superbuild):

- `build/bin/stp2stl` (or `build/bin/Release/stp2stl.exe` on multi-config generators)
- `build/bin/libstp2stl.*` (or `build/bin/Release/stp2stl.dll` on Windows)

## CLI usage

```sh
stp2stl input.step output.stl --binary --relative --deflection 0.001 --angle 20
```

Options:

- `--deflection <v>` Linear deflection (default: `0.001`)
- `--angle <deg>` Angular deflection in degrees (default: `20`)
- `--relative` / `--absolute` Relative/absolute deflection (default: relative)
- `--binary` / `--ascii` Output format (default: binary)
- `--scale <v>` Scale factor (default: `1.0`)
- `--parallel` Enable parallel meshing (default: off)
- `--version` Print version

## Library API

Public API is declared in `include/stp2stl/stp2stl.h`.

- `stp2stl_default_options(stp2stl_options*)`
- `stp2stl_convert_utf8(const char* input_step, const char* output_stl, const stp2stl_options*)`
- `stp2stl_last_error_utf8()`
- `stp2stl_version()`

### Path encoding contract

The library accepts UTF-8 encoded byte strings for both input/output paths.
On Windows, the CLI reads arguments as UTF-16 (`wmain`) and converts them to UTF-8 before calling the library.

## Notes

- STL has no unit metadata. Use `--scale` to control the output size (e.g. `--scale 0.001` converts mm to m).
- OCCT licensing applies to the OCCT submodule and to binaries that include OCCT; see `THIRD_PARTY_NOTICES.md` and `licenses/`.

## Licensing

### stp2stl

The `stp2stl` code in this repository is released under the MIT License. See `LICENSE`.

### Third-party software (OCCT)

This project vendors Open CASCADE Technology (OCCT) under `third_party/occt` (LGPL-2.1 with the OCCT exception).

If you redistribute binaries that include OCCT (including static linking), you are responsible for complying with OCCT's license terms.
See:

- `THIRD_PARTY_NOTICES.md`
- `licenses/LICENSE_LGPL_21.txt`
- `licenses/OCCT_LGPL_EXCEPTION.txt`
