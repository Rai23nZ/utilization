/* ==========================================================================
   .xlsx / .xlsm: первый лист, общие строки, стили дат.
   ========================================================================== */
#include "Book.hpp"
#include "Zip.hpp"

#include <QDateTime>
#include <QHash>
#include <QRegExp>
#include <QStringList>
#include <QXmlStreamReader>

using namespace BookUtil;

/* _x000D_ и подобные экранирования Excel */
static QString unescapeOoxml(const QString &s)
{
    if (!s.contains(QS("_x"))) return s;
    QString out = s;
    QRegExp rx(QS("_x([0-9A-Fa-f]{4})_"));
    int pos = 0;
    while ((pos = rx.indexIn(out, pos)) >= 0) {
        QString rep(QChar((ushort)rx.cap(1).toUInt(0, 16)));
        out.replace(pos, rx.matchedLength(), rep);
        pos += rep.size();
    }
    return out;
}

static QString attrLocal(const QXmlStreamAttributes &attrs, const QString &local, bool prefixed)
{
    for (int i = 0; i < attrs.size(); ++i) {
        const QXmlStreamAttribute &a = attrs.at(i);
        if (a.name() == local && (prefixed ? !a.prefix().isEmpty() : a.prefix().isEmpty()))
            return a.value().toString();
    }
    return QString();
}

static QString dirOf(const QString &path)
{
    int slash = path.lastIndexOf(QChar('/'));
    return slash >= 0 ? path.left(slash + 1) : QString();
}

static QString resolveTarget(const QString &base, const QString &target)
{
    if (target.startsWith(QChar('/'))) return target.mid(1);
    QStringList parts = (dirOf(base) + target).split(QChar('/'));
    QStringList out;
    for (int i = 0; i < parts.size(); ++i) {
        if (parts.at(i) == QS("..")) { if (!out.isEmpty()) out.removeLast(); }
        else if (parts.at(i) != QS(".") && !parts.at(i).isEmpty()) out << parts.at(i);
    }
    return out.join(QS("/"));
}

/* rId → путь из файла связей */
static QHash<QString, QString> readRels(const QByteArray &xmlData, const QString &ownerPath)
{
    QHash<QString, QString> map;
    QXmlStreamReader xml(xmlData);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QS("Relationship")) {
            QString id = xml.attributes().value(QS("Id")).toString();
            QString target = xml.attributes().value(QS("Target")).toString();
            QString mode = xml.attributes().value(QS("TargetMode")).toString();
            if (mode == QS("External")) continue;
            map.insert(id, resolveTarget(ownerPath, target));
        }
    }
    return map;
}

static QString relsPathFor(const QString &part)
{
    return dirOf(part) + QS("_rels/") + part.mid(dirOf(part).size()) + QS(".rels");
}

/* Индекс столбца по ссылке «AB12» */
static bool parseRef(const QString &ref, int &row, int &col)
{
    int i = 0, c = 0;
    while (i < ref.size() && ref.at(i).isLetter()) {
        c = c * 26 + (ref.at(i).toUpper().unicode() - 'A' + 1);
        ++i;
    }
    if (i == 0) return false;
    bool ok = false;
    int r = ref.mid(i).toInt(&ok);
    if (!ok) return false;
    col = c - 1;
    row = r - 1;
    return true;
}

bool readXlsx(const QByteArray &data, Book &book, QString &error)
{
    ZipReader zip;
    if (!zip.open(data, error)) return false;

    // главный документ книги
    QString workbookPath = QS("xl/workbook.xml");
    if (zip.contains(QS("_rels/.rels"))) {
        QXmlStreamReader xml(zip.file(QS("_rels/.rels")));
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == QS("Relationship") &&
                xml.attributes().value(QS("Type")).toString().endsWith(QS("/officeDocument"))) {
                workbookPath = resolveTarget(QString(), xml.attributes().value(QS("Target")).toString());
            }
        }
    }
    workbookPath = zip.resolve(workbookPath);
    if (workbookPath.isEmpty()) {
        error = QS("Это не книга Excel: внутри архива нет workbook.xml");
        return false;
    }

    // первый лист и режим дат
    QString firstRid, firstName;
    bool date1904 = false;
    {
        QXmlStreamReader xml(zip.file(workbookPath));
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) continue;
            if (xml.name() == QS("workbookPr")) {
                QString v = xml.attributes().value(QS("date1904")).toString();
                date1904 = (v == QS("1") || v == QS("true"));
            } else if (xml.name() == QS("sheet") && firstRid.isEmpty()) {
                firstName = xml.attributes().value(QS("name")).toString();
                firstRid = attrLocal(xml.attributes(), QS("id"), true);
            }
        }
    }
    if (firstRid.isEmpty()) { error = QS("В книге нет ни одного листа"); return false; }

    QHash<QString, QString> rels = readRels(zip.file(relsPathFor(workbookPath)), workbookPath);
    QString sheetPath = zip.resolve(rels.value(firstRid));
    if (sheetPath.isEmpty()) sheetPath = zip.resolve(QS("xl/worksheets/sheet1.xml"));
    if (sheetPath.isEmpty()) { error = QS("Не найден первый лист книги"); return false; }

    QString sstPath, stylesPath;
    QHash<QString, QString>::const_iterator it = rels.constBegin();
    for (; it != rels.constEnd(); ++it) {
        QString low = it.value().toLower();
        if (low.endsWith(QS("sharedstrings.xml"))) sstPath = it.value();
        if (low.endsWith(QS("styles.xml"))) stylesPath = it.value();
    }
    if (sstPath.isEmpty()) sstPath = QS("xl/sharedStrings.xml");
    if (stylesPath.isEmpty()) stylesPath = QS("xl/styles.xml");

    // общие строки
    QStringList sst;
    if (zip.contains(sstPath)) {
        QXmlStreamReader xml(zip.file(sstPath));
        QString cur;
        bool inSi = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                if (xml.name() == QS("si")) { inSi = true; cur.clear(); }
                else if (xml.name() == QS("rPh")) xml.skipCurrentElement();
                else if (inSi && xml.name() == QS("t")) cur += xml.readElementText();
            } else if (xml.isEndElement() && xml.name() == QS("si")) {
                sst << unescapeOoxml(cur);
                inSi = false;
            }
        }
    }

    // стили: какие xf — даты
    QList<bool> xfIsDate;
    if (zip.contains(stylesPath)) {
        QHash<int, QString> customFmt;
        QXmlStreamReader xml(zip.file(stylesPath));
        bool inCellXfs = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                if (xml.name() == QS("numFmt")) {
                    customFmt.insert(xml.attributes().value(QS("numFmtId")).toString().toInt(),
                                     xml.attributes().value(QS("formatCode")).toString());
                } else if (xml.name() == QS("cellXfs")) {
                    inCellXfs = true;
                } else if (inCellXfs && xml.name() == QS("xf")) {
                    int id = xml.attributes().value(QS("numFmtId")).toString().toInt();
                    bool isDate = customFmt.contains(id) ? isDateFormatCode(customFmt.value(id))
                                                         : isDateFormatId(id);
                    xfIsDate << isDate;
                }
            } else if (xml.isEndElement() && xml.name() == QS("cellXfs")) {
                inCellXfs = false;
            }
        }
    }

    // лист
    QList<Cell> cells;
    int firstCol = 0;
    {
        QXmlStreamReader xml(zip.file(sheetPath));
        int row = -1, col = -1;
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) continue;
            QString name = xml.name().toString();
            if (name == QS("dimension")) {
                int r = 0, c = 0;
                QString ref = xml.attributes().value(QS("ref")).toString().section(QChar(':'), 0, 0);
                if (parseRef(ref, r, c)) firstCol = c;
            } else if (name == QS("row")) {
                QString r = xml.attributes().value(QS("r")).toString();
                row = r.isEmpty() ? row + 1 : r.toInt() - 1;
                col = -1;
            } else if (name == QS("c")) {
                QString ref = xml.attributes().value(QS("r")).toString();
                int r = row, c = col + 1;
                if (!ref.isEmpty()) parseRef(ref, r, c);
                if (r < 0) r = 0;
                row = r; col = c;
                QString t = xml.attributes().value(QS("t")).toString();
                int s = xml.attributes().value(QS("s")).toString().toInt();

                QString v, inlineText;
                bool hasV = false, hasInline = false;
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isEndElement() && xml.name() == QS("c")) break;
                    if (!xml.isStartElement()) continue;
                    if (xml.name() == QS("v")) { v = xml.readElementText(); hasV = true; }
                    else if (xml.name() == QS("is")) {
                        hasInline = true;
                        while (!xml.atEnd()) {
                            xml.readNext();
                            if (xml.isEndElement() && xml.name() == QS("is")) break;
                            if (xml.isStartElement() && xml.name() == QS("rPh")) xml.skipCurrentElement();
                            else if (xml.isStartElement() && xml.name() == QS("t")) inlineText += xml.readElementText();
                        }
                    } else {
                        xml.skipCurrentElement();
                    }
                }

                QVariant value;
                if (t == QS("s")) {
                    int idx = v.toInt();
                    if (hasV && idx >= 0 && idx < sst.size()) value = sst.at(idx);
                } else if (t == QS("inlineStr")) {
                    if (hasInline) value = unescapeOoxml(inlineText);
                } else if (t == QS("str") || t == QS("e")) {
                    if (hasV) value = unescapeOoxml(v);
                } else if (t == QS("b")) {
                    if (hasV) value = (v.trimmed() == QS("1") || v.trimmed() == QS("true"));
                } else if (t == QS("d")) {
                    if (hasV) {
                        QDateTime dt = QDateTime::fromString(v.left(19), Qt::ISODate);
                        value = dt.isValid() ? QVariant(dt) : QVariant(v);
                    }
                } else if (hasV) {
                    bool ok = false;
                    double d = v.toDouble(&ok);
                    if (!ok) value = v;
                    else if (s >= 0 && s < xfIsDate.size() && xfIsDate.at(s)) value = excelDate(d, date1904);
                    else value = d;
                }
                if (value.isValid()) {
                    Cell cell; cell.row = r; cell.col = c; cell.value = value;
                    cells.append(cell);
                }
            }
        }
        if (xml.hasError() && cells.isEmpty()) {
            error = QS("Лист повреждён: ") + xml.errorString();
            return false;
        }
    }

    book.sheet = firstName.isEmpty() ? QS("Sheet1") : firstName;
    book.rows = assemble(cells, firstCol);
    return true;
}
