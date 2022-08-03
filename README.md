[![Ubuntu](https://github.com/krupkat/xpano/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/krupkat/xpano/actions/workflows/ubuntu.yml)
[![Windows](https://github.com/krupkat/xpano/actions/workflows/windows.yml/badge.svg)](https://github.com/krupkat/xpano/actions/workflows/windows.yml)

# xpano
Tool for stitching photos.

## Build

The project can be built by running a single script from the `misc/install` directory. On both supported systems you will need at least CMake 3.21 and a compiler with C++20 support.

### Ubuntu

Library prerequisites:

```
sudo apt install libgtk-3-dev libopencv-dev libsdl2-dev
```

Run the install script from the root of the repository:

```
./misc/install/build-ubuntu-22.04.sh
```

### Windows

Open the "Developer PowerShell for VS 2022" profile in Windows Terminal and run the install script from the root of the repository:

```
./misc/install/build-windows-latest.ps1
```
