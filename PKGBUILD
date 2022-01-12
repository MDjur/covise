# This is an example PKGBUILD file. Use this as a start to creating your own,
# and remove these comments. For more information, see 'man PKGBUILD'.
# NOTE: Please fill out the license field for your package! If it is unknown,
# then please put 'unknown'.

# Maintainer: Test <youremail@domain.com>
pkgname=hlrs-covise-git
pkgver=1.0.r6982.8529aac23
pkgrel=1
epoch=
pkgdesc="HLRS Visualization Software Tools for VR/AR"
arch=(x86_64)
url="https://github.com/hlrs-vis/covise"
license=('LGPL')
groups=()
depends=(
    cef-standard
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
    libvncserver
)
makedepends=(git cmake make)
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

# prepare() {
# 	cd "$pkgname-$pkgver"
# 	patch -p1 -i "$srcdir/$pkgname-$pkgver.patch"
# }

pkgver() {
    cd "${_pkgname}"
    printf "1.0.r%s.%s" "$( git rev-list --count HEAD )" "$(git rev-parse --short HEAD)"
}

build() {
    cd covise
    git submodule sync
    git submodule update --init --recursive
    source .covise.sh
    mkdir -p build.covise
    cd build.covise
    export CEF_ROOT=/opt/cef
    cmake -DCOVISE_BUILD_DRIVINGSIM=off ..
	make -j$(nproc)
}

# check() {
# 	cd "$pkgname-$pkgver"
# 	make -k check
# }

package() {
    cd covise/build.covise
    mkdir -p ${pkgdir}/opt/${pkgname}
    cp -rf * ${pkgdir}/opt/${pkgname}
    make install -j$(nproc) PREFIX=/usr DESTDIR="${pkgdir}"
}
