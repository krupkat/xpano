{ pkgs ? import <nixpkgs> { }
}:
let
  # In LLVM 22, run-clang-tidy.py moved from share/clang/ to bin/, so nixpkgs
  # clang-tools no longer links it. Wrap it manually.
  runClangTidy = pkgs.writeShellScriptBin "run-clang-tidy-22" ''
    exec ${pkgs.python3}/bin/python3 ${pkgs.llvmPackages_22.clang-unwrapped}/bin/run-clang-tidy "$@"
  '';
  runClangFormat = pkgs.writeShellScriptBin "clang-format-22" ''
    ${pkgs.llvmPackages_22.clang-tools}/bin/clang-format "$@"
  '';
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    ninja
    pkg-config
    # opencv
    SDL2
    libx11.dev
    # catch2_3
    # spdlog
    # exiv2
    dbus
    (python3.withPackages (pkgs: with pkgs; [ pyyaml ]))
    llvmPackages_22.clang-tools
    runClangTidy
    runClangFormat
    ruby_4_0
  ];
}
