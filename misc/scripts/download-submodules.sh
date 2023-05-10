# useful for building from tarball

TAG_IMGUI="docking_v1.89.5_highdpi"
TAG_NFDE="v1.0.3"
TAG_THREAD_POOL="v3.3.1"
TAG_ALPACA="v0.2.1"

cd external

wget -O imgui.tar.gz "https://github.com/krupkat/imgui/archive/refs/tags/${TAG_IMGUI}.tar.gz"
tar xf imgui.tar.gz
mv imgui-*/* imgui

wget -O nfde.tar.gz "https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/${TAG_NFDE}.tar.gz"
tar xf nfde.tar.gz
mv nativefiledialog-extended-*/* nativefiledialog-extended

wget -O thread-pool.tar.gz "https://github.com/krupkat/thread-pool/archive/refs/tags/${TAG_THREAD_POOL}.tar.gz"
tar xf thread-pool.tar.gz
mv thread-pool-*/* thread-pool

wget -O alpaca.tar.gz "https://github.com/p-ranav/alpaca/archive/refs/tags/${TAG_ALPACA}.tar.gz"
tar xf alpaca.tar.gz
mv alpaca-*/* alpaca

cd ..
