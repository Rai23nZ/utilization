#!/bin/sh
# blackberry-nativepackager из BlackBerry 10 NDK на современной Java (17+):
# разрешаем SecurityManager и доступ к внутреннему XML-парсеру, которым пользуется упаковщик.
BBNDK="${BBNDK:-/home/user/bbsdk}"
L="$BBNDK/host_10_3_1_12/win32/x86/usr/lib"
CP="$L/EccpressoAll.jar:$L/EccpressoJDK15ECC.jar:$L/TrustpointAll.jar:$L/TrustpointJDK15.jar:$L/TrustpointProviders.jar:$L/BarSigner.jar:$L/BarPackager.jar:$L/KeyTool.jar:$L/BarDeploy.jar:$L/BarAir.jar"
exec java -Djava.security.manager=allow -Djava.awt.headless=true -Xmx512M \
    --add-exports java.xml/com.sun.org.apache.xerces.internal.parsers=ALL-UNNAMED \
    --add-exports java.xml/com.sun.org.apache.xerces.internal.dom=ALL-UNNAMED \
    --add-exports java.xml/com.sun.org.apache.xml.internal.serialize=ALL-UNNAMED \
    --add-opens java.base/java.lang=ALL-UNNAMED \
    -cp "$CP" com.qnx.bbt.nativepackager.BarNativePackager "$@" 2>&1 | grep -vE 'JAVA_TOOL_OPTIONS|^WARNING'
