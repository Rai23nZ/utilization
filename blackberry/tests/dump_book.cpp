/* Хост-утилита: печатает первый лист книги построчно (cellText, через табуляцию). */
#include "../src/Book.hpp"

#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>
#include <stdio.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: dump_book FILE\n"); return 2; }
    Book book;
    QString error;
    if (!readBook(QString::fromLocal8Bit(argv[1]), book, error)) {
        printf("ERROR\t%s\n", error.toUtf8().constData());
        return 1;
    }
    printf("SHEET\t%s\n", book.sheet.toUtf8().constData());
    for (int r = 0; r < book.rows.size(); ++r) {
        QStringList cells;
        for (int c = 0; c < book.rows.at(r).size(); ++c) {
            QString t = BookUtil::cellText(book.rows.at(r).at(c));
            t.replace(QChar('\t'), QChar(' ')).replace(QChar('\n'), QChar(' '));
            cells << t;
        }
        printf("%s\n", cells.join(QString::fromUtf8("\t")).toUtf8().constData());
    }
    return 0;
}
