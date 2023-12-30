{ pkgs ? import
    (builtins.fetchTarball {
      name = "nixos-tapir-2023-12-29";
      url = "https://github.com/nixos/nixpkgs/archive/d02d818f22c777aa4e854efc3242ec451e5d462a.tar.gz";
      sha256 = "1r6ma7lmi4x5pcxi84fbils1xgnpmmkzi7dd75zhqvg1sds3z47z";
    })
    { }
}:

let
  asanEnv = pkgs.withCFlags [
    "-g"
    "-fsanitize=address"
    "-fno-omit-frame-pointer"
    "-Wno-error=maybe-uninitialized"
  ]
    pkgs.gcc12Stdenv;
  addAsanEnv = pkg: pkg.override { stdenv = asanEnv; };
in

asanEnv.mkDerivation {
  name = "asan-environment-xpano";

  buildInputs = with pkgs; ([
    cmake
    ninja
    pkg-config
    (python3.withPackages (pkgs: with pkgs; [ pyyaml ]))
  ] ++ (map addAsanEnv [
    opencv
    SDL2
    catch2_3
    spdlog
    exiv2
    dbus
  ]));
}