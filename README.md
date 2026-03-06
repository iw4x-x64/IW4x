<p align="center">
  <img src=".github/banner.svg" alt="IW4x (x64) Banner"/>
</p> <hr>

## Getting Started

IW4x is built using the [build2](https://build2.org/build2-toolchain/doc/build2-toolchain-intro.xhtml#preface) build system. To get started, you'll need the staged toolchain, which you can grab as either source or binary packages. The pre-built binaries are available here:

* [Staged Toolchain](https://stage.build2.org/0/0.18.0-a.0/bindist/)
* [SHA256 Checksums](https://stage.build2.org/0/toolchain-bindist.sha256)

You'll also need a working C++ compiler. Use MinGW if you're cross-compiling on Linux, or MSVC if you're on Windows. If you need help setting these up, you can find installation instructions below:

* [MinGW-w64](https://www.mingw-w64.org/)
* [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)

### Consumption

> [!NOTE]
> **Consumption** is just `build2`'s term for building the package without actively modifying the code. These steps are great if you want to test the latest development builds, but we don't recommend them for regular players just looking to install and play the game.

#### Windows

```powershell
# Create the build configuration:
bpkg create -d iw4x cc                     ^
  config.cxx=cl                            ^
  config.install.bin=/path/to/game/folder  ^
  config.install.root=/path/to/game/folder ^
  config.install.filter='include/@false lib/@false share/@false'
cd iw4x

# To build:
bpkg build libiw4x@https://github.com/iw4x-x64/IW4x.git#main

# To test:
bpkg test libiw4x

# To install:
bpkg install libiw4x

# To uninstall:
bpkg uninstall libiw4x

# To upgrade:
bpkg fetch
bpkg status libiw4x
bpkg uninstall libiw4x
bpkg build --upgrade --recursive libiw4x
bpkg install libiw4x
```

#### Linux

```bash
# Create the build configuration:
bpkg create -d iw4x cc                     \
  config.cxx=x86_64-w64-mingw32-g++        \
  config.install.bin=/path/to/game/folder  \
  config.install.root=/path/to/game/folder \
  config.install.filter='include/@false lib/@false share/@false'
cd iw4x

# To build:
bpkg build libiw4x@https://github.com/iw4x-x64/IW4x.git#main

# To test:
bpkg test libiw4x

# To install:
bpkg install libiw4x

# To uninstall:
bpkg uninstall libiw4x

# To upgrade:
bpkg fetch
bpkg status libiw4x
bpkg uninstall libiw4x
bpkg build --upgrade --recursive libiw4x
bpkg install libiw4x
```

If you want to dive deeper into how these commands work, check out the [build2 package consumption guide](https://build2.org/build2-toolchain/doc/build2-toolchain-intro.xhtml#guide-consume-pkg).

### Development

> [!NOTE]
> These instructions are meant for developers looking to modify and contribute to the IW4x codebase. If you just want to compile and test the latest build, follow the **Consumption** steps above instead.

#### Linux

```bash
# Clone the repository:
git clone --recursive https://github.com/iw4x-x64/IW4x.git iw4x && cd iw4x

# Create the build configuration:
bdep init -C @mingw32-debug                                      \
  config.cxx=x86_64-w64-mingw32-g++                              \
  config.cc.coptions="-ggdb                                      \
                      -grecord-gcc-switches                      \
                      -pipe                                      \
                      -mtune=generic                             \
                      -fasynchronous-unwind-tables               \
                      -fno-omit-frame-pointer                    \
                      -mno-omit-leaf-frame-pointer"              \
  config.cc.loptions="-static-libgcc -static-libstdc++"          \
  config.cc.compiledb=./                                         \
  cc                                                             \
  config.install.bin=/path/to/game/folder                        \
  config.install.root=/path/to/game/folder                       \
  config.install.filter='include/@false lib/@false share/@false'

# To build:
b

# To test:
b test
```

#### Windows

```powershell
# Clone the repository:
git clone --recursive https://github.com/iw4x-x64/IW4x.git iw4x && cd iw4x

# Create the build configuration:
bdep init -C -@msvc cc                                           ^
  config.cxx=cl                                                  ^
  config.cc.coptions=/Z7                                         ^
  config.cc.compiledb=./                                         ^
  config.install.bin=/path/to/game/folder                        ^
  config.install.root=/path/to/game/folder                       ^
  config.install.filter='include/@false lib/@false share/@false'

# To build:
b

# To test:
b test
```

For more details on `bdep` and typical development workflows, take a look at the [build2 build system manual](https://build2.org/build2/doc/build2-build-system-manual.xhtml).

## License

IW4x is licensed under the GNU General Public License v3.0 or later. See [LICENSE.md](LICENSE.md) for details.
