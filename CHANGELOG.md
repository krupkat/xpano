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
