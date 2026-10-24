# IW4x

The repository holds one package, [`libiw4x`](libiw4x/README.md).

## Development

The development setup uses the standard `bdep`-based workflow and needs a C++26
compiler with reflection, such as GCC 16. Every configuration targets Windows.
On Linux, use a MinGW-w64 GCC configuration that runs the tests under Wine.
Wine has to find the MinGW runtime DLLs, so the test runner adds their
directory to `WINEPATH` (here, as on Fedora):

```
git clone https://github.com/iw4x-x64/iw4x.git
cd iw4x

bdep init --create-host-config -C @mingw cc \
  config.cxx=x86_64-w64-mingw32-g++ \
  config.test.runner="env WINEPATH=/usr/x86_64-w64-mingw32/sys-root/mingw/bin wine"
bdep update
bdep test
```

On Windows, with MSVC, from the Visual Studio "x64 Native Tools Command
Prompt", which builds 64-bit binaries:

```
git clone https://github.com/iw4x-x64/iw4x.git
cd iw4x

bdep init --create-host-config -C @msvc cc config.cxx=cl
bdep update
bdep test
```

In PowerShell, a leading `@` has a special meaning (splatting), so write the
configuration name as `-@msvc`, which `bdep` also accepts:

```
bdep init --create-host-config -C -@msvc cc config.cxx=cl
```

## Contributing

See [CONTRIBUTING](.github/CONTRIBUTING.md).

## License

IW4x is licensed under the GNU General Public License, version 3, subject to
the additional permissions described in version 1.1 of the IW4x Linking
Exception.

See [LICENSE](LICENSE.md), [LICENSE-EXCEPTION](LICENSE-EXCEPTION.md), and
[AUTHORS](AUTHORS).
