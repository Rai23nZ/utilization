/* ==========================================================================
   Минимальный ZIP: чтение (stored/deflate) для .xlsx и запись для выгрузки.
   Работает целиком в памяти — файлы здесь небольшие.
   ========================================================================== */
#ifndef ZIP_HPP
#define ZIP_HPP

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

class ZipReader {
public:
    bool open(const QByteArray &data, QString &error);
    QStringList names() const;
    bool contains(const QString &name) const;
    /* Имя с учётом регистра; если нет — ищет без учёта регистра. */
    QString resolve(const QString &name) const;
    QByteArray file(const QString &name, bool *ok = 0) const;

private:
    struct Entry { QString name; int method; quint32 compSize; quint32 size; quint32 offset; };
    QByteArray m_data;
    QList<Entry> m_entries;
};

class ZipWriter {
public:
    void add(const QString &name, const QByteArray &content);
    QByteArray finish();

private:
    struct Entry { QByteArray name; quint32 crc; quint32 compSize; quint32 size; quint32 offset; };
    QByteArray m_out;
    QList<Entry> m_entries;
};

#endif
