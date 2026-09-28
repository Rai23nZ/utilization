#include "Platform.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#ifdef Q_OS_BLACKBERRY
#include <bb/cascades/Application>
#include <bb/cascades/ScreenIdleMode>
#include <bb/cascades/Window>
#include <bb/data/JsonDataAccess>
#include <bb/device/VibrationController>
#include <bb/system/Clipboard>
#else
#include <QJsonDocument>
#include <stdlib.h>
#endif

#define QS(s) QString::fromUtf8(s)

namespace Platform {

QString sdRoot()
{
#ifdef Q_OS_BLACKBERRY
    return QS("/accounts/1000/removable/sdcard");
#else
    QByteArray env = qgetenv("UTIL_SD_ROOT");
    return env.isEmpty() ? QS("/tmp/util-sd") : QString::fromUtf8(env);
#endif
}

bool sdReady()
{
    QFileInfo fi(sdRoot());
    return fi.exists() && fi.isDir() && fi.isWritable();
}

QString outDir()
{
    return sdRoot() + QS("/utilization");
}

bool ensureOutDir()
{
    if (!sdReady()) return false;
    QDir dir(outDir());
    if (dir.exists()) return true;
    return QDir().mkpath(outDir());
}

QString dataDir()
{
#ifdef Q_OS_BLACKBERRY
    return QDir::homePath();   // песочница: /accounts/1000/appdata/<app>/data
#else
    QByteArray env = qgetenv("UTIL_DATA_DIR");
    QString d = env.isEmpty() ? QS("/tmp/util-data") : QString::fromUtf8(env);
    QDir().mkpath(d);
    return d;
#endif
}

void vibrate(int ms)
{
#ifdef Q_OS_BLACKBERRY
    static bb::device::VibrationController *vc = 0;
    if (!vc) vc = new bb::device::VibrationController();
    if (vc->isSupported()) vc->start(80, ms);
#else
    Q_UNUSED(ms);
#endif
}

bool copyText(const QString &text)
{
#ifdef Q_OS_BLACKBERRY
    bb::system::Clipboard cb;
    cb.clear();
    return cb.insert(QS("text/plain"), text.toUtf8());
#else
    Q_UNUSED(text);
    return true;
#endif
}

void keepAwake(bool on)
{
#ifdef Q_OS_BLACKBERRY
    bb::cascades::Application *app = bb::cascades::Application::instance();
    if (app && app->mainWindow())
        app->mainWindow()->setScreenIdleMode(on ? bb::cascades::ScreenIdleMode::KeepAwake
                                                : bb::cascades::ScreenIdleMode::Normal);
#else
    Q_UNUSED(on);
#endif
}

bool saveJson(const QString &path, const QVariant &value)
{
    QString tmp = path + QS(".tmp");
#ifdef Q_OS_BLACKBERRY
    bb::data::JsonDataAccess jda;
    jda.save(value, tmp);
    if (jda.hasError()) return false;
#else
    QFile f(tmp);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact));
    f.close();
#endif
    // замена «через временный файл», чтобы сбой питания не оставил половину сессии
    QFile::remove(path);
    return QFile::rename(tmp, path);
}

QVariant loadJson(const QString &path, bool *ok)
{
    if (ok) *ok = false;
    if (!QFileInfo(path).exists()) return QVariant();
#ifdef Q_OS_BLACKBERRY
    bb::data::JsonDataAccess jda;
    QVariant v = jda.load(path);
    if (jda.hasError()) return QVariant();
#else
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QVariant();
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (doc.isNull()) return QVariant();
    QVariant v = doc.toVariant();
#endif
    if (ok) *ok = true;
    return v;
}

bool writeText(const QString &path, const QString &text)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QString crlf = text;
    crlf.replace(QS("\r\n"), QS("\n")).replace(QS("\n"), QS("\r\n"));
    f.write("\xEF\xBB\xBF");
    f.write(crlf.toUtf8());
    f.close();
    return true;
}

}  // namespace Platform
