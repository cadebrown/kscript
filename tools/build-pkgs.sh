#!/bin/sh
# tools/build-pkgs.sh - Build packages for various platforms/OSes/package managers
#
# TODO: Add support 'rpm', 'osxpkg', 'freebsd', 'sh', 'pacman'
#
# @author: Cade Brown <cade@kscript.org>

# Input & Output diretories
IN=.tmp/prefix
OUT=pkgs

# Ensure output is cleaned
mkdir -p $OUT

# Shared variables
VERS=0.0.1
ARCH=x86_64
NAME=$OUT/ks-$VERS-$ARCH

# Main contact
MAIN='Cade Brown <cade@kscript.org>'

# Project URL
URL='https://kscript.org'

# Description
DESC="kscript ([https://kscript.org](https://kscript.org)) is a dynamic programming language with expressive syntax, cross platform support, and a rich standard library. It's primary aim is to allow developers to write platform agnostic programs that can run anywhere, and require little or no platform- or os- specific code."


# -*- Outputs -*-

# Tarred-directory
rm -f $NAME.tar.gz
fpm \
    -n ks -v $VERS --vendor ChemicalDevelopment \
    --license GPLv3 \
    -m "$MAIN" \
    --url "$URL" \
    -s dir -C $IN/usr \
    -t tar -p $NAME.tar.gz \
    bin lib include


# Debian (.deb)
rm -f $NAME.deb
fpm \
    -n ks -v $VERS --vendor ChemicalDevelopment \
    --license GPLv3 \
    -m "$MAIN" \
    --url "URL" \
    -s dir -C $IN \
    -t deb -p $NAME.deb \
      -d 'libc6' \
      -d 'libpthread-subs0-dev' \
      -d 'libgmp-dev' \
      -d 'libreadline-dev' \
      -d 'libavformat-dev' \
      -d 'libavcodec-dev' \
      -d 'libavutil-dev' \
      -d 'libswscale-dev' \
    usr/bin usr/lib usr/include

