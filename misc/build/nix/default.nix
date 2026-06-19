{ pkgs ? import <nixpkgs> { }
}:

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
    ruby_4_0
  ];
}