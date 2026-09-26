/* ==========================================================================
   Логика утилизации без интерфейса: разбор файла проверки и акта, сверка,
   поиск по номеру, форматирование. Перенесено из app.js один в один.
   ========================================================================== */
#ifndef LOGIC_HPP
#define LOGIC_HPP

#include "Book.hpp"

#include <QHash>
#include <QStringList>

struct Item {
    QString knt;        // полный номер, только цифры
    QString shortNo;    // короткий номер (столбец «КНТ» или 4 последние цифры)
    QString name;       // наименование
    QString decision;   // текст решения
    QStringList cells;  // все столбцы строки как текст
};

struct CheckFile {
    QString fileName, sheet;
    QStringList headers;
    int headerRow;
    QList<Item> allowed, denied;
    int empty;      // пустое решение
    int noKnt;      // строки без номера
    int total;
};

struct ActFile {
    QString fileName, sheet;
    QStringList knt;   // уникальные номера в порядке акта
    int dupes;
    int headerRow;
};

struct Extra {
    QString knt, why, name;
    bool miss;          // номера нет в файле проверки вообще
};

struct MatchResult {
    QList<Extra> extra;     // лишнее в акте — работа блокируется
    QList<Item> inWork;     // допущенное и есть в акте
};

namespace Logic {

bool isAllowed(const QString &decision);

bool parseCheck(const Book &book, const QString &fileName, CheckFile &out, QString &error);
bool parseAct(const Book &book, const QString &fileName, ActFile &out, QString &error);
MatchResult match(const CheckFile &check, const ActFile &act);

/* Сортировка только по наименованию (как localeCompare('ru')), затем по номеру. */
bool itemLess(const Item &a, const Item &b);
int ruCompare(const QString &a, const QString &b);

/* Позиции, подходящие под введённые цифры. */
QList<int> candidates(const QList<Item> &items, const QString &query);

/* Индексы столбцов по нормализованным заголовкам. */
int columnOf(const QStringList &headers, const QStringList &aliases);

QString plural(int n, const char *one, const char *few, const char *many);
QString hhmmss(qint64 ms);
QString clockOf(qint64 ts);
QString stampOf(qint64 ts);
QString dateOf(qint64 ts);
QString fileStamp(qint64 ts);   // ДД_ММ_ГГГГ_ЧЧММ

extern const char *const ALIAS_NAME[];
QStringList aliases(const char *const *list);
QStringList attrsCode();
QStringList attrsBrand();
QStringList attrsPrice();

}

#endif
