/* ==========================================================================
   Запись .xlsx: несколько листов, строки как inlineStr, числа как числа.
   ========================================================================== */
#ifndef XLSXWRITER_HPP
#define XLSXWRITER_HPP

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVariant>

class XlsxWriter {
public:
    /* rows: QString — текст, double/int — число, пустое — пропуск ячейки.
       widths — ширина столбцов в символах (необязательно). */
    void addSheet(const QString &name, const QList<QVariantList> &rows, const QList<int> &widths = QList<int>());
    QByteArray build() const;
    bool save(const QString &path, QString &error) const;

private:
    struct Sheet { QString name; QList<QVariantList> rows; QList<int> widths; };
    QList<Sheet> m_sheets;
};

#endif
