{ pkgs ? import <nixpkgs> { }
}:
let
  # Workaround taken from https://github.com/NixOS/nixpkgs/issues/189753
  #
  # To use the tool extend your cmake settings:
  #   "cmake.configureSettings": {
  #     "CMAKE_CXX_INCLUDE_WHAT_YOU_USE": [
  #         "include-what-you-use",
  #         "-Xiwyu",
  #         "--mapping_file=${workspaceFolder}/misc/mappings/iwyu_nix.imp",
  #         "-Xiwyu",
  #         "--no_fwd_decls",
  #     ]
  # },
  #
  include-what-you-use-wrapped =
    let
      llvm = pkgs.llvmPackages_15;
    in
    pkgs.wrapCCWith rec {
      cc = pkgs.include-what-you-use.override {
        llvmPackages = llvm;
      };
      extraBuildCommands = ''
        wrap include-what-you-use $wrapper $ccPath/include-what-you-use
        substituteInPlace "$out/bin/include-what-you-use" --replace 'dontLink=0' 'dontLink=1'
        substituteInPlace "$out/bin/include-what-you-use" --replace ' && isCxx=1 || isCxx=0' '&&  true; isCxx=1'

        rsrc="$out/resource-root"
        mkdir "$rsrc"
        ln -s "${llvm.clang}/resource-root/include" "$rsrc"
        echo "-resource-dir=$rsrc" >> $out/nix-support/cc-cflags
      '';
      libcxx = llvm.libcxx;
    };
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    ninja
    pkg-config
    opencv
    SDL2
    catch2_3
    spdlog
    exiv2
    dbus
    (python3.withPackages (pkgs: with pkgs; [ pyyaml ]))
    clang-tools_15
    include-what-you-use-wrapped
  ];
}
