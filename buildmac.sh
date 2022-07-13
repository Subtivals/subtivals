qmake src/subtivals.pro -r -spec macx-clang CONFIG+=release
make clean -w
make -w
macdeployqt subtivals.app -dmg
