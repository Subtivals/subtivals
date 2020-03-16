qmake src/subtivals.pro -r -spec macx-g++ CONFIG+=release
make clean -w
make -w
macdeployqt subtivals.app -dmg
