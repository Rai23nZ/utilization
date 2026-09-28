/* ==========================================================================
   Утилизация КНТ — точка входа для BlackBerry 10 (Cascades, без Android Runtime).
   ========================================================================== */
#include "Controller.hpp"

#include <bb/cascades/AbstractPane>
#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>

#include <QDeclarativeContext>
#include <QTextCodec>
#include <QVariantMap>

using namespace bb::cascades;

/* Палитра «пульта» — общая для всех QML-файлов (context property T). */
static QVariantMap theme()
{
    QVariantMap t;
    t.insert("bg", "#0a0b0a");
    t.insert("panel", "#111311");
    t.insert("key", "#161816");
    t.insert("keyFn", "#111311");
    t.insert("keyDown", "#2a2e27");
    t.insert("line", "#2a2d29");
    t.insert("lineSoft", "#1d1f1c");
    t.insert("segOff", "#242622");
    t.insert("ink", "#e8e4d8");
    t.insert("ink2", "#9a978c");
    t.insert("dim", "#6b675d");
    t.insert("amber", "#ffb000");
    t.insert("amberDk", "#c98a00");
    t.insert("amberEdge", "#7a5a12");
    t.insert("amberSel", "#231d0c");
    t.insert("green", "#5bd68a");
    t.insert("red", "#ff6b5e");
    t.insert("blue", "#2a62d4");
    return t;
}

Q_DECL_EXPORT int main(int argc, char **argv)
{
    // имена файлов на карте и строки в исходниках — UTF-8
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(utf8);
    QTextCodec::setCodecForCStrings(utf8);
    QTextCodec::setCodecForTr(utf8);

    Application app(argc, argv);
    Controller *controller = new Controller(&app);

    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(&app);
    qml->setContextProperty("app", controller);
    qml->documentContext()->setContextProperty("T", QVariant(theme()));

    AbstractPane *root = qml->createRootObject<AbstractPane>();
    Application::instance()->setScene(root);
    return Application::exec();
}
