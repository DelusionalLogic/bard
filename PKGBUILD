# Maintainer: Jesper Jensen <delusionallogic@gmail.com> 

_pkgname=bard
pkgname=${_pkgname}-git
pkgver=49
pkgrel=1
pkgdesc='A tool for generating a system bar by using lemonbar'
arch=('i686' 'x86_64')
url="https://github.com/DelusionalLogic/${_pkgname}"
license=('GPL')
depends=('lemonbar' 'iniparser' 'libx11')
makedepends=('git')
optdepends=()
provides=("${_pkgname}")
conflicts=("${_pkgname}")
source=("git://github.com/DelusionalLogic/${_pkgname}.git")
md5sums=('SKIP')

pkgver() {
	cd "$srcdir/$_pkgname"
	git rev-list --count HEAD
}

build() {
	cd "$srcdir/$_pkgname"
	./configure
	make PREFIX=/usr
}

package() {
	cd "$srcdir/$_pkgname"
	make PREFIX=/usr DESTDIR="$pkgdir" install
}
