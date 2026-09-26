/* Хост-тест контроллера: полный сценарий утилизации без интерфейса.
   Запуск: controller_test FIXTURES_DIR  (UTIL_SD_ROOT и UTIL_DATA_DIR — временные каталоги) */
#include "../src/Controller.hpp"
#include "../src/Platform.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QStringList>
#include <stdio.h>

static int fails = 0;
static QStringList toasts;
static QString lastConfirm;

#define CHECK(cond, msg) do { if (!(cond)) { ++fails; printf("FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); } \
                              else printf("ok  : %s\n", msg); } while (0)

class Spy : public QObject {
    Q_OBJECT
public slots:
    void onToast(const QString &t) { toasts << t; }
    void onConfirm(const QString &a, const QString &, const QString &, const QString &) { lastConfirm = a; }
};

static void pump(int ms = 120)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

static QString s(const QVariant &v) { return v.toString(); }

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QString fix = QString::fromLocal8Bit(argv[1]);
    QString sd = Platform::sdRoot();
    QDir(sd).removeRecursively();
    QDir().mkpath(sd);
    QDir(Platform::dataDir()).removeRecursively();

    Spy spy;
    Controller *c = new Controller;
    QObject::connect(c, SIGNAL(toast(QString)), &spy, SLOT(onToast(QString)));
    QObject::connect(c, SIGNAL(confirm(QString,QString,QString,QString)), &spy, SLOT(onConfirm(QString,QString,QString,QString)));

    CHECK(QDir(sd + "/utilization").exists(), "папка utilization создана при первом запуске");
    CHECK(c->screen() == "load", "стартовый экран — загрузка");
    CHECK(!c->resumeInfo().value("has").toBool(), "нет незавершённой сессии");

    // файлы на карте
    QDir().mkpath(sd + "/Documents");
    QFile::copy(fix + "/check_lo97.xls", sd + "/Documents/Проверка_КНТ.xls");
    QFile::copy(fix + "/act_1251.csv", sd + "/Documents/Акт_1245.csv");
    QFile::copy(fix + "/check.xlsx", sd + "/Проверка.xlsx");
    QFile(sd + "/readme.pdf").open(QIODevice::WriteOnly);

    c->browse("check");
    CHECK(c->screen() == "files", "экран выбора файла");
    QVariantList items = c->browseItems();
    QStringList names;
    for (int i = 0; i < items.size(); ++i) names << s(items.at(i).toMap().value("name"));
    printf("      корень: %s\n", names.join(" | ").toUtf8().constData());
    CHECK(names.contains("Documents/") && names.contains("utilization/") && names.contains("Проверка.xlsx"), "видны папки и таблицы");
    CHECK(!names.contains("readme.pdf"), "посторонние файлы скрыты");
    CHECK(!c->browseCanUp(), "выше корня карты не подняться");
    c->browseOpen(names.indexOf("Documents/"));
    CHECK(c->browsePath() == QString::fromUtf8("SD-карта / Documents /"), "путь внутри карты");
    items = c->browseItems();
    names.clear();
    for (int i = 0; i < items.size(); ++i) names << s(items.at(i).toMap().value("name"));
    c->browseOpen(names.indexOf(QString::fromUtf8("Проверка_КНТ.xls")));
    CHECK(c->checkInfo().value("state") == "busy", "проверка читается");
    pump();
    CHECK(c->checkInfo().value("state") == "ok", "файл проверки (.xls) разобран");
    printf("      %s\n", s(c->checkInfo().value("line")).toUtf8().constData());

    c->browse("act");
    CHECK(c->browsePath() == QString::fromUtf8("SD-карта / Documents /"), "выбор акта открывается в той же папке");
    items = c->browseItems();
    names.clear();
    for (int i = 0; i < items.size(); ++i) names << s(items.at(i).toMap().value("name"));
    c->browseOpen(names.indexOf(QString::fromUtf8("Акт_1245.csv")));
    pump();
    CHECK(c->actInfo().value("state") == "ok", "акт (CSV 1251) разобран");
    printf("      %s\n", s(c->actInfo().value("line")).toUtf8().constData());

    // в акте 12 номеров: среди них есть НЕ ДОПУЩЕН и пустое решение → блокировка
    CHECK(c->screen() == "block", "лишние КНТ в акте блокируют работу");
    QVariantList extra = c->extraList();
    CHECK(extra.size() > 0, "список лишних не пуст");
    for (int i = 0; i < extra.size(); ++i)
        printf("      лишний: %s — %s\n", s(extra.at(i).toMap().value("knt")).toUtf8().constData(),
               s(extra.at(i).toMap().value("why")).toUtf8().constData());
    c->copyExtra();
    CHECK(!QDir(sd + "/utilization").entryList(QStringList() << QString::fromUtf8("Лишние_КНТ_*.txt")).isEmpty(), "список лишних сохранён в utilization");

    // исправленный акт: только допущенные номера
    QFile f(sd + "/Documents/Акт_исправленный.csv");
    f.open(QIODevice::WriteOnly);
    f.write("Акт\n\n№ КНТ;Наименование\n30012000;x\n30012037;y\n30012074;z\n30012259;w\n30012296;v\n30012000;дубль\n");
    f.close();
    c->loadFile("act", sd + "/Documents/Акт_исправленный.csv");
    pump();
    CHECK(c->screen() == "load", "после исправленного акта — снова загрузка");
    QVariantMap mi = c->matchInfo();
    CHECK(mi.value("ready").toBool() && mi.value("inWork").toInt() == 5, "к утилизации 5 позиций");
    QVariantList notes = mi.value("notes").toList();
    for (int i = 0; i < notes.size(); ++i)
        printf("      заметка [%s] %s: %s\n", s(notes.at(i).toMap().value("c")).toUtf8().constData(),
               s(notes.at(i).toMap().value("k")).toUtf8().constData(), s(notes.at(i).toMap().value("v")).toUtf8().constData());

    c->start();
    CHECK(c->screen() == "work" && c->total() == 5 && c->doneCount() == 0, "сессия начата");
    CHECK(QFile::exists(sd + "/utilization/.session.json"), "сессия сохраняется на карту");
    CHECK(c->status().value("text").toString() == QString::fromUtf8("ВВЕДИТЕ 4 ПОСЛЕДНИЕ ЦИФРЫ КНТ"), "подсказка ввода");

    // поиск по 4 цифрам
    c->key("2"); c->key("0");
    CHECK(!c->card().value("has").toBool(), "пока набираем — карточки нет");
    c->key("3"); c->key("7");
    QVariantMap card = c->card();
    CHECK(card.value("has").toBool() && card.value("knt") == "30012037", "4 цифры открыли карточку");
    CHECK(card.value("tail") == "2037" && card.value("head") == "3001", "набранные цифры выделены");
    printf("      карточка: %s · %s · теги %d · действие %s\n", s(card.value("knt")).toUtf8().constData(),
           s(card.value("name")).toUtf8().constData(), card.value("tags").toList().size(), s(card.value("action")).toUtf8().constData());
    CHECK(card.value("action") == "off", "без таймера отметка недоступна");
    c->mark();
    CHECK(c->doneCount() == 0 && toasts.last() == QString::fromUtf8("Таймер не запущен"), "отметка без таймера отклонена");

    c->timerToggle();
    CHECK(c->timerState() == "run", "таймер запущен");
    CHECK(c->card().value("action") == "mark", "теперь можно отмечать");
    c->keyPress(32, " ");
    CHECK(c->doneCount() == 1, "пробел отмечает");
    CHECK(c->typed().isEmpty(), "после отметки ввод очищен");

    // неизвестный номер
    c->key("9"); c->key("9"); c->key("9"); c->key("9");
    CHECK(c->status().value("kind") == "err", "9999 — нет в выборке");
    c->key("2");
    CHECK(c->typed() == "2", "после ошибки новая цифра начинает новый номер");
    c->clearInput();

    // повторная отметка того же
    c->key("2"); c->key("0"); c->key("3"); c->key("7");
    CHECK(c->card().value("marked").toBool() && c->card().value("action") == "unmark", "отмеченная позиция: снять отметку");
    c->primary();
    CHECK(lastConfirm == "unmark", "снятие требует подтверждения");
    c->confirmed("unmark");
    CHECK(c->doneCount() == 0, "отметка снята");

    // сетка
    c->openGrid();
    CHECK(c->screen() == "grid" && c->gridCount() == 5, "сетка: 5 плиток");
    QVariantList grid = c->gridSnapshot();
    QString undoSt;
    for (int i = 0; i < grid.size(); ++i) if (grid.at(i).toMap().value("st") == "undo") undoSt = s(grid.at(i).toMap().value("sn"));
    CHECK(undoSt == "2037", "плитка со снятой отметкой помечена");
    int firstIdx = grid.at(0).toMap().value("idx").toInt();
    c->gridOpen(firstIdx);
    CHECK(c->sheetOpen() && c->card().value("idx").toInt() == firstIdx, "шторка открыта на выбранной позиции");
    c->mark();
    CHECK(c->doneCount() == 1 && c->gridCount() == 4, "отмечено в сетке, утилизированное скрыто");
    c->sheetClose();
    c->gridToggleDone();
    CHECK(c->gridShowDone() && c->gridCount() == 5, "показать утилизированные");
    c->gridOpen(firstIdx);
    c->primary();
    c->confirmed("unmark");
    CHECK(c->doneCount() == 0, "снятие отметки из сетки");
    c->mark();
    c->sheetClose();
    c->closeGrid();
    CHECK(c->screen() == "work", "назад из сетки");

    // пауза и продолжение
    c->timerToggle();
    CHECK(c->timerState() == "pause", "пауза");
    c->nextTodo();
    CHECK(c->card().value("action") == "off" && c->status().value("kind") == "warn", "на паузе отмечать нельзя");
    c->timerToggle();
    c->mark();
    CHECK(c->doneCount() == 2, "вторая отметка");

    // перезапуск приложения: сессия восстанавливается
    delete c;
    c = new Controller;
    QObject::connect(c, SIGNAL(toast(QString)), &spy, SLOT(onToast(QString)));
    QObject::connect(c, SIGNAL(confirm(QString,QString,QString,QString)), &spy, SLOT(onConfirm(QString,QString,QString,QString)));
    QVariantMap ri = c->resumeInfo();
    CHECK(ri.value("has").toBool(), "после перезапуска предложено продолжить");
    printf("      %s\n", s(ri.value("text")).toUtf8().constData());
    c->resume();
    CHECK(c->screen() == "work" && c->doneCount() == 2 && c->total() == 5 && c->timerState() == "run", "сессия восстановлена");

    // завершение
    c->requestFinish();
    CHECK(lastConfirm == "finish", "завершение требует подтверждения");
    c->confirmed("finish");
    CHECK(c->screen() == "done" && c->timerState() == "done", "итог");
    QVariantList rc = c->receipt();
    for (int i = 0; i < rc.size(); ++i) {
        QVariantMap r = rc.at(i).toMap();
        printf("      | %-22s %s\n", s(r.value("k")).toUtf8().constData(), s(r.value("v")).toUtf8().constData());
    }
    c->exportXlsx();
    CHECK(c->exportNote().startsWith(QString::fromUtf8("ЗАПИСАНО")), "xlsx выгружен");
    c->exportText();
    QStringList out = QDir(sd + "/utilization").entryList(QDir::Files | QDir::Hidden);
    printf("      utilization/: %s\n", out.join(" | ").toUtf8().constData());
    CHECK(!QDir(sd + "/utilization").entryList(QStringList() << "*.xlsx").isEmpty(), "файл .xlsx на карте");
    CHECK(!QDir(sd + "/utilization").entryList(QStringList() << "*.txt").isEmpty(), "файл .txt на карте");

    // начать заново
    c->requestRestart();
    c->confirmed("restart");
    CHECK(c->screen() == "load" && !QFile::exists(sd + "/utilization/.session.json"), "начать заново стирает сессию");
    delete c;

    printf("FAILS: %d\n", fails);
    return fails ? 1 : 0;
}

#include "controller_test.moc"
