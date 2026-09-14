#!/bin/bash
set -x
export DEBIAN_FRONTEND=noninteractive
dpkg --add-architecture i386
apt-get update
apt-get install -y --no-install-recommends wine64 winetricks build-essential gcc-mingw-w64-x86-64 gcc-mingw-w64-i686 bison flex pkg-config gettext autoconf automake libtool libx11-dev libxext-dev libxrender-dev libxfixes-dev libxi-dev libxcomposite-dev libxcursor-dev libxinerama-dev libxrandr-dev libxxf86vm-dev libfreetype-dev libfontconfig-dev libgl1-mesa-dev libosmesa6-dev libvulkan-dev libgnutls28-dev libdbus-1-dev libasound2-dev libpulse-dev libcups2-dev libkrb5-dev libtiff-dev libudev-dev libunwind-dev libodbc2 unixodbc-dev libsane-dev libgsm1-dev libjpeg-dev libpng-dev libxml2-dev libxslt1-dev libldap2-dev libsasl2-dev libpcap-dev libsdl2-dev libudev-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libv4l-dev libxkbcommon-dev wayland-protocols libwayland-dev libxkbcommon-x11-dev
echo APT_EXIT=$?
apt-get install -y wine64 wine32:i386 2>&1 | tail -20
echo WINEPKGS_EXIT=$?
