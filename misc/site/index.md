---
layout: default
---

Tool for stitching photos with focus on simplicity and ease of use.

After you import your images, the app auto detects groups of images to stitch into panoramas. You can then check the panorama previews and run export to save the full resolution results.

## Demo

Check out the app in action on [YouTube](https://youtu.be/-TuKaO9gxsU).

This is how the app looks after importing a directory of 200 images.

![Main Xpano gui](https://raw.githubusercontent.com/krupkat/xpano/main/misc/screenshots/xpano.jpg)

## Built with

The app uses the excellent [OpenCV](https://opencv.org/) library for image manipulation and its [stitching](https://docs.opencv.org/4.x/d1/d46/group__stitching.html) module for computing the panoramas.

Other dependencies include [imgui](https://github.com/ocornut/imgui), [SDL](https://github.com/libsdl-org/SDL), [spdlog](https://github.com/gabime/spdlog/), [Catch2](https://github.com/catchorg/Catch2), [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended), [thread-pool](https://github.com/bshoshany/thread-pool) and the [Google Noto](https://fonts.google.com/noto) fonts.

## Package verification

You can verify that the binaries were built from the repository source by checking the [build logs]({{ site.build_log }}):
- The commit hash that was used for the build (in the "Checkout target repo" step)
- The sha256 checksum value (in the "Compute checksum" step)

## License

Distributed under the GPL-3.0 license. See the [license](https://github.com/krupkat/xpano/blob/main/LICENSE) page for more information.
