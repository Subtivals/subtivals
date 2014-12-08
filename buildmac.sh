# This a Makefile for Desktop Qt 4.8.1 for GCC (Qt SDK) Release

#export PATH=/usr/bin:/Users/mallox/QtSDK/Desktop/Qt/4.8.1/gcc/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH
#export QTDIR=/Users/mallox/QtSDK/Desktop/Qt/4.8.1/gcc/

##Build Steps

#clean 
"/Users/mallox/QtSDK/Desktop/Qt/4.8.1/gcc/bin/qmake" /Users/mallox/Documents/subtivals/src/subtivals.pro -r -spec macx-g++ CONFIG+=release
make clean -w
make -w

# generate DMG file
"/Users/mallox/QtSDK/Desktop/Qt/4.8.1/gcc/bin/macdeployqt" subtivals.app -dmg