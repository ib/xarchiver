#!/bin/sh

test "$BASH" && set -o pipefail

mkdir m4

echo -n "Creating the build system... "

intltoolize --copy --force --automake || exit
libtoolize --copy --force --quiet || exit
aclocal || exit
autoheader || exit
automake --copy --force-missing --add-missing --gnu 2>&1 | sed "/installing/d" || exit
autoconf -Wno-obsolete || exit

echo "done."

# clean up in order to keep repository small
# (will be needed if 'make dist' is used though)
rm INSTALL aclocal.m4 intltool-extract.in intltool-merge.in intltool-update.in
rm -f config.h.in~ configure~
rm -r autom4te.cache m4
