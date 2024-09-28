# Xpano 0.19.1

- Update Flatpak runtime to 24.08

# Xpano 0.19.0

- User selectable maximum panorama size (in MPx)
  - Useful to limit cpu / memory usage with very large panoramas

# Xpano 0.18.1

- Fix cropped panorama export

# Xpano 0.18.0

- Projection adjustments
  - Drag the panorama by its horizontal / vertical axis
  - Rotate the panorama by its center point

# Xpano 0.17.0

- Image stack detection
  - Useful to filter out groups of images that aren't supposed to be stitched

# Xpano 0.16.3

- AppImage support

# Xpano 0.16.1

- Fix for panoramas with fully obscured images

# Xpano 0.16.0

- Improved progress monitoring for stitching
- More responsive cancellation of operations
  - This allows improved interactive editing of panoramas by CTRL clicking

# Xpano 0.15.2

- Multiblend availability on ARM cpus
- Switch to Multiblend by default

# Xpano 0.15.1

- Multiblend with OpenCV seam optimization
  - To use set "Multiblend (with alpha)" in "Options -> Panorama stitching"

# Xpano 0.15.0

- Multiblend integration
  - Original code by David Horman: https://horman.net/multiblend/
  - Improved detail preservation, smoother image transitions
  - To use set the new blending type in "Options -> Panorama stitching"

# Xpano 0.14.0

- Exif metadata support
  - Copy over exif metadata from the first image of the exported panorama
- Fix crash when opening a directory (flatpak + xdg-desktop-portal < 1.7.1)

# Xpano 0.13.0

- Basic command line support
- Panorama detection type: auto, single pano, none

# Xpano 0.12.0

- Persistent user preferences
  - With option to reset to defaults
- Added support for popup info messages, e.g.:
  - Show changelog after version update
  - Warn when loading 16-bit images
- Allow loading files with the 'tif' extension
- Fix crash when minimizing the app

# Xpano 0.11.0

- Automatic panorama wave correction
  - Detects horizontal / vertical panorama, with option to override
- Quickly select projection type from the sidebar
- Chroma subsampling options for jpeg exports

# Xpano 0.10.0

- Fixes to make the stitching algorithm consistent with panorama detection
  - Default to SIFT algorithm in pano stitching
  - Match confidence the same in pano detection / stitching

# Xpano 0.9.0

- Flatpak support
  - Appstream support (since 0.9.1)
  - Simplified app icon (since 0.9.2)
  - Updated appstream release info (since 0.9.3)

# Xpano 0.8.0

- Quick import buttons
- Support window (thanks @poshi1865)

# Xpano 0.7.0

- Auto fill
  - Automatically fills the panorama boundaries with a generated texture
  - This allows the user to extend the crop rectangle
- Fix the file dialog on Linux to also show files with uppercase extensions

# Xpano 0.6.0

- Crop mode
  - Automatic detection of the largest rectangle in the panorama
  - Manual adjustment
- New action buttons in the gui: Full resolution preview, Crop mode, Export
- Stitching speedup

# Xpano 0.5.0

- Selectable projection types (OpenCV implementations):
  - perspective, cylindrical, spherical, fisheye, stereographic
  - compressed rectilinear, panini, mercator, transverse mercator
- Smooth preview zoom
- Smooth thumbnail scrolling
- Persistent window size between launches

# Xpano 0.4.0

- Linux support release
  - support system install, added deb packaing
- Control image preview size
  - decrease to get faster loading times
  - increase to get more precision for panorama detection

# Xpano 0.3.0

- MacOS build support
  - Support for Retina display and command key shortcuts
- Linux multi-monitor support
  - Sharp fractional scaling on X11, Wayland, blurry on XWayland
- Control the auto detection algorithm with two new options:
  - Matching distance: 
    - How many neighboring images will be considered for panorama auto detection
  - Matching threshold: 
    - Number of keypoints that need to match in two images to include them in a panorama
- Faster startup time thanks to async asset loading (thanks @GhostVaibhav)

# Xpano 0.2.0

First release targeting Microsoft Store:

- Microsoft Store support + app icons
- Window state + logs are saved in appdata folders
- Show stitching error code in gui (thanks @GhostVaibhav)

# Xpano 0.1.0

This release satisfies the "minimum viable product" status, supporting the
following functionality:

- Importing multiple images / a whole directory of images
- Autodetecting groups of images that can be stitched into panoramas
- Preview half resolution panoramas in the app
- Zoom and pan of the preview image
- Add / edit / delete the groups via CTRL clicking in the gui
- Export full resolution panoramas to the disk
- Choosing export format / compression options (jpg, png, tiff)
- About page showing licenses of all the dependencies

Additional features for development purposes
- Logging via spdlog
- Show debug info with CTRL+D
