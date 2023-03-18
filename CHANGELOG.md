# Xpano 0.9.0

- Flatpak support

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
