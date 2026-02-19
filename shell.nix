{ pkgs ? import <nixpkgs> {} }:

let
  mingw = pkgs.pkgsCross.mingwW64;
in
pkgs.mkShell {
  name = "iw4x";

  buildInputs = [
    # The MinGW-w64 cross-compiler itself.
    #
    mingw.stdenv.cc
    mingw.windows.pthreads

    # Fish shell.
    #
    pkgs.fish
  ];

  shellHook = ''
    # Replace bash process with fish.
    #
    exec fish

    # The build2 toolchain (b, bpkg, bdep).
    #
    # Notice that we "cheat" here: we provide the build2 toolchain manually,
    # which effectively bypasses the Nix sandbox. We do this because:
    #
    #   (I)  We require the build2 'stage' (bleeding edge) toolchain.
    #   (II) Revisions change almost daily, making standard Nix fixed-output
    #        derivations high-maintenance.
    #
    # Note also that this is only temporary, we will transition back to a pure
    # Nix derivation once 0.18.0 reaches a stable release.
    #
    fish_add_path --path --append $PWD/.build2-toolchain
  '';
}
