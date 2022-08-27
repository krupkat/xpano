[![MacOS](https://github.com/krupkat/xpano/actions/workflows/macos.yml/badge.svg)](https://github.com/krupkat/xpano/actions/workflows/macos.yml)
[![Ubuntu](https://github.com/krupkat/xpano/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/krupkat/xpano/actions/workflows/ubuntu.yml)
[![Windows](https://github.com/krupkat/xpano/actions/workflows/windows.yml/badge.svg)](https://github.com/krupkat/xpano/actions/workflows/windows.yml)

# xpano

Tool for stitching photos with focus on simplicity and ease of use.

After you import your images, the app autodetects groups of images to stitch into panoramas. You can then check the panorama previews and run export to save the full resolution results.

## Built with

The app uses the excellent [OpenCV](https://opencv.org/) library for image manipulation and its [stitching](https://docs.opencv.org/4.x/d1/d46/group__stitching.html) module for computing the panoramas.

Other dependencies include [imgui](https://github.com/ocornut/imgui), [SDL](https://github.com/libsdl-org/SDL), [spdlog](https://github.com/gabime/spdlog/), [Catch2](https://github.com/catchorg/Catch2), [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended), [thread-pool](https://github.com/bshoshany/thread-pool) and the [Google Noto](https://fonts.google.com/noto) fonts.

## Demo

Check out the demo on [YouTube](https://youtu.be/-TuKaO9gxsU). This is how the app looks after importing a directory of 200 images.

![Main Xpano gui](misc/screenshots/xpano.jpg)

## Releases

Currently there are release packages available for the Windows x64 platform. You can either get the app from the [Microsoft Store](https://www.microsoft.com/store/productId/9PGQ5X33L0SC) or here on GitHub from the [release page](https://github.com/krupkat/xpano/releases). In case you download the binaries from here, you can verify the build commit and checksum in the [build logs](https://github.com/krupkat/xpano-release-builder/actions/workflows/build.yml).

## Local build

The project can be built by running a single script from the `misc/build` directory. You will need at least CMake 3.21, git and a compiler with C++20 support.

### MacOS

Run the install script from the root of the repository:

```
./misc/build/build-macos-12.sh
```

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

## Contributions

Contributions are more than welcome, there is a couple of ideas for enhancements in [open issues](https://github.com/krupkat/xpano/issues) which you could take on - if you start working on one of them, please add a comment there. 

Feel free to open an issue with your comments / encountered bugs / requests for features. 

In case you want to contribute a PR, please open an issue first with a bit of details about the planned change.

## License

Distributed under the GPL-3.0 license. See `LICENSE` for more information.

## Contact

Tomas Krupka - [krupkat.cz](https://krupkat.cz)
