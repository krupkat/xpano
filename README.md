[![Ubuntu](https://github.com/krupkat/xpano/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/krupkat/xpano/actions/workflows/ubuntu.yml)
[![Windows](https://github.com/krupkat/xpano/actions/workflows/windows.yml/badge.svg)](https://github.com/krupkat/xpano/actions/workflows/windows.yml)

# xpano

Tool for stitching photos with focus on simplicity and ease of use.

After you import your images in the app you can select from a list of autodetected panoramas to check their preview and then run export to save the full resolution result.

## Built with

The app uses the excellent [OpenCV](https://opencv.org/) library for image manipulation and its [stitching](https://docs.opencv.org/4.x/d1/d46/group__stitching.html) module for computing the panoramas.

Other dependencies include [imgui](https://github.com/ocornut/imgui), [SDL](https://github.com/libsdl-org/SDL), [spdlog](https://github.com/gabime/spdlog/), [Catch2](https://github.com/catchorg/Catch2), [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended), [thread-pool](https://github.com/bshoshany/thread-pool) and the [Google Noto](https://fonts.google.com/noto) fonts.

## Screenshot

This is how the app looks after importing a directory of 200 images.

![Main Xpano gui](misc/screenshots/xpano.jpg)

## Build

The project can be built by running a single script from the `misc/build` directory. On both supported systems you will need at least CMake 3.21 and a compiler with C++20 support.

### Ubuntu

Library prerequisites:

```
sudo apt install libgtk-3-dev libopencv-dev libsdl2-dev libspdlog-dev
```

Run the install script from the root of the repository:

```
./misc/build/build-ubuntu-22.04.sh
```

### Windows

Open the "Developer PowerShell for VS 2022" profile in Windows Terminal and run the install script from the root of the repository:

```
./misc/build/build-windows-latest.ps1
```
