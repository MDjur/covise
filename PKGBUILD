# Maintainer: Marko Djuric <hpcmdjur@hlrs.de>

_pkgname=covise
pkgname=$_pkgname-git
pkgver=1.0
pkgrel=1
epoch=
pkgdesc="COVISE - COllaborative Visualization and Simulation Environment - HLRS"
arch=(x86_64)
url="https://github.com/hlrs-vis/covise"
license=('LGPL')
groups=()
depends=(
    xerces-c
    qt5-base
    qt5-tools
    qt5-connectivity
    qt5-datavis3d
    qt5-declarative
    qt5-graphicaleffects
    qt5-imageformats
    qt5-location
    qt5-multimedia
    qt5-networkauth
    qt5-quick3d
    qt5-quickcontrols
    qt5-quickcontrols2
    qt5-script
    qt5-sensors
    qt5-serialbus
    qt5-serialport
    qt5-speech
    qt5-svg
    qt5-translations
    qt5-virtualkeyboard
    qt5-wayland
    qt5-webchannel
    qt5-webengine
    qt5-webglplugin
    qt5-webkit
    qt5-websockets
    qt5-webview
    qt5-x11extras
    qt5-xmlpatterns
    boost
    boost-libs
    glew
    lib32-glew
    libjpeg-turbo
    libtiff
    openmpi
    libpng
    giflib
    assimp
    swig
    ffmpeg
    hdf5-openmpi
    netcdf-openmpi
    netcdf-cxx
    cgns
    freealut
    freetype2
    libarchive
    libzip
    fftw
    openscenegraph
    cfitsio
    snappy
    audiofile
    proj
    gdal
    eigen
    hidapi
    sdl
    sdl2
    blas
    libvncserver
)
# makedepends=(git cmake make)
makedepends=(git cmake ninja)
checkdepends=()
optdepends=()
provides=(covise COVER opencover)
conflicts=(covise COVER opencover)
replaces=()
backup=()
options=()
install=
changelog=
source=("git+$url")
noextract=()
md5sums=('SKIP')
validpgpkeys=()

pkgver() {
    # cd "${_pkgname}"
    # printf "1.0.r%s.%s" "$( git rev-list --count HEAD )" "$(git rev-parse --short HEAD)"
    cd "$srcdir/$_pkgname"
    printf "1.0.r%s.%s" "$( git rev-list --count HEAD )" "$(git rev-parse --short HEAD)"
    # git describe --long | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}

prepare() {
    mkdir -p build
    cd "$_pkgname"
    git submodule sync
    git submodule update --init --recursive
    source .covise.sh
}

build() {
    # cd covise
    # git submodule sync
    # git submodule update --init --recursive
    # source .covise.sh
    # mkdir -p linux64opt/build.covise
    # cd linux64opt/build.covise
	# make -j$(nproc)
    cd build
    cmake ../$_pkgname \
        -DCMAKE_BUILD_TYPE=Release \
        -DCOVISE_BUILD_DRIVINGSIM=off \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -GNinja

    ninja ${MAKEFLAGS}
}

package() {
    # cd covise/build.covise
    # make install DESTDIR="$pkgdir/"
    cd build
    DESTDIR="$pkgdir/" ninja install
}
