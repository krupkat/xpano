---
layout: default
---

## Features

The tool focuses on simplicity and ease of use, features include:

- Auto detection of groups of images that can be stitched into panoramas
- Preview + zoom + pan of the computed panoramas
- Crop mode, boundary auto fill, selectable projection types
- Export of full resolution panoramas

## Demo

Check out the app in action on [YouTube](https://youtu.be/MyiTC3i1hK0).

This is how the app looks after importing a directory of 200 images.

![Main Xpano gui](https://raw.githubusercontent.com/krupkat/xpano/main/misc/screenshots/xpano.jpg)

## Built with

The app uses the excellent [OpenCV](https://opencv.org/) library for image manipulation and its [stitching](https://docs.opencv.org/4.x/d1/d46/group__stitching.html) module for computing the panoramas.

Other dependencies include [imgui](https://github.com/ocornut/imgui), [SDL](https://github.com/libsdl-org/SDL), [spdlog](https://github.com/gabime/spdlog/), [Catch2](https://github.com/catchorg/Catch2), [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended), [alpaca](https://github.com/p-ranav/alpaca), [thread-pool](https://github.com/bshoshany/thread-pool) and the [Google Noto](https://fonts.google.com/noto) fonts.

## Download

Install directly from Flathub or the Microsoft Store:

<a href='https://flathub.org/apps/details/cz.krupkat.Xpano'><img height='70' alt='Download from Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/></a>&nbsp;&nbsp;<a href='https://apps.microsoft.com/store/detail/9PGQ5X33L0SC?launch=true&mode=full'><img height='70' alt='Download from the Microsoft Store' src='https://get.microsoft.com/images/en-US%20dark.svg'/></a>

Additionally you can download Windows executables from [GitHub](https://github.com/krupkat/xpano/releases) and Ubuntu packages from a [Launchpad PPA](https://launchpad.net/~krupkat/+archive/ubuntu/code).

## Command line

Xpano has basic CLI support, you can either run it fully automatic in the command line, or launch to gui with the `--gui` flag.

```
Xpano [<input files>] [--output=<path>] [--gui] [--help] [--version]
```

## Development

Check out the [build instructions](https://github.com/krupkat/xpano#development) for MacOS, Linux and Windows.

## License

Distributed under the GPL-3.0 license. See the [license](https://github.com/krupkat/xpano/blob/main/LICENSE) page for more information.
