/* ==========================================================================
   Старый .xls: контейнер OLE2 (Compound File) + записи BIFF8 (Excel 97–2003)
   и BIFF5/7 (Excel 5/95). Нужен только первый рабочий лист и значения ячеек.
   ========================================================================== */
#include "Book.hpp"

#include <QDateTime>
#include <QHash>
#include <QStringList>
#include <QTextCodec>

#include <string.h>

using namespace BookUtil;

static quint16 u16(const QByteArray &d, int p)
{
    if (p < 0 || p + 2 > d.size()) return 0;
    const uchar *b = (const uchar *)d.constData() + p;
    return (quint16)(b[0] | (b[1] << 8));
}

static quint32 u32(const QByteArray &d, int p)
{
    if (p < 0 || p + 4 > d.size()) return 0;
    const uchar *b = (const uchar *)d.constData() + p;
    return (quint32)b[0] | ((quint32)b[1] << 8) | ((quint32)b[2] << 16) | ((quint32)b[3] << 24);
}

static double f64(const QByteArray &d, int p)
{
    double v = 0;
    if (p < 0 || p + 8 > d.size()) return 0;
    // BIFF хранит double в little-endian; ARM на BlackBerry тоже little-endian
    memcpy(&v, d.constData() + p, 8);
    return v;
}

/* ------------------------------------------------------------ Compound File */

namespace {

const quint32 ENDOFCHAIN = 0xFFFFFFFE;
const quint32 FREESECT = 0xFFFFFFFF;

class Cfb {
public:
    bool open(const QByteArray &data, QString &error);
    bool stream(const QString &name, QByteArray &out) const;

private:
    QByteArray chain(quint32 start, qint64 size) const;
    QByteArray sector(quint32 n) const;

    struct Dir { QString name; int type; quint32 start; quint64 size; };
    QByteArray m_data;
    int m_secSize, m_miniSize;
    quint32 m_miniCutoff;
    QList<quint32> m_fat, m_miniFat;
    QList<Dir> m_dirs;
    QByteArray m_miniStream;
};

QByteArray Cfb::sector(quint32 n) const
{
    qint64 off = (qint64)(n + 1) * m_secSize;
    if (off + m_secSize > m_data.size()) {
        if (off >= m_data.size()) return QByteArray();
        QByteArray part = m_data.mid((int)off);
        part.append(QByteArray(m_secSize - part.size(), '\0'));
        return part;
    }
    return m_data.mid((int)off, m_secSize);
}

QByteArray Cfb::chain(quint32 start, qint64 size) const
{
    QByteArray out;
    quint32 cur = start;
    int guard = 0;
    while (cur != ENDOFCHAIN && cur != FREESECT && (int)cur < m_fat.size() && guard < m_fat.size() + 1) {
        out.append(sector(cur));
        cur = m_fat.at(cur);
        ++guard;
        if (size >= 0 && out.size() >= size) break;
    }
    if (size >= 0 && out.size() > size) out.truncate((int)size);
    return out;
}

bool Cfb::open(const QByteArray &data, QString &error)
{
    m_data = data;
    if (data.size() < 512) { error = QS("Файл .xls повреждён"); return false; }
    int shift = u16(data, 0x1E);
    int miniShift = u16(data, 0x20);
    if (shift < 7 || shift > 16 || miniShift < 2 || miniShift > shift) { error = QS("Файл .xls повреждён"); return false; }
    m_secSize = 1 << shift;
    m_miniSize = 1 << miniShift;
    quint32 numFat = u32(data, 0x2C);
    quint32 firstDir = u32(data, 0x30);
    m_miniCutoff = u32(data, 0x38);
    quint32 firstMiniFat = u32(data, 0x3C);
    quint32 numMiniFat = u32(data, 0x40);
    quint32 firstDifat = u32(data, 0x44);
    quint32 numDifat = u32(data, 0x48);

    // номера секторов FAT: 109 в заголовке + цепочка DIFAT
    QList<quint32> fatSectors;
    for (int i = 0; i < 109 && (quint32)fatSectors.size() < numFat; ++i) {
        quint32 s = u32(data, 0x4C + i * 4);
        if (s == FREESECT) break;
        fatSectors << s;
    }
    quint32 difat = firstDifat;
    int perDifat = m_secSize / 4 - 1;
    for (quint32 k = 0; k < numDifat && difat != ENDOFCHAIN && difat != FREESECT; ++k) {
        QByteArray sec = sector(difat);
        if (sec.isEmpty()) break;
        for (int i = 0; i < perDifat && (quint32)fatSectors.size() < numFat; ++i) {
            quint32 s = u32(sec, i * 4);
            if (s == FREESECT) continue;
            fatSectors << s;
        }
        difat = u32(sec, perDifat * 4);
    }
    for (int i = 0; i < fatSectors.size(); ++i) {
        QByteArray sec = sector(fatSectors.at(i));
        for (int j = 0; j + 4 <= sec.size(); j += 4) m_fat << u32(sec, j);
    }
    if (m_fat.isEmpty()) { error = QS("Файл .xls повреждён: нет таблицы размещения"); return false; }

    // каталог
    QByteArray dir = chain(firstDir, -1);
    for (int p = 0; p + 128 <= dir.size(); p += 128) {
        Dir d;
        int nameLen = u16(dir, p + 0x40);
        QString name;
        for (int i = 0; i + 2 <= nameLen - 2 && i < 64; i += 2) name.append(QChar(u16(dir, p + i)));
        d.name = name;
        d.type = (uchar)dir.at(p + 0x42);
        d.start = u32(dir, p + 0x74);
        d.size = u32(dir, p + 0x78);
        if (m_secSize == 4096) d.size |= ((quint64)u32(dir, p + 0x7C) << 32);
        m_dirs << d;
    }
    if (m_dirs.isEmpty() || m_dirs.at(0).type != 5) { error = QS("Файл .xls повреждён: нет корневого каталога"); return false; }

    // мини-поток и мини-FAT
    m_miniStream = chain(m_dirs.at(0).start, (qint64)m_dirs.at(0).size);
    if (numMiniFat > 0 && firstMiniFat != ENDOFCHAIN) {
        QByteArray mf = chain(firstMiniFat, -1);
        for (int j = 0; j + 4 <= mf.size(); j += 4) m_miniFat << u32(mf, j);
    }
    return true;
}

bool Cfb::stream(const QString &name, QByteArray &out) const
{
    for (int i = 0; i < m_dirs.size(); ++i) {
        const Dir &d = m_dirs.at(i);
        if (d.type != 2 || d.name.compare(name, Qt::CaseInsensitive) != 0) continue;
        if (d.size < m_miniCutoff) {
            out.clear();
            quint32 cur = d.start;
            int guard = 0;
            while (cur != ENDOFCHAIN && cur != FREESECT && (int)cur < m_miniFat.size() &&
                   guard <= m_miniFat.size() && (quint64)out.size() < d.size) {
                out.append(m_miniStream.mid((int)(cur * m_miniSize), m_miniSize));
                cur = m_miniFat.at(cur);
                ++guard;
            }
            out.truncate((int)qMin((quint64)out.size(), d.size));
        } else {
            out = chain(d.start, (qint64)d.size);
        }
        return true;
    }
    return false;
}

/* --------------------------------------------------------------- BIFF-записи */

struct Record { quint16 type; int pos; int len; };
struct SheetRef { quint32 offset; QString name; int type; };

/* Несколько сегментов данных (запись + её CONTINUE): строки SST могут
   разрываться между ними, и в начале продолжения идёт новый байт флагов. */
class Segments {
public:
    QList<QByteArray> parts;
    int seg, pos;
    Segments() : seg(0), pos(0) {}

    bool atEnd() const { return seg >= parts.size(); }
    void norm() { while (seg < parts.size() && pos >= parts.at(seg).size()) { ++seg; pos = 0; } }
    int avail() const { return seg < parts.size() ? parts.at(seg).size() - pos : 0; }
    uchar byte() { norm(); if (atEnd()) return 0; return (uchar)parts.at(seg).at(pos++); }
    quint16 word() { quint16 lo = byte(); quint16 hi = byte(); return (quint16)(lo | (hi << 8)); }
    quint32 dword() { quint32 a = word(); quint32 b = word(); return a | (b << 16); }
    void skip(qint64 n)
    {
        while (n > 0) {
            norm();
            if (atEnd()) return;
            int take = (int)qMin<qint64>(n, avail());
            pos += take;
            n -= take;
        }
    }
    QString chars(int count, bool high)
    {
        QString out;
        out.reserve(count);
        while (count > 0) {
            norm();
            if (atEnd()) break;
            int bpc = high ? 2 : 1;
            int n = qMin(count, avail() / bpc);
            const QByteArray &p = parts.at(seg);
            for (int i = 0; i < n; ++i) {
                if (high) out.append(QChar((ushort)((uchar)p.at(pos) | ((uchar)p.at(pos + 1) << 8))));
                else out.append(QChar((ushort)(uchar)p.at(pos)));
                pos += bpc;
            }
            count -= n;
            if (count > 0) {
                // символы продолжаются в следующей записи CONTINUE — с новым флагом
                ++seg; pos = 0;
                if (atEnd()) break;
                high = (byte() & 0x01) != 0;
            }
        }
        return out;
    }
};

class Biff {
public:
    Biff(const QByteArray &s) : stream(s), biff8(true), date1904(false), codec(0) {}

    bool parse(Book &book, QString &error);

private:
    bool readRecord(int &p, Record &r) const
    {
        if (p + 4 > stream.size()) return false;
        r.type = u16(stream, p);
        r.len = u16(stream, p + 2);
        r.pos = p + 4;
        if (r.pos + r.len > stream.size()) r.len = stream.size() - r.pos;
        p = r.pos + r.len;
        return true;
    }
    QByteArray body(const Record &r) const { return stream.mid(r.pos, r.len); }

    /* строка BIFF8 XLUnicodeString (cch — 1 или 2 байта) или BIFF5 (байты в кодовой странице) */
    QString unicodeString(const QByteArray &d, int p, bool wideLen, int *used = 0) const
    {
        int cch = wideLen ? u16(d, p) : (uchar)d.at(p);
        int q = p + (wideLen ? 2 : 1);
        if (!biff8) {
            QByteArray raw = d.mid(q, cch);
            if (used) *used = q + cch - p;
            return codec ? codec->toUnicode(raw) : QString::fromLatin1(raw);
        }
        uchar flags = (uchar)d.at(q);
        ++q;
        bool high = flags & 0x01;
        int runs = 0, ext = 0;
        if (flags & 0x08) { runs = u16(d, q); q += 2; }
        if (flags & 0x04) { ext = (int)u32(d, q); q += 4; }
        QString out;
        out.reserve(cch);
        for (int i = 0; i < cch; ++i) {
            if (high) { out.append(QChar(u16(d, q))); q += 2; }
            else { out.append(QChar((ushort)(uchar)d.at(q))); q += 1; }
        }
        q += runs * 4 + ext;
        if (used) *used = q - p;
        return out;
    }

    QVariant number(double v, int xf) const
    {
        if (xf >= 0 && xf < xfDate.size() && xfDate.at(xf)) return excelDate(v, date1904);
        return v;
    }

    static double rk(quint32 raw)
    {
        double v;
        if (raw & 0x02) {
            qint32 i = (qint32)raw;
            v = (double)(i >> 2);
        } else {
            quint64 bits = ((quint64)(raw & 0xFFFFFFFC)) << 32;
            memcpy(&v, &bits, 8);
        }
        if (raw & 0x01) v /= 100.0;
        return v;
    }

    const QByteArray &stream;
    bool biff8, date1904;
    QTextCodec *codec;
    QStringList sst;
    QList<bool> xfDate;
};

bool Biff::parse(Book &book, QString &error)
{
    int p = 0;
    Record r;
    if (!readRecord(p, r) || (r.type != 0x0809 && r.type != 0x0409 && r.type != 0x0209)) {
        error = QS("Это не книга Excel (нет заголовка BIFF)");
        return false;
    }
    int version = u16(stream, r.pos);
    if (r.type != 0x0809 || (version != 0x0600 && version != 0x0500)) {
        error = QS("Слишком старая версия Excel (до Excel 5). Пересохраните файл как .xls 97–2003 или .xlsx");
        return false;
    }
    biff8 = (version == 0x0600);
    codec = QTextCodec::codecForName(biff8 ? "UTF-16LE" : "Windows-1252");

    // глобальная часть книги
    QList<SheetRef> sheets;
    QHash<int, QString> formats;
    QList<int> xfFormats;
    while (readRecord(p, r)) {
        if (r.type == 0x000A) break;   // EOF
        QByteArray d = body(r);
        switch (r.type) {
        case 0x002F:   // FILEPASS
            error = QS("Файл защищён паролем — снимите защиту в Excel и сохраните заново");
            return false;
        case 0x0042: { // CODEPAGE
            int cp = u16(d, 0);
            QTextCodec *c = 0;
            if (cp == 1200) c = QTextCodec::codecForName("UTF-16LE");
            else if (cp == 1251) c = QTextCodec::codecForName("Windows-1251");
            else if (cp == 866) c = QTextCodec::codecForName("IBM 866");
            else if (cp == 10007) c = QTextCodec::codecForName("x-mac-cyrillic");
            else if (cp == 1252 || cp == 0x8001 || cp == 367) c = QTextCodec::codecForName("Windows-1252");
            else if (cp >= 1250 && cp <= 1258)
                c = QTextCodec::codecForName(QString(QS("Windows-%1")).arg(cp).toLatin1());
            if (c) codec = c;
            break;
        }
        case 0x0022:   // DATEMODE
            date1904 = u16(d, 0) == 1;
            break;
        case 0x0085: { // BOUNDSHEET
            SheetRef s;
            s.offset = u32(d, 0);
            s.type = (uchar)d.at(5);
            s.name = unicodeString(d, 6, false);
            sheets << s;
            break;
        }
        case 0x041E:   // FORMAT (BIFF5/8)
        case 0x001E: {
            int id = u16(d, 0);
            formats.insert(id, unicodeString(d, 2, biff8));
            break;
        }
        case 0x00E0:   // XF
            xfFormats << u16(d, 2);
            break;
        case 0x00FC: { // SST + CONTINUE
            Segments seg;
            seg.parts << d.mid(8);
            int q = p;
            Record c;
            while (readRecord(q, c) && c.type == 0x003C) { seg.parts << body(c); p = q; }
            quint32 unique = u32(d, 4);
            for (quint32 i = 0; i < unique && !seg.atEnd(); ++i) {
                int cch = seg.word();
                uchar flags = seg.byte();
                int runs = 0;
                quint32 ext = 0;
                if (flags & 0x08) runs = seg.word();
                if (flags & 0x04) ext = seg.dword();
                sst << seg.chars(cch, flags & 0x01);
                seg.skip((qint64)runs * 4 + ext);
            }
            break;
        }
        default:
            break;
        }
    }

    for (int i = 0; i < xfFormats.size(); ++i) {
        int id = xfFormats.at(i);
        xfDate << (formats.contains(id) ? isDateFormatCode(formats.value(id)) : isDateFormatId(id));
    }

    int sheetIdx = -1;
    for (int i = 0; i < sheets.size(); ++i) if (sheets.at(i).type == 0) { sheetIdx = i; break; }
    if (sheetIdx < 0) { error = QS("В книге нет ни одного листа"); return false; }
    book.sheet = sheets.at(sheetIdx).name;

    // первый лист
    p = (int)sheets.at(sheetIdx).offset;
    if (!readRecord(p, r) || r.type != 0x0809) { error = QS("Лист книги повреждён"); return false; }

    QList<Cell> cells;
    int firstCol = 0;
    int pendingRow = -1, pendingCol = -1, pendingXf = -1;   // FORMULA со строковым результатом
    while (readRecord(p, r)) {
        if (r.type == 0x000A) break;
        const QByteArray d = body(r);
        Cell c;
        switch (r.type) {
        case 0x0200:   // DIMENSION
            firstCol = biff8 ? u16(d, 8) : u16(d, 4);
            break;
        case 0x00FD: { // LABELSST
            quint32 idx = u32(d, 6);
            c.row = u16(d, 0); c.col = u16(d, 2);
            c.value = (int)idx < sst.size() ? QVariant(sst.at(idx)) : QVariant();
            cells << c;
            break;
        }
        case 0x0204:   // LABEL
        case 0x00D6: { // RSTRING
            c.row = u16(d, 0); c.col = u16(d, 2);
            c.value = unicodeString(d, 6, true);
            cells << c;
            break;
        }
        case 0x0203:   // NUMBER
            c.row = u16(d, 0); c.col = u16(d, 2);
            c.value = number(f64(d, 6), u16(d, 4));
            cells << c;
            break;
        case 0x027E:   // RK
            c.row = u16(d, 0); c.col = u16(d, 2);
            c.value = number(rk(u32(d, 6)), u16(d, 4));
            cells << c;
            break;
        case 0x00BD: { // MULRK
            int row = u16(d, 0), col = u16(d, 2);
            int n = (d.size() - 6) / 6;
            for (int i = 0; i < n; ++i) {
                Cell m;
                m.row = row; m.col = col + i;
                m.value = number(rk(u32(d, 4 + i * 6 + 2)), u16(d, 4 + i * 6));
                cells << m;
            }
            break;
        }
        case 0x0205: { // BOOLERR
            c.row = u16(d, 0); c.col = u16(d, 2);
            uchar val = (uchar)d.at(6), isErr = (uchar)d.at(7);
            if (!isErr) { c.value = (bool)val; cells << c; }
            break;
        }
        case 0x0006: { // FORMULA
            int row = u16(d, 0), col = u16(d, 2), xf = u16(d, 4);
            if ((uchar)d.at(12) == 0xFF && (uchar)d.at(13) == 0xFF) {
                uchar kind = (uchar)d.at(6);
                if (kind == 0) { pendingRow = row; pendingCol = col; pendingXf = xf; }
                else if (kind == 1) { c.row = row; c.col = col; c.value = (bool)d.at(8); cells << c; }
            } else {
                c.row = row; c.col = col; c.value = number(f64(d, 6), xf);
                cells << c;
            }
            break;
        }
        case 0x0207: { // STRING — результат предыдущей формулы
            if (pendingRow >= 0) {
                c.row = pendingRow; c.col = pendingCol;
                c.value = unicodeString(d, 0, true);
                cells << c;
                pendingRow = -1;
            }
            break;
        }
        default:
            break;
        }
    }
    Q_UNUSED(pendingXf);

    book.rows = assemble(cells, firstCol);
    return true;
}

}  // namespace

bool readXls(const QByteArray &data, Book &book, QString &error)
{
    Cfb cfb;
    if (!cfb.open(data, error)) return false;
    QByteArray stream;
    if (!cfb.stream(QS("Workbook"), stream) && !cfb.stream(QS("Book"), stream)) {
        error = QS("Это не книга Excel: в файле нет потока Workbook (возможно, это документ Word)");
        return false;
    }
    Biff biff(stream);
    return biff.parse(book, error);
}
