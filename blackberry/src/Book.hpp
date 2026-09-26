/* ==========================================================================
   Чтение таблиц: первый лист книги как матрица значений.
   Понимает .xlsx/.xlsm (Office Open XML), .xls (BIFF8 и BIFF5 внутри OLE2,
   а также «xls», сохранённые как XML 2003 или HTML) и .csv/.txt.
   Зависит только от QtCore и zlib — собирается и на устройстве, и на хосте.
   ========================================================================== */
#ifndef BOOK_HPP
#define BOOK_HPP

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVariant>

#define QS(s) QString::fromUtf8(s)

/* Значение ячейки: QString, double, bool или QDateTime (если формат ячейки — дата).
   Пустая ячейка — пустой QVariant. */
typedef QList<QVariantList> Rows;

struct Book {
    QString sheet;   // имя первого листа
    Rows rows;       // строки без полностью пустых, выровненные по ширине листа
};

/* Читает файл целиком и разбирает по сигнатуре, а при её отсутствии — по расширению. */
bool readBook(const QString &path, Book &book, QString &error);
bool readBookData(const QByteArray &data, const QString &fileName, Book &book, QString &error);

bool readXlsx(const QByteArray &data, Book &book, QString &error);
bool readXls(const QByteArray &data, Book &book, QString &error);
bool readCsv(const QByteArray &data, Book &book, QString &error);
bool readXml2003(const QByteArray &data, Book &book, QString &error);
bool readHtmlTable(const QByteArray &data, Book &book, QString &error);

namespace BookUtil {

/* Текст ячейки для показа и выгрузки — как cellText() в app.js:
   дата → ДД.ММ.ГГГГ, целое → без дробной части, дробное → до сотых. */
QString cellText(const QVariant &v);

/* Только цифры из строки. */
QString digits(const QString &s);

/* Нормализация заголовка: регистр, ё→е, лишние пробелы, хвостовые «:» и «.». */
QString normHeader(const QVariant &v);

/* Серийный номер даты Excel → дата/время. */
QDateTime excelDate(double serial, bool date1904);

/* Похож ли числовой формат на дату. */
bool isDateFormatId(int id);
bool isDateFormatCode(const QString &code);

/* Собирает строки из разреженного набора ячеек: убирает пустые строки,
   выравнивает все строки по ширине. */
struct Cell { int row; int col; QVariant value; };
Rows assemble(const QList<Cell> &cells, int firstCol);

/* Строка «похоже на CSV»? */
bool isCsvName(const QString &name);

/* Декодирование текста: UTF-8, при битых последовательностях — Windows-1251. */
QString decodeText(const QByteArray &data);

}

#endif
