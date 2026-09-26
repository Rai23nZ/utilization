#include "XlsxWriter.hpp"
#include "Zip.hpp"

#include <QFile>
#include <QStringList>

#define QS(s) QString::fromUtf8(s)

static QString xmlEscape(const QString &s)
{
    QString out;
    out.reserve(s.size() + 8);
    for (int i = 0; i < s.size(); ++i) {
        QChar c = s.at(i);
        ushort u = c.unicode();
        if (u == '&') out += QS("&amp;");
        else if (u == '<') out += QS("&lt;");
        else if (u == '>') out += QS("&gt;");
        else if (u == '"') out += QS("&quot;");
        else if (u < 0x20 && u != '\t' && u != '\n' && u != '\r') continue;   // недопустимо в XML
        else if (u == 0xFFFE || u == 0xFFFF) continue;
        else out += c;
    }
    return out;
}

static QString colName(int c)
{
    QString s;
    ++c;
    while (c > 0) {
        int m = (c - 1) % 26;
        s.prepend(QChar('A' + m));
        c = (c - 1) / 26;
    }
    return s;
}

/* Имя листа: не длиннее 31 символа и без []:*?/\ */
static QString sheetName(const QString &name, int idx)
{
    QString s = name;
    s.remove(QChar('[')).remove(QChar(']')).remove(QChar(':')).remove(QChar('*'))
     .remove(QChar('?')).remove(QChar('/')).remove(QChar('\\'));
    s = s.left(31).trimmed();
    if (s.isEmpty()) s = QS("Лист") + QString::number(idx + 1);
    return s;
}

void XlsxWriter::addSheet(const QString &name, const QList<QVariantList> &rows, const QList<int> &widths)
{
    Sheet s;
    s.name = name;
    s.rows = rows;
    s.widths = widths;
    m_sheets.append(s);
}

QByteArray XlsxWriter::build() const
{
    ZipWriter zip;
    const QString xmlHead = QS("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");

    QString types = xmlHead +
        QS("<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
          "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
          "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
          "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>");
    for (int i = 0; i < m_sheets.size(); ++i)
        types += QString(QS("<Override PartName=\"/xl/worksheets/sheet%1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>")).arg(i + 1);
    types += QS("</Types>");
    zip.add(QS("[Content_Types].xml"), types.toUtf8());

    zip.add(QS("_rels/.rels"), (xmlHead +
        QS("<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
          "</Relationships>")).toUtf8());

    QString wb = xmlHead +
        QS("<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
          "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheets>");
    QString wbRels = xmlHead +
        QS("<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">");
    QStringList used;
    for (int i = 0; i < m_sheets.size(); ++i) {
        QString nm = sheetName(m_sheets.at(i).name, i);
        while (used.contains(nm, Qt::CaseInsensitive)) nm = nm.left(28) + QS(" ") + QString::number(i + 1);
        used << nm;
        wb += QString(QS("<sheet name=\"%1\" sheetId=\"%2\" r:id=\"rId%2\"/>")).arg(xmlEscape(nm)).arg(i + 1);
        wbRels += QString(QS("<Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet%1.xml\"/>")).arg(i + 1);
    }
    wb += QS("</sheets></workbook>");
    wbRels += QString(QS("<Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>")).arg(m_sheets.size() + 1);
    wbRels += QS("</Relationships>");
    zip.add(QS("xl/workbook.xml"), wb.toUtf8());
    zip.add(QS("xl/_rels/workbook.xml.rels"), wbRels.toUtf8());

    zip.add(QS("xl/styles.xml"), (xmlHead +
        QS("<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
          "<fonts count=\"1\"><font><sz val=\"11\"/><name val=\"Calibri\"/><family val=\"2\"/></font></fonts>"
          "<fills count=\"2\"><fill><patternFill patternType=\"none\"/></fill><fill><patternFill patternType=\"gray125\"/></fill></fills>"
          "<borders count=\"1\"><border><left/><right/><top/><bottom/><diagonal/></border></borders>"
          "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
          "<cellXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/></cellXfs>"
          "<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles>"
          "</styleSheet>")).toUtf8());

    for (int i = 0; i < m_sheets.size(); ++i) {
        const Sheet &s = m_sheets.at(i);
        QString x = xmlHead +
            QS("<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
              "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">");
        if (!s.widths.isEmpty()) {
            x += QS("<cols>");
            for (int c = 0; c < s.widths.size(); ++c)
                x += QString(QS("<col min=\"%1\" max=\"%1\" width=\"%2\" customWidth=\"1\"/>")).arg(c + 1).arg(qBound(4, s.widths.at(c), 80));
            x += QS("</cols>");
        }
        x += QS("<sheetData>");
        for (int r = 0; r < s.rows.size(); ++r) {
            const QVariantList &row = s.rows.at(r);
            x += QString(QS("<row r=\"%1\">")).arg(r + 1);
            for (int c = 0; c < row.size(); ++c) {
                const QVariant &v = row.at(c);
                if (!v.isValid() || v.isNull()) continue;
                QString ref = colName(c) + QString::number(r + 1);
                if (v.type() == QVariant::Double || v.type() == QVariant::Int || v.type() == QVariant::LongLong ||
                    v.type() == QVariant::UInt || v.type() == QVariant::ULongLong) {
                    x += QString(QS("<c r=\"%1\"><v>%2</v></c>")).arg(ref, QString::number(v.toDouble(), 'g', 15));
                } else {
                    QString text = v.toString();
                    if (text.isEmpty()) continue;
                    x += QString(QS("<c r=\"%1\" t=\"inlineStr\"><is><t xml:space=\"preserve\">%2</t></is></c>"))
                             .arg(ref, xmlEscape(text));
                }
            }
            x += QS("</row>");
        }
        x += QS("</sheetData></worksheet>");
        zip.add(QString(QS("xl/worksheets/sheet%1.xml")).arg(i + 1), x.toUtf8());
    }
    return zip.finish();
}

bool XlsxWriter::save(const QString &path, QString &error) const
{
    QByteArray data = build();
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = f.errorString();
        return false;
    }
    if (f.write(data) != data.size()) {
        error = f.errorString();
        f.close();
        return false;
    }
    f.close();
    return true;
}
