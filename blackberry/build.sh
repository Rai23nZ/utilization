#!/bin/bash
# ============================================================================
#  Сборка .bar для BlackBerry 10 на Linux.
#
#  Нужны: BlackBerry 10 Native SDK 10.3.1 (архив: archive.org/details/bbdevtools —
#  bbndk.win32.tools.10.3.1.12.zip и bbndk.win32.libraries.10.3.1.995.zip),
#  Wine с 32-битной частью (компилятор qcc и moc — программы для Windows) и Java.
#
#  BBNDK=/путь/к/распакованному/sdk ./build.sh        → dist/UtilizationKNT-<версия>.bar
#  ./build.sh host-test                                 → тесты ядра на хосте (Qt5 + zlib)
# ============================================================================
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE"

VERSION="$(sed -n 's:.*<versionNumber>\(.*\)</versionNumber>.*:\1:p' bar-descriptor.xml)"
SOURCES="src/main.cpp src/Controller.cpp src/Logic.cpp src/Platform.cpp src/BookRead.cpp src/XlsxRead.cpp src/XlsRead.cpp src/XlsxWriter.cpp src/Zip.cpp"

if [ "${1:-}" = "host-test" ]; then
    B=build/host
    mkdir -p "$B"
    MOC=/usr/lib/qt5/bin/moc
    QT="$(pkg-config --cflags --libs Qt5Core)"
    CORE="src/Controller.cpp src/Logic.cpp src/Platform.cpp src/BookRead.cpp src/XlsxRead.cpp src/XlsRead.cpp src/XlsxWriter.cpp src/Zip.cpp"
    $MOC src/Controller.hpp -o $B/moc_Controller.cpp
    $MOC tests/controller_test.cpp -o $B/controller_test.moc
    g++ -O1 -fPIC -Wall -Wno-deprecated-declarations -o $B/dump_book tests/dump_book.cpp src/BookRead.cpp src/XlsxRead.cpp src/XlsRead.cpp src/Zip.cpp $QT -lz
    g++ -O1 -fPIC -Wall -Wno-deprecated-declarations -I$B -Isrc -o $B/controller_test tests/controller_test.cpp $CORE $B/moc_Controller.cpp $QT -lz
    python3 tests/make_fixtures.py $B/fixtures
    python3 tests/check_readers.py $B/dump_book $B/fixtures
    rm -rf $B/sd $B/appdata
    UTIL_SD_ROOT=$HERE/$B/sd UTIL_DATA_DIR=$HERE/$B/appdata $B/controller_test $B/fixtures
    python3 tools/check_qml.py
    exit 0
fi

BBNDK="${BBNDK:-/home/user/bbsdk}"
HOST="$BBNDK/host_10_3_1_12/win32/x86"
TARGET="$BBNDK/target_10_3_1_995/qnx6"
[ -d "$HOST" ] && [ -d "$TARGET" ] || { echo "Не найден BlackBerry 10 NDK в $BBNDK"; exit 1; }

winpath() { echo "Z:$1" | tr '/' '\\'; }
export WINEDEBUG=-all
export WINEPREFIX="${WINEPREFIX:-$HOME/.wine-bb}"
export QNX_HOST="$(winpath "$HOST")"
export QNX_TARGET="$(winpath "$TARGET")"
export WINEPATH="$(winpath "$HOST/usr/bin")"
QCC="wine $HOST/usr/bin/qcc.exe"
MOC="wine $HOST/usr/bin/moc.exe"

python3 tools/check_qml.py

OUT=build/arm
mkdir -p "$OUT" dist
DEFS="-DQ_OS_BLACKBERRY -DQT_NO_IMPORT_QT47_QML -D_REENTRANT -DQT_DECLARATIVE_LIB -DQT_CORE_LIB -DQT_GUI_LIB -DQT_NO_DEBUG -D_FORTIFY_SOURCE=2"
INC="-I$TARGET/usr/include -I$TARGET/usr/include/qt4 -I$TARGET/usr/include/qt4/QtCore -I$TARGET/usr/include/qt4/QtGui -I$TARGET/usr/include/qt4/QtDeclarative -Isrc -I$OUT"
CFLAGS="-Vgcc_ntoarmv7le -lang-c++ -c -Wno-psabi -Wall -O2 -mthumb -fstack-protector-strong"

echo "== moc"
$MOC -DQ_OS_BLACKBERRY src/Controller.hpp -o $OUT/moc_Controller.cpp

OBJS=""
for f in $SOURCES $OUT/moc_Controller.cpp; do
    o="$OUT/$(basename "${f%.cpp}").o"
    echo "== qcc $f"
    $QCC $CFLAGS $DEFS $INC -o "$o" "$f"
    OBJS="$OBJS $o"
done

L="$TARGET/armle-v7"
echo "== link"
$QCC -Vgcc_ntoarmv7le -lang-c++ -Wl,-z,relro \
    -Wl,-rpath-link,$L/lib -Wl,-rpath-link,$L/usr/lib -Wl,-rpath-link,$L/usr/lib/qt4/lib \
    -L$L/lib -L$L/usr/lib -L$L/usr/lib/qt4/lib \
    -o $OUT/UtilizationKNT $OBJS \
    -lbbcascades -lbb -lbbdata -lbbsystem -lbbdevice \
    -lQtDeclarative -lQtScript -lQtSvg -lQtSql -lQtXmlPatterns -lQtXml -lQtGui -lQtNetwork -lQtCore \
    -lbps -lz -lm
wine $HOST/usr/bin/ntoarmv7-strip.exe $OUT/UtilizationKNT 2>/dev/null || true
file $OUT/UtilizationKNT || true

echo "== bar"
BAR="dist/UtilizationKNT-$VERSION.bar"
rm -f "$BAR"
bash tools/bbpkg.sh -package "$BAR" -devMode bar-descriptor.xml
ls -la "$BAR"
