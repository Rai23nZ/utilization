/* ==========================================================================
   Общие функции чтения таблиц, выбор формата, CSV, XML 2003 и HTML-таблицы.
   ========================================================================== */
#include "Book.hpp"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegExp>
#include <QTextCodec>
#include <QXmlStreamReader>

#include <math.h>

namespace BookUtil {

static QString pad2(int n) { return (n < 10 ? QS("0") : QString()) + QString::number(n); }

QString cellText(const QVariant &v)
{
    if (!v.isValid() || v.isNull()) return QString();
    switch (v.type()) {
    case QVariant::DateTime:
    case QVariant::Date: {
        QDate d = v.type() == QVariant::Date ? v.toDate() : v.toDateTime().date();
        return pad2(d.day()) + QS(".") + pad2(d.month()) + QS(".") + QString::number(d.year());
    }
    case QVariant::Double:
    case QVariant::Int:
    case QVariant::LongLong:
    case QVariant::UInt:
    case QVariant::ULongLong: {
        double x = v.toDouble();
        if (x != x) return QS("NaN");
        if (x == floor(x) && fabs(x) < 9.0e15) return QString::number((qlonglong)x);
        if (x == floor(x)) return QString::number(x, 'f', 0);
        double r = floor(x * 100.0 + 0.5) / 100.0;   // Math.round(v * 100) / 100
        if (r == floor(r) && fabs(r) < 9.0e15) return QString::number((qlonglong)r);
        return QString::number(r, 'g', 15);
    }
    case QVariant::Bool:
        return v.toBool() ? QS("true") : QS("false");
    default: {
        QString s = v.toString();
        s.replace(QChar(0x00A0), QChar(' '));
        return s.trimmed();
    }
    }
}

QString digits(const QString &s)
{
    QString out;
    out.reserve(s.size());
    for (int i = 0; i < s.size(); ++i) {
        ushort c = s.at(i).unicode();
        if (c >= '0' && c <= '9') out.append(s.at(i));
    }
    return out;
}

QString normHeader(const QVariant &v)
{
    QString s = v.isValid() ? v.toString() : QString();
    s.replace(QChar(0x00A0), QChar(' '));
    s = s.trimmed().toLower();
    s.replace(QChar(0x0451), QChar(0x0435));          // ё → е
    s = s.simplified();
    while (!s.isEmpty() && (s.endsWith(QChar(':')) || s.endsWith(QChar('.')))) s.chop(1);
    return s;
}

QDateTime excelDate(double serial, bool date1904)
{
    qint64 days = (qint64)floor(serial);
    double frac = serial - floor(serial);
    QDate base;
    if (date1904) {
        base = QDate(1904, 1, 1);
    } else if (days < 60) {
        base = QDate(1899, 12, 31);   // до несуществующего 29.02.1900
    } else {
        base = QDate(1899, 12, 30);
    }
    QDate d = base.addDays(days);
    int secs = (int)floor(frac * 86400.0 + 0.5);
    if (secs >= 86400) { d = d.addDays(1); secs -= 86400; }
    return QDateTime(d, QTime(0, 0, 0).addSecs(secs));
}

bool isDateFormatId(int id)
{
    return (id >= 14 && id <= 22) || (id >= 27 && id <= 36) || (id >= 45 && id <= 47) ||
           (id >= 50 && id <= 58);
}

bool isDateFormatCode(const QString &code)
{
    if (code.isEmpty()) return false;
    if (code.compare(QS("General"), Qt::CaseInsensitive) == 0) return false;
    QString s;
    bool inQuote = false;
    for (int i = 0; i < code.size(); ++i) {
        QChar c = code.at(i);
        if (inQuote) { if (c == QChar('"')) inQuote = false; continue; }
        if (c == QChar('"')) { inQuote = true; continue; }
        if (c == QChar('\\') || c == QChar('_') || c == QChar('*')) { ++i; continue; }
        if (c == QChar('[')) {
            int end = code.indexOf(QChar(']'), i);
            if (end < 0) break;
            QString inner = code.mid(i + 1, end - i - 1).toLower();
            if (inner == QS("h") || inner == QS("hh") || inner == QS("m") || inner == QS("mm") ||
                inner == QS("s") || inner == QS("ss"))
                s.append(QChar('h'));
            i = end;
            continue;
        }
        s.append(c);
    }
    // только первая секция формата (положительные числа)
    int semi = s.indexOf(QChar(';'));
    if (semi >= 0) s = s.left(semi);
    QString low = s.toLower();
    if (low.contains(QChar('e')) && !low.contains(QChar('d')) && !low.contains(QChar('y'))) return false;
    for (int i = 0; i < low.size(); ++i) {
        ushort c = low.at(i).unicode();
        if (c == 'd' || c == 'y' || c == 'h' || c == 's' || c == 'm') return true;
    }
    return false;
}

Rows assemble(const QList<Cell> &cells, int firstCol)
{
    Rows out;
    if (cells.isEmpty()) return out;
    int maxCol = -1;
    QMap<int, QMap<int, QVariant> > grid;
    for (int i = 0; i < cells.size(); ++i) {
        const Cell &c = cells.at(i);
        // пустые ячейки не нужны: строки из одних пустых ячеек выпадают (blankrows: false)
        if (c.col < firstCol || !c.value.isValid() || cellText(c.value).isEmpty()) continue;
        grid[c.row][c.col] = c.value;
        if (c.col > maxCol) maxCol = c.col;
    }
    if (maxCol < firstCol) return out;
    int width = maxCol - firstCol + 1;
    QMap<int, QMap<int, QVariant> >::const_iterator it = grid.constBegin();
    for (; it != grid.constEnd(); ++it) {
        QVariantList row;
        for (int c = 0; c < width; ++c) row.append(QVariant());
        QMap<int, QVariant>::const_iterator jt = it.value().constBegin();
        for (; jt != it.value().constEnd(); ++jt) row[jt.key() - firstCol] = jt.value();
        out.append(row);
    }
    return out;
}

bool isCsvName(const QString &name)
{
    QString low = name.toLower();
    return low.endsWith(QS(".csv")) || low.endsWith(QS(".txt"));
}

QString decodeText(const QByteArray &data)
{
    QByteArray bytes = data;
    if (bytes.startsWith("\xEF\xBB\xBF")) bytes = bytes.mid(3);
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState state;
    QString text = utf8->toUnicode(bytes.constData(), bytes.size(), &state);
    if (state.invalidChars > 0 || text.contains(QChar(0xFFFD))) {
        QTextCodec *cp = QTextCodec::codecForName("Windows-1251");
        if (cp) text = cp->toUnicode(bytes);
    }
    return text;
}

}  // namespace BookUtil

using namespace BookUtil;

/* ------------------------------------------------------------------ выбор формата */

bool readBook(const QString &path, Book &book, QString &error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        error = QS("Файл не читается: ") + f.errorString();
        return false;
    }
    QByteArray data = f.readAll();
    f.close();
    return readBookData(data, QFileInfo(path).fileName(), book, error);
}

bool readBookData(const QByteArray &data, const QString &fileName, Book &book, QString &error)
{
    book = Book();
    if (data.isEmpty()) { error = QS("Файл пустой"); return false; }

    if (data.startsWith("PK\x03\x04")) return readXlsx(data, book, error);
    if (data.startsWith("\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1")) return readXls(data, book, error);

    QByteArray head = data.left(2048).toLower();
    int bom = head.startsWith("\xef\xbb\xbf") ? 3 : 0;
    QByteArray lead = head.mid(bom).trimmed();
    if (lead.startsWith("<?xml") && head.contains("urn:schemas-microsoft-com:office:spreadsheet"))
        return readXml2003(data, book, error);
    if (lead.startsWith("<") && (head.contains("<table") || head.contains("<html")))
        return readHtmlTable(data, book, error);

    if (isCsvName(fileName) || !fileName.contains(QChar('.')))
        return readCsv(data, book, error);

    // неизвестное расширение, но похоже на текст — пробуем как CSV
    int zeros = 0;
    for (int i = 0; i < qMin(data.size(), 4096); ++i) if (data.at(i) == 0) ++zeros;
    if (zeros == 0) return readCsv(data, book, error);

    error = QS("Неизвестный формат файла");
    return false;
}

/* --------------------------------------------------------------------------- CSV */

bool readCsv(const QByteArray &data, Book &book, QString &error)
{
    QString text = decodeText(data);
    if (text.startsWith(QChar(0xFEFF))) text = text.mid(1);

    int eol = text.indexOf(QChar('\n'));
    QString first = eol >= 0 ? text.left(eol) : text;
    int semi = first.count(QChar(';')), comma = first.count(QChar(',')), tab = first.count(QChar('\t'));
    QChar fs = (tab > semi && tab > comma) ? QChar('\t') : (semi >= comma ? QChar(';') : QChar(','));

    QList<Cell> cells;
    int row = 0, col = 0;
    QString field;
    bool quoted = false, fieldStarted = false;
    const int n = text.size();
    for (int i = 0; i < n; ++i) {
        QChar c = text.at(i);
        if (quoted) {
            if (c == QChar('"')) {
                if (i + 1 < n && text.at(i + 1) == QChar('"')) { field.append(c); ++i; }
                else quoted = false;
            } else {
                field.append(c);
            }
            continue;
        }
        if (c == QChar('"') && !fieldStarted) { quoted = true; fieldStarted = true; continue; }
        if (c == fs || c == QChar('\n') || c == QChar('\r')) {
            Cell cell; cell.row = row; cell.col = col; cell.value = field;
            cells.append(cell);
            field.clear(); fieldStarted = false;
            if (c == fs) { ++col; continue; }
            if (c == QChar('\r') && i + 1 < n && text.at(i + 1) == QChar('\n')) ++i;
            ++row; col = 0;
            continue;
        }
        field.append(c);
        fieldStarted = true;
    }
    if (fieldStarted || !field.isEmpty() || col > 0) {
        Cell cell; cell.row = row; cell.col = col; cell.value = field;
        cells.append(cell);
    }

    book.sheet = QS("Sheet1");
    book.rows = assemble(cells, 0);
    if (book.rows.isEmpty()) { error = QS("В файле нет данных"); return false; }
    return true;
}

/* -------------------------------------------------------- SpreadsheetML (XML 2003) */

bool readXml2003(const QByteArray &data, Book &book, QString &error)
{
    QXmlStreamReader xml(data);
    QList<Cell> cells;
    bool inSheet = false, done = false;
    int row = -1, col = 0;
    while (!xml.atEnd() && !done) {
        xml.readNext();
        if (xml.isStartElement()) {
            QString name = xml.name().toString();
            if (name == QS("Worksheet")) {
                if (inSheet) { done = true; break; }
                inSheet = true;
                book.sheet = xml.attributes().value(QS("urn:schemas-microsoft-com:office:spreadsheet"),
                                                    QS("Name")).toString();
            } else if (inSheet && name == QS("Row")) {
                QString idx = xml.attributes().value(QS("urn:schemas-microsoft-com:office:spreadsheet"),
                                                     QS("Index")).toString();
                row = idx.isEmpty() ? row + 1 : idx.toInt() - 1;
                col = 0;
            } else if (inSheet && name == QS("Cell")) {
                QString idx = xml.attributes().value(QS("urn:schemas-microsoft-com:office:spreadsheet"),
                                                     QS("Index")).toString();
                if (!idx.isEmpty()) col = idx.toInt() - 1;
                QString merge = xml.attributes().value(QS("urn:schemas-microsoft-com:office:spreadsheet"),
                                                       QS("MergeAcross")).toString();
                int span = merge.isEmpty() ? 0 : merge.toInt();
                QVariant value;
                // Data внутри Cell
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isEndElement() && xml.name() == QS("Cell")) break;
                    if (xml.isStartElement() && xml.name() == QS("Data")) {
                        QString type = xml.attributes().value(QS("urn:schemas-microsoft-com:office:spreadsheet"),
                                                              QS("Type")).toString();
                        QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                        if (type == QS("Number")) {
                            bool ok = false;
                            double d = text.toDouble(&ok);
                            value = ok ? QVariant(d) : QVariant(text);
                        } else if (type == QS("DateTime")) {
                            QDateTime dt = QDateTime::fromString(text.left(19), Qt::ISODate);
                            value = dt.isValid() ? QVariant(dt) : QVariant(text);
                        } else if (type == QS("Boolean")) {
                            value = QVariant(text.trimmed() == QS("1"));
                        } else {
                            value = text;
                        }
                    }
                }
                Cell c; c.row = row; c.col = col; c.value = value;
                cells.append(c);
                col += 1 + span;
            }
        } else if (xml.isEndElement() && xml.name() == QS("Worksheet")) {
            done = true;
        }
    }
    if (xml.hasError() && cells.isEmpty()) {
        error = QS("Не удалось разобрать XML-таблицу: ") + xml.errorString();
        return false;
    }
    if (book.sheet.isEmpty()) book.sheet = QS("Sheet1");
    book.rows = assemble(cells, 0);
    if (book.rows.isEmpty()) { error = QS("В книге нет данных"); return false; }
    return true;
}

/* ----------------------------------------------------------------- HTML-таблица */

static QString htmlDecode(QString s)
{
    s.replace(QRegExp(QS("<br\\s*/?>"), Qt::CaseInsensitive), QS(" "));
    s.remove(QRegExp(QS("<[^>]*>")));
    QRegExp num(QS("&#(x?)([0-9a-fA-F]+);"));
    int pos = 0;
    while ((pos = num.indexIn(s, pos)) >= 0) {
        bool ok = false;
        uint code = num.cap(1).isEmpty() ? num.cap(2).toUInt(&ok, 10) : num.cap(2).toUInt(&ok, 16);
        QString rep = ok && code < 0x10000 ? QString(QChar((ushort)code)) : QString();
        s.replace(pos, num.matchedLength(), rep);
        pos += rep.size();
    }
    s.replace(QS("&nbsp;"), QS(" "), Qt::CaseInsensitive);
    s.replace(QS("&lt;"), QS("<"), Qt::CaseInsensitive);
    s.replace(QS("&gt;"), QS(">"), Qt::CaseInsensitive);
    s.replace(QS("&quot;"), QS("\""), Qt::CaseInsensitive);
    s.replace(QS("&apos;"), QS("'"), Qt::CaseInsensitive);
    s.replace(QS("&amp;"), QS("&"), Qt::CaseInsensitive);
    return s.simplified();
}

bool readHtmlTable(const QByteArray &data, Book &book, QString &error)
{
    QByteArray head = data.left(4096).toLower();
    QString text;
    if (head.contains("charset=windows-1251") || head.contains("charset=\"windows-1251\"") ||
        head.contains("charset=cp1251")) {
        text = QTextCodec::codecForName("Windows-1251")->toUnicode(data);
    } else {
        text = decodeText(data);
    }

    QList<Cell> cells;
    QRegExp trRx(QS("<tr[^>]*>(.*)</tr>"), Qt::CaseInsensitive);
    trRx.setMinimal(true);
    QRegExp tdRx(QS("<t([dh])([^>]*)>(.*)</t\\1>"), Qt::CaseInsensitive);
    tdRx.setMinimal(true);
    QRegExp spanRx(QS("colspan\\s*=\\s*[\"']?(\\d+)"), Qt::CaseInsensitive);
    int row = 0, pos = 0;
    while ((pos = trRx.indexIn(text, pos)) >= 0) {
        QString tr = trRx.cap(1);
        pos += trRx.matchedLength();
        int col = 0, p = 0;
        while ((p = tdRx.indexIn(tr, p)) >= 0) {
            Cell c; c.row = row; c.col = col; c.value = htmlDecode(tdRx.cap(3));
            cells.append(c);
            int span = 1;
            if (spanRx.indexIn(tdRx.cap(2)) >= 0) span = qMax(1, spanRx.cap(1).toInt());
            col += span;
            p += tdRx.matchedLength();
        }
        ++row;
    }
    book.sheet = QS("Sheet1");
    book.rows = assemble(cells, 0);
    if (book.rows.isEmpty()) { error = QS("В HTML-файле не найдено таблицы"); return false; }
    return true;
}
