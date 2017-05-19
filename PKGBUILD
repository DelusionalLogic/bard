# Maintainer: Jesper Jensen <delusionallogic@gmail.com> 

WANT_DOCS="yes"

_pkgname=bard
pkgname=${_pkgname}-git
pkgver=125
pkgrel=1
pkgdesc='A tool for generating a system bar by using lemonbar'
arch=('i686' 'x86_64')
url="https://github.com/DelusionalLogic/${_pkgname}"
license=('GPL')
depends=('lemonbar' 'iniparser' 'libx11' 'judy' 'pcre2')
makedepends=('git')
optdepends=()
provides=("${_pkgname}")
conflicts=("${_pkgname}")
source=("git://github.com/DelusionalLogic/${_pkgname}.git#branch=rewire")
md5sums=('SKIP')

if [ "$WANT_DOCS" != "no" ]; then
	makedepends+=('asciidoc')
	DOC_FLAG="--enable-documentation"
else
	DOC_FLAG="--disable-documentation"
fi


pkgver() {
	cd "$srcdir/$_pkgname"
	git rev-list --count HEAD
}

build() {
	cd "$srcdir/$_pkgname"
	./configure "$DOC_FLAG"
	make PREFIX=/usr
}

package() {
	cd "$srcdir/$_pkgname"
	make PREFIX=/usr DESTDIR="$pkgdir" mandir=/usr/share/man install
}
