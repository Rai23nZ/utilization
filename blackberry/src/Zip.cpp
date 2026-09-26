#include "Zip.hpp"

#include <QDateTime>
#include <string.h>
#include <zlib.h>

#define QS(s) QString::fromUtf8(s)

static quint16 rd16(const QByteArray &d, int p)
{
    const uchar *b = (const uchar *)d.constData() + p;
    return (quint16)(b[0] | (b[1] << 8));
}

static quint32 rd32(const QByteArray &d, int p)
{
    const uchar *b = (const uchar *)d.constData() + p;
    return (quint32)b[0] | ((quint32)b[1] << 8) | ((quint32)b[2] << 16) | ((quint32)b[3] << 24);
}

static void wr16(QByteArray &d, quint16 v)
{
    d.append((char)(v & 0xFF));
    d.append((char)((v >> 8) & 0xFF));
}

static void wr32(QByteArray &d, quint32 v)
{
    for (int i = 0; i < 4; ++i) d.append((char)((v >> (8 * i)) & 0xFF));
}

/* ------------------------------------------------------------------- чтение */

bool ZipReader::open(const QByteArray &data, QString &error)
{
    m_data = data;
    m_entries.clear();
    const int n = data.size();
    if (n < 22) { error = QS("Архив повреждён"); return false; }

    // End of central directory: ищем с конца (комментарий — до 64 КБ)
    int eocd = -1;
    for (int p = n - 22; p >= 0 && p >= n - 22 - 65535; --p) {
        if (rd32(data, p) == 0x06054b50) { eocd = p; break; }
    }
    if (eocd < 0) { error = QS("Архив повреждён: нет оглавления"); return false; }

    int count = rd16(data, eocd + 10);
    quint32 cdOffset = rd32(data, eocd + 16);
    int p = (int)cdOffset;
    for (int i = 0; i < count; ++i) {
        if (p + 46 > n || rd32(data, p) != 0x02014b50) { error = QS("Архив повреждён: оглавление"); return false; }
        quint16 flags = rd16(data, p + 8);
        Entry e;
        e.method = rd16(data, p + 10);
        e.compSize = rd32(data, p + 20);
        e.size = rd32(data, p + 24);
        int nameLen = rd16(data, p + 28), extraLen = rd16(data, p + 30), commentLen = rd16(data, p + 32);
        e.offset = rd32(data, p + 42);
        QByteArray rawName = data.mid(p + 46, nameLen);
        e.name = (flags & 0x0800) ? QString::fromUtf8(rawName) : QString::fromLatin1(rawName);
        m_entries.append(e);
        p += 46 + nameLen + extraLen + commentLen;
    }
    return true;
}

QStringList ZipReader::names() const
{
    QStringList out;
    for (int i = 0; i < m_entries.size(); ++i) out << m_entries.at(i).name;
    return out;
}

bool ZipReader::contains(const QString &name) const
{
    return !resolve(name).isEmpty();
}

QString ZipReader::resolve(const QString &name) const
{
    QString want = name;
    while (want.startsWith(QChar('/'))) want = want.mid(1);
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries.at(i).name == want) return want;
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries.at(i).name.compare(want, Qt::CaseInsensitive) == 0) return m_entries.at(i).name;
    return QString();
}

QByteArray ZipReader::file(const QString &name, bool *ok) const
{
    if (ok) *ok = false;
    QString real = resolve(name);
    if (real.isEmpty()) return QByteArray();
    const Entry *e = 0;
    for (int i = 0; i < m_entries.size(); ++i) if (m_entries.at(i).name == real) { e = &m_entries.at(i); break; }
    if (!e) return QByteArray();

    int p = (int)e->offset;
    if (p + 30 > m_data.size() || rd32(m_data, p) != 0x04034b50) return QByteArray();
    int start = p + 30 + rd16(m_data, p + 26) + rd16(m_data, p + 28);
    if (start + (qint64)e->compSize > m_data.size()) return QByteArray();

    if (e->method == 0) {
        if (ok) *ok = true;
        return m_data.mid(start, e->compSize);
    }
    if (e->method != 8) return QByteArray();

    QByteArray out;
    out.resize(e->size > 0 ? (int)e->size : 65536);
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) return QByteArray();
    zs.next_in = (Bytef *)(m_data.constData() + start);
    zs.avail_in = e->compSize;
    int produced = 0, rc = Z_OK;
    for (;;) {
        if (produced >= out.size()) out.resize(out.size() * 2);
        zs.next_out = (Bytef *)(out.data() + produced);
        zs.avail_out = out.size() - produced;
        rc = inflate(&zs, Z_NO_FLUSH);
        produced = out.size() - zs.avail_out;
        if (rc == Z_STREAM_END) break;
        if (rc != Z_OK) break;
        if (zs.avail_in == 0 && zs.avail_out > 0) break;
    }
    inflateEnd(&zs);
    if (rc != Z_STREAM_END && rc != Z_OK) return QByteArray();
    out.resize(produced);
    if (ok) *ok = true;
    return out;
}

/* ------------------------------------------------------------------- запись */

void ZipWriter::add(const QString &name, const QByteArray &content)
{
    Entry e;
    e.name = name.toUtf8();
    e.size = content.size();
    e.crc = crc32(0L, (const Bytef *)content.constData(), content.size());
    e.offset = m_out.size();

    QByteArray comp;
    comp.resize(compressBound(content.size()) + 64);
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
    zs.next_in = (Bytef *)content.constData();
    zs.avail_in = content.size();
    zs.next_out = (Bytef *)comp.data();
    zs.avail_out = comp.size();
    deflate(&zs, Z_FINISH);
    comp.resize(comp.size() - zs.avail_out);
    deflateEnd(&zs);
    e.compSize = comp.size();

    QDateTime now = QDateTime::currentDateTime();
    quint16 dosTime = (quint16)((now.time().hour() << 11) | (now.time().minute() << 5) | (now.time().second() / 2));
    quint16 dosDate = (quint16)(((now.date().year() - 1980) << 9) | (now.date().month() << 5) | now.date().day());

    wr32(m_out, 0x04034b50);
    wr16(m_out, 20);          // версия для распаковки
    wr16(m_out, 0x0800);      // имена в UTF-8
    wr16(m_out, 8);           // deflate
    wr16(m_out, dosTime);
    wr16(m_out, dosDate);
    wr32(m_out, e.crc);
    wr32(m_out, e.compSize);
    wr32(m_out, e.size);
    wr16(m_out, e.name.size());
    wr16(m_out, 0);
    m_out.append(e.name);
    m_out.append(comp);
    m_entries.append(e);
}

QByteArray ZipWriter::finish()
{
    QDateTime now = QDateTime::currentDateTime();
    quint16 dosTime = (quint16)((now.time().hour() << 11) | (now.time().minute() << 5) | (now.time().second() / 2));
    quint16 dosDate = (quint16)(((now.date().year() - 1980) << 9) | (now.date().month() << 5) | now.date().day());

    quint32 cdStart = m_out.size();
    for (int i = 0; i < m_entries.size(); ++i) {
        const Entry &e = m_entries.at(i);
        wr32(m_out, 0x02014b50);
        wr16(m_out, 20);
        wr16(m_out, 20);
        wr16(m_out, 0x0800);
        wr16(m_out, 8);
        wr16(m_out, dosTime);
        wr16(m_out, dosDate);
        wr32(m_out, e.crc);
        wr32(m_out, e.compSize);
        wr32(m_out, e.size);
        wr16(m_out, e.name.size());
        wr16(m_out, 0);
        wr16(m_out, 0);
        wr16(m_out, 0);
        wr16(m_out, 0);
        wr32(m_out, 0);
        wr32(m_out, e.offset);
        m_out.append(e.name);
    }
    quint32 cdSize = m_out.size() - cdStart;
    wr32(m_out, 0x06054b50);
    wr16(m_out, 0);
    wr16(m_out, 0);
    wr16(m_out, m_entries.size());
    wr16(m_out, m_entries.size());
    wr32(m_out, cdSize);
    wr32(m_out, cdStart);
    wr16(m_out, 0);
    return m_out;
}
