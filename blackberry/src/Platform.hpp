/* ==========================================================================
   Всё, что зависит от BlackBerry 10: пути, вибрация, буфер обмена, экран, JSON.
   На хосте (для тестов) те же функции работают через QtCore.
   ========================================================================== */
#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include <QString>
#include <QVariant>

namespace Platform {

QString sdRoot();                 // корень SD-карты
bool sdReady();                   // карта вставлена и доступна на запись
QString outDir();                 // <SD>/utilization
bool ensureOutDir();              // создаёт папку utilization, если её нет
QString dataDir();                // песочница приложения (если карты нет)

void vibrate(int ms);
bool copyText(const QString &text);
void keepAwake(bool on);

bool saveJson(const QString &path, const QVariant &value);
QVariant loadJson(const QString &path, bool *ok);

bool writeText(const QString &path, const QString &text);   // UTF-8 с BOM, строки CRLF

}

#endif
