cmake -B build `
  -DBUILD_TESTING=ON `
  -DXPANO_STATIC_VCRT=ON `
  -DCMAKE_INSTALL_PREFIX=install `
  -DSDL2_DIR="sdl/install/cmake" `
  -DOpenCV_STATIC=ON `
  -DOpenCV_DIR="opencv/install" `
  -Dspdlog_DIR="spdlog/install_spdlog/lib/cmake/spdlog" `
  -DCatch2_DIR="../catch/install/lib/cmake/Catch2" `

cmake --build build --config $env:BUILD_TYPE --target install -j 12