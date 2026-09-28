#include "Controller.hpp"
#include "Platform.hpp"
#include "XlsxWriter.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QtAlgorithms>

using namespace Logic;
using namespace BookUtil;

static QVariantMap kv(const QString &k, const QVariant &v, const QString &c = QString())
{
    QVariantMap m;
    m.insert(QS("k"), k);
    m.insert(QS("v"), v);
    m.insert(QS("c"), c);
    return m;
}

static QString upperWords(const QString &s) { return s.toUpper(); }

/* Ширина строк в пикселях Passport (1 пт ≈ 6,4 пк — замерено на устройстве) с запасом 3 %.
   Наименование: IBM Plex Sans 8,5 пт в табло шириной 1300 пк.
   Имя файла: Plex Mono 5,5 пт на плитке шириной 600 пк. */
static const double PX_PER_PT = 6.4;
static const double NAME_PX = 1300 * 0.97;
static const double NAME_FONT = 8.5 * PX_PER_PT;
static const double FILE_PX = 600 * 0.97;
static const double FILE_FONT = 5.5 * PX_PER_PT;
/* «Подробно»: значения Plex Sans 7 пт, подписи Plex Mono 5 пт, ширина 1356 пк; строк — сколько нужно */
static const double DETAIL_PX = 1356 * 0.97;
static const double DETAIL_FONT = 7 * PX_PER_PT;
static const double DETAIL_KEY_FONT = 5 * PX_PER_PT;

/* JsonDataAccess на устройстве надёжно пишет только QVariantList/QVariantMap/скаляры */
static QVariantList listOf(const QStringList &l)
{
    QVariantList out;
    for (int i = 0; i < l.size(); ++i) out << l.at(i);
    return out;
}

Controller::Controller(QObject *parent)
    : QObject(parent),
      m_screen(QS("load")),
      m_hasCheck(false), m_hasAct(false), m_matchReady(false),
      m_active(false), m_createdAt(0), m_cursor(-1), m_colCode(-1), m_colBrand(-1), m_colPrice(-1),
      m_resumeAvailable(false), m_fresh(false), m_detailsOpen(false),
      m_gridShowDone(false), m_sheetOpen(false), m_sheetSeq(0), m_closeSeq(-1)
{
    m_checkState = m_actState = QS("empty");
    resetSession();
#ifdef Q_OS_BLACKBERRY
    m_gridModel = new bb::cascades::ArrayDataModel(this);
#endif
    m_tick.setInterval(1000);
    connect(&m_tick, SIGNAL(timeout()), this, SLOT(tick()));
    m_tick.start();

    Platform::ensureOutDir();   // папка utilization создаётся при первом запуске
    if (loadSession()) m_resumeAvailable = true;
}

/* ------------------------------------------------------------------ общее */

qint64 Controller::now() const { return QDateTime::currentMSecsSinceEpoch(); }

void Controller::setScreen(const QString &s)
{
    if (m_screen == s) return;
    m_screen = s;
    emit screenChanged();
}

bool Controller::sdReady() const { return Platform::sdReady(); }

QString Controller::outDirText() const
{
    return Platform::sdReady() ? QS("SD-карта / utilization") : QS("SD-карта не найдена");
}

void Controller::refreshSd()
{
    Platform::ensureOutDir();
    emit sdChanged();
    if (m_screen == QS("files")) listDir();
}

void Controller::tick()
{
    if (m_timer.state == QS("run") || m_timer.state == QS("pause")) emit timerChanged();
}

/* ------------------------------------------------------------- выбор файла */

void Controller::browse(const QString &kind)
{
    m_browseKind = kind;
    if (m_screen != QS("files")) m_prevScreen = m_screen;
    Platform::ensureOutDir();
    QString root = Platform::sdRoot();
    QString dir = m_lastDir.value(kind);
    if (dir.isEmpty()) dir = m_lastDir.value(QS("any"));
    if (dir.isEmpty() || !QDir(dir).exists() || !dir.startsWith(root)) dir = root;
    m_browseDir = dir;
    listDir();
    setScreen(QS("files"));
}

void Controller::listDir()
{
    m_browseItems.clear();
    if (!Platform::sdReady()) {
        m_browseNote = QS("SD-КАРТА НЕ НАЙДЕНА");
        emit browseChanged();
        emit sdChanged();
        return;
    }
    QDir d(m_browseDir);
    QFileInfoList dirs = d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    QFileInfoList files = d.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase);
    QString checkPath = m_hasCheck ? m_lastDir.value(QS("checkFile")) : QString();
    QString actPath = m_hasAct ? m_lastDir.value(QS("actFile")) : QString();

    for (int i = 0; i < dirs.size(); ++i) {
        QVariantMap m;
        m.insert(QS("name"), dirs.at(i).fileName() + QS("/"));
        m.insert(QS("dir"), true);
        m.insert(QS("info"), dirs.at(i).fileName() == QS("utilization") ? QS("ОТЧЁТЫ") : QS("ПАПКА"));
        m.insert(QS("sel"), QString());
        m.insert(QS("path"), dirs.at(i).absoluteFilePath());
        m_browseItems << m;
    }
    int fileCount = 0;
    for (int i = 0; i < files.size(); ++i) {
        QString suffix = files.at(i).suffix().toLower();
        if (suffix != QS("xlsx") && suffix != QS("xlsm") && suffix != QS("xls") && suffix != QS("csv") && suffix != QS("txt"))
            continue;
        QVariantMap m;
        QString path = files.at(i).absoluteFilePath();
        qint64 kb = (files.at(i).size() + 1023) / 1024;
        m.insert(QS("name"), files.at(i).fileName());
        m.insert(QS("dir"), false);
        m.insert(QS("info"), files.at(i).lastModified().toString(QS("dd.MM")) + QS(" · ") + QString::number(kb) + QS(" КБ"));
        m.insert(QS("sel"), path == checkPath ? QS("1") : (path == actPath ? QS("2") : QString()));
        m.insert(QS("path"), path);
        m_browseItems << m;
        ++fileCount;
    }
    if (m_browseItems.isEmpty()) m_browseNote = QS("ПУСТО");
    else m_browseNote = plural(fileCount, "ТАБЛИЦА", "ТАБЛИЦЫ", "ТАБЛИЦ");
    emit browseChanged();
}

QString Controller::browsePath() const
{
    QString root = Platform::sdRoot();
    QString rel = m_browseDir.startsWith(root) ? m_browseDir.mid(root.size()) : m_browseDir;
    QStringList parts = rel.split(QChar('/'), QString::SkipEmptyParts);
    parts.prepend(QS("SD-карта"));
    return parts.join(QS(" / ")) + QS(" /");
}

bool Controller::browseCanUp() const
{
    return QDir(m_browseDir).absolutePath() != QDir(Platform::sdRoot()).absolutePath();
}

void Controller::browseOpen(int index)
{
    if (index < 0 || index >= m_browseItems.size()) return;
    QVariantMap m = m_browseItems.at(index).toMap();
    QString path = m.value(QS("path")).toString();
    if (m.value(QS("dir")).toBool()) {
        m_browseDir = path;
        listDir();
        return;
    }
    m_lastDir.insert(m_browseKind, m_browseDir);
    m_lastDir.insert(QS("any"), m_browseDir);
    loadFile(m_browseKind, path);
}

void Controller::browseUp()
{
    if (!browseCanUp()) return;
    m_browseDir = QFileInfo(m_browseDir).absolutePath();
    listDir();
}

void Controller::browseCancel()
{
    setScreen(m_prevScreen.isEmpty() || m_prevScreen == QS("files") ? QS("load") : m_prevScreen);
}

/* ------------------------------------------------------------ разбор файлов */

void Controller::loadFile(const QString &kind, const QString &path)
{
    m_pendingKind = kind;
    m_pendingPath = path;
    QString name = QFileInfo(path).fileName();
    if (kind == QS("check")) { m_checkState = QS("busy"); m_checkName = name; m_checkLine = QS("читаю…"); }
    else { m_actState = QS("busy"); m_actName = name; m_actLine = QS("читаю…"); }
    setScreen(QS("load"));
    emit loadChanged();
    // даём интерфейсу отрисовать «читаю…», потом разбираем
    QTimer::singleShot(40, this, SLOT(parsePending()));
}

void Controller::parsePending()
{
    QString kind = m_pendingKind, path = m_pendingPath;
    if (path.isEmpty()) return;
    m_pendingPath.clear();
    bool isCheck = kind == QS("check");
    QString name = QFileInfo(path).fileName();

    Book book;
    QString err;
    bool ok = readBook(path, book, err);
    if (ok && isCheck) {
        CheckFile c;
        ok = parseCheck(book, name, c, err);
        if (ok) {
            m_check = c;
            m_hasCheck = true;
            m_checkState = QS("ok");
            m_checkName = name;
            m_checkLine = plural(c.total, "строка", "строки", "строк") + QS(" · допущено ") + QString::number(c.allowed.size());
            m_lastDir.insert(QS("checkFile"), path);
        }
    } else if (ok) {
        ActFile a;
        ok = parseAct(book, name, a, err);
        if (ok) {
            m_act = a;
            m_hasAct = true;
            m_actState = QS("ok");
            m_actName = name;
            m_actLine = plural(a.knt.size(), "номер", "номера", "номеров") +
                        (a.dupes ? QS(" · дублей ") + QString::number(a.dupes) : QS(" · дублей нет"));
            m_lastDir.insert(QS("actFile"), path);
        }
    }

    if (!ok) {
        if (isCheck) { m_hasCheck = false; m_checkState = QS("err"); m_checkName = name; m_checkLine = QS("ошибка"); }
        else { m_hasAct = false; m_actState = QS("err"); m_actName = name; m_actLine = QS("ошибка"); }
        m_matchReady = false;
        m_loadError = name + QS(" — ") + err;
        Platform::vibrate(150);
        setScreen(QS("load"));
        emit loadChanged();
        return;
    }
    if (m_screen != QS("load")) setScreen(QS("load"));
    runMatch();
}

void Controller::runMatch()
{
    m_loadError.clear();
    if (!m_hasCheck || !m_hasAct) {
        m_matchReady = false;
        emit loadChanged();
        return;
    }
    m_match = match(m_check, m_act);
    if (!m_match.extra.isEmpty()) {
        m_matchReady = false;
        emit blockChanged();
        emit loadChanged();
        Platform::vibrate(150);
        setScreen(QS("block"));
        return;
    }
    m_matchReady = true;
    emit loadChanged();
}

QVariantMap Controller::checkInfo() const
{
    QVariantMap m;
    m.insert(QS("state"), m_checkState);
    m.insert(QS("file"), m_checkName);
    m.insert(QS("fileLines"), wrapLines(m_checkName, FILE_PX, FILE_FONT, true, 2));
    m.insert(QS("line"), m_checkLine);
    return m;
}

QVariantMap Controller::actInfo() const
{
    QVariantMap m;
    m.insert(QS("state"), m_actState);
    m.insert(QS("file"), m_actName);
    m.insert(QS("fileLines"), wrapLines(m_actName, FILE_PX, FILE_FONT, true, 2));
    m.insert(QS("line"), m_actLine);
    return m;
}

QVariantMap Controller::matchInfo() const
{
    QVariantMap m;
    m.insert(QS("ready"), m_matchReady);
    if (!m_matchReady) return m;
    int inWork = m_match.inWork.size();
    m.insert(QS("inWork"), inWork);
    m.insert(QS("total"), m_check.total);
    m.insert(QS("allowed"), m_check.allowed.size());
    m.insert(QS("denied"), m_check.denied.size() - m_check.empty);
    m.insert(QS("empty"), m_check.empty);
    m.insert(QS("act"), m_act.knt.size());

    QVariantList notes;
    if (inWork < m_check.allowed.size()) {
        int rest = m_check.allowed.size() - inWork;
        notes << kv(QS("Выборка сокращена по акту"),
                    QS("В акте ") + QString::number(inWork) + QS(" из ") + QString::number(m_check.allowed.size()) +
                    QS(" допущенных позиций. Ещё ") + plural(rest, "позиция", "позиции", "позиций") +
                    QS(" в работу не берутся."), QS("warn"));
    } else {
        notes << kv(QS("Акт совпадает со списком допущенных"), QS("Лишнего товара в акте нет."), QS("ok"));
    }
    if (m_act.dupes)
        notes << kv(QS("Дубли в акте"), QS("Повторов номеров: ") + QString::number(m_act.dupes) +
                    QS(" — учтены по одному разу."), QS("warn"));
    if (m_check.empty)
        notes << kv(QS("Есть непроверенные строки"), QS("В файле проверки ") +
                    plural(m_check.empty, "строка", "строки", "строк") +
                    QS(" с пустым «Решением» — они считаются недопущенными."), QS("warn"));
    m.insert(QS("notes"), notes);
    return m;
}

QVariantMap Controller::resumeInfo() const
{
    QVariantMap m;
    bool has = m_resumeAvailable && m_active;
    m.insert(QS("has"), has);
    if (has) {
        m.insert(QS("text"), QS("Акт «") + (m_metaAct.isEmpty() ? QS("—") : m_metaAct) + QS("», отмечено ") +
                 QString::number(m_marks.size()) + QS(" из ") + QString::number(m_items.size()) +
                 QS(", таймер ") + hhmmss(elapsedMs()));
        m.insert(QS("done"), m_timer.state == QS("done"));
    }
    return m;
}

/* ------------------------------------------------------------- блокировка */

QVariantList Controller::extraList() const
{
    QVariantList out;
    for (int i = 0; i < m_match.extra.size(); ++i) {
        const Extra &e = m_match.extra.at(i);
        QVariantMap m;
        m.insert(QS("knt"), e.knt);
        m.insert(QS("why"), e.why);
        m.insert(QS("miss"), e.miss);
        out << m;
    }
    return out;
}

QString Controller::extraNote() const
{
    bool allMissing = !m_match.extra.isEmpty() && m_hasAct && m_match.extra.size() == m_act.knt.size();
    for (int i = 0; allMissing && i < m_match.extra.size(); ++i) allMissing = m_match.extra.at(i).miss;
    return allMissing ? QS("Ни один номер из акта не найден в проверке. Похоже, загружен не тот файл или в акте другой столбец с номерами.")
                      : QString();
}

QString Controller::extraAsText() const
{
    QStringList lines;
    lines << QS("Акт списания необходимо исправить: содержит товар, не допущенный к утилизации.")
          << QS("Лишние КНТ (") + QString::number(m_match.extra.size()) + QS("):");
    for (int i = 0; i < m_match.extra.size(); ++i)
        lines << m_match.extra.at(i).knt + QS(" — ") + m_match.extra.at(i).why;
    return lines.join(QS("\n"));
}

void Controller::copyExtra()
{
    QString text = extraAsText();
    Platform::copyText(text);
    if (Platform::ensureOutDir()) {
        QString name = QS("Лишние_КНТ_") + fileStamp(now()) + QS(".txt");
        if (Platform::writeText(Platform::outDir() + QS("/") + name, text)) {
            emit toast(QS("Список скопирован и сохранён: utilization/") + name);
            return;
        }
    }
    emit toast(QS("Список лишних КНТ скопирован"));
}

void Controller::blockBack()
{
    m_hasAct = false;
    m_actState = QS("empty");
    m_actName.clear();
    m_actLine.clear();
    m_matchReady = false;
    emit loadChanged();
    setScreen(QS("load"));
}

/* ------------------------------------------------------------------ сессия */

void Controller::resetSession()
{
    m_active = false;
    m_sessionId.clear();
    m_metaCheck.clear();
    m_metaAct.clear();
    m_createdAt = 0;
    m_headers.clear();
    m_items.clear();
    m_marks.clear();
    m_log.clear();
    m_timer.state = QS("idle");
    m_timer.startedAt = m_timer.anchor = m_timer.accMs = m_timer.pausedMs = m_timer.pauseAnchor = m_timer.finishedAt = 0;
    m_timer.pauses = 0;
    m_cursor = -1;
    m_typed.clear();
    m_find.clear();
    m_fresh = false;
    m_picks.clear();
    m_detailsOpen = false;
    m_sheetOpen = false;
    m_gridRows.clear();
    m_exportNote.clear();
    m_colCode = m_colBrand = m_colPrice = -1;
}

void Controller::computeColumns()
{
    QStringList norm;
    for (int i = 0; i < m_headers.size(); ++i) norm << normHeader(m_headers.at(i));
    m_colCode = columnOf(norm, attrsCode());
    m_colBrand = columnOf(norm, attrsBrand());
    m_colPrice = columnOf(norm, attrsPrice());
}

QString Controller::cellOf(const Item &it, int col) const
{
    return col >= 0 && col < it.cells.size() ? it.cells.at(col) : QString();
}

void Controller::start()
{
    if (!m_matchReady || m_match.inWork.isEmpty()) return;
    resetSession();
    m_active = true;
    m_resumeAvailable = false;
    m_createdAt = now();
    m_sessionId = QString::number(m_createdAt);
    m_metaCheck = m_check.fileName;
    m_metaAct = m_act.fileName;
    m_headers = m_check.headers;
    m_items = m_match.inWork;
    qStableSort(m_items.begin(), m_items.end(), itemLess);
    computeColumns();
    saveItems();
    saveState();
    rebuildGrid();
    emit resumeChanged();
    emit timerChanged();
    emit progressChanged();
    emit workChanged();
    setScreen(QS("work"));
}

void Controller::resume()
{
    if (!m_active) return;
    m_resumeAvailable = false;
    emit resumeChanged();
    rebuildGrid();
    Platform::keepAwake(m_timer.state == QS("run"));
    emit timerChanged();
    emit progressChanged();
    emit workChanged();
    if (m_timer.state == QS("done")) { emit summaryChanged(); setScreen(QS("done")); }
    else setScreen(QS("work"));
}

void Controller::restart()
{
    dropSession();
    resetSession();
    m_resumeAvailable = false;
    m_hasCheck = m_hasAct = m_matchReady = false;
    m_checkState = m_actState = QS("empty");
    m_checkName.clear(); m_checkLine.clear(); m_actName.clear(); m_actLine.clear();
    m_loadError.clear();
    m_match = MatchResult();
    Platform::keepAwake(false);
#ifdef Q_OS_BLACKBERRY
    m_gridModel->clear();
#endif
    emit resumeChanged();
    emit loadChanged();
    emit blockChanged();
    emit timerChanged();
    emit progressChanged();
    emit workChanged();
    emit gridChanged();
    emit summaryChanged();
    setScreen(QS("load"));
}

/* ---------------------------------------------------------- сохранение */

QString Controller::statePath() const
{
    return Platform::sdReady() ? Platform::outDir() + QS("/.session.json") : Platform::dataDir() + QS("/session.json");
}

QString Controller::itemsPath() const
{
    return Platform::sdReady() ? Platform::outDir() + QS("/.session-items.json") : Platform::dataDir() + QS("/session-items.json");
}

void Controller::saveItems()
{
    Platform::ensureOutDir();
    QVariantList items;
    for (int i = 0; i < m_items.size(); ++i) {
        const Item &it = m_items.at(i);
        QVariantList row;
        row << it.knt << it.shortNo << it.name << it.decision << QVariant(listOf(it.cells));
        items << QVariant(row);
    }
    QVariantMap m;
    m.insert(QS("v"), 1);
    m.insert(QS("id"), m_sessionId);
    m.insert(QS("headers"), QVariant(listOf(m_headers)));
    m.insert(QS("items"), items);
    Platform::saveJson(itemsPath(), m);
}

void Controller::saveState()
{
    if (!m_active) return;
    QVariantMap marks;
    QHash<QString, qint64>::const_iterator it = m_marks.constBegin();
    for (; it != m_marks.constEnd(); ++it) marks.insert(it.key(), (double)it.value());
    QVariantList log;
    for (int i = 0; i < m_log.size(); ++i) {
        QVariantList e;
        e << (double)m_log.at(i).ts << m_log.at(i).knt << m_log.at(i).name << m_log.at(i).type;
        log << QVariant(e);
    }
    QVariantMap t;
    t.insert(QS("state"), m_timer.state);
    t.insert(QS("startedAt"), (double)m_timer.startedAt);
    t.insert(QS("anchor"), (double)m_timer.anchor);
    t.insert(QS("accMs"), (double)m_timer.accMs);
    t.insert(QS("pausedMs"), (double)m_timer.pausedMs);
    t.insert(QS("pauseAnchor"), (double)m_timer.pauseAnchor);
    t.insert(QS("finishedAt"), (double)m_timer.finishedAt);
    t.insert(QS("pauses"), m_timer.pauses);

    QVariantMap m;
    m.insert(QS("v"), 1);
    m.insert(QS("id"), m_sessionId);
    m.insert(QS("savedAt"), (double)now());
    m.insert(QS("checkFile"), m_metaCheck);
    m.insert(QS("actFile"), m_metaAct);
    m.insert(QS("createdAt"), (double)m_createdAt);
    m.insert(QS("count"), m_items.size());
    m.insert(QS("marks"), marks);
    m.insert(QS("log"), log);
    m.insert(QS("timer"), t);
    m.insert(QS("cursor"), m_cursor);
    Platform::saveJson(statePath(), m);
}

bool Controller::loadSession()
{
    // сессия может лежать на карте и (если карту вынимали) в песочнице — берём свежую
    QStringList states, itemsFiles;
    states << Platform::outDir() + QS("/.session.json") << Platform::dataDir() + QS("/session.json");
    itemsFiles << Platform::outDir() + QS("/.session-items.json") << Platform::dataDir() + QS("/session-items.json");
    int best = -1;
    double bestAt = -1;
    QVariantMap bestState;
    for (int i = 0; i < states.size(); ++i) {
        bool ok = false;
        QVariantMap st = Platform::loadJson(states.at(i), &ok).toMap();
        if (!ok || st.value(QS("v")).toInt() != 1) continue;
        if (st.value(QS("savedAt")).toDouble() > bestAt) { best = i; bestAt = st.value(QS("savedAt")).toDouble(); bestState = st; }
    }
    if (best < 0) return false;

    bool ok = false;
    QVariantMap itemsMap;
    for (int i = 0; i < itemsFiles.size() && !ok; ++i) {
        QVariantMap m = Platform::loadJson(itemsFiles.at(i), &ok).toMap();
        if (ok && m.value(QS("id")).toString() == bestState.value(QS("id")).toString()) itemsMap = m;
        else ok = false;
    }
    if (!ok) return false;

    resetSession();
    m_sessionId = bestState.value(QS("id")).toString();
    m_metaCheck = bestState.value(QS("checkFile")).toString();
    m_metaAct = bestState.value(QS("actFile")).toString();
    m_createdAt = (qint64)bestState.value(QS("createdAt")).toDouble();
    m_headers = itemsMap.value(QS("headers")).toStringList();
    QVariantList items = itemsMap.value(QS("items")).toList();
    for (int i = 0; i < items.size(); ++i) {
        QVariantList row = items.at(i).toList();
        if (row.size() < 5) continue;
        Item it;
        it.knt = row.at(0).toString();
        it.shortNo = row.at(1).toString();
        it.name = row.at(2).toString();
        it.decision = row.at(3).toString();
        it.cells = row.at(4).toStringList();
        m_items << it;
    }
    if (m_items.isEmpty() || m_items.size() != bestState.value(QS("count")).toInt()) { resetSession(); return false; }

    QVariantMap marks = bestState.value(QS("marks")).toMap();
    for (QVariantMap::const_iterator it = marks.constBegin(); it != marks.constEnd(); ++it)
        m_marks.insert(it.key(), (qint64)it.value().toDouble());
    QVariantList log = bestState.value(QS("log")).toList();
    for (int i = 0; i < log.size(); ++i) {
        QVariantList e = log.at(i).toList();
        if (e.size() < 4) continue;
        LogEntry le;
        le.ts = (qint64)e.at(0).toDouble();
        le.knt = e.at(1).toString();
        le.name = e.at(2).toString();
        le.type = e.at(3).toString();
        m_log << le;
    }
    QVariantMap t = bestState.value(QS("timer")).toMap();
    m_timer.state = t.value(QS("state"), QS("idle")).toString();
    m_timer.startedAt = (qint64)t.value(QS("startedAt")).toDouble();
    m_timer.anchor = (qint64)t.value(QS("anchor")).toDouble();
    m_timer.accMs = (qint64)t.value(QS("accMs")).toDouble();
    m_timer.pausedMs = (qint64)t.value(QS("pausedMs")).toDouble();
    m_timer.pauseAnchor = (qint64)t.value(QS("pauseAnchor")).toDouble();
    m_timer.finishedAt = (qint64)t.value(QS("finishedAt")).toDouble();
    m_timer.pauses = t.value(QS("pauses")).toInt();
    m_cursor = bestState.value(QS("cursor"), -1).toInt();
    if (m_cursor >= m_items.size()) m_cursor = -1;
    computeColumns();
    m_active = true;
    return true;
}

void Controller::dropSession()
{
    QFile::remove(Platform::outDir() + QS("/.session.json"));
    QFile::remove(Platform::outDir() + QS("/.session-items.json"));
    QFile::remove(Platform::dataDir() + QS("/session.json"));
    QFile::remove(Platform::dataDir() + QS("/session-items.json"));
}

/* ------------------------------------------------------------------ таймер */

qint64 Controller::elapsedMs() const
{
    return m_timer.accMs + (m_timer.state == QS("run") ? now() - m_timer.anchor : 0);
}

qint64 Controller::pausedTotal() const
{
    return m_timer.pausedMs + (m_timer.state == QS("pause") ? now() - m_timer.pauseAnchor : 0);
}

QString Controller::timerText() const { return hhmmss(elapsedMs()); }

QString Controller::timerLine() const
{
    QStringList line;
    if (m_timer.startedAt) line << QS("старт ") + clockOf(m_timer.startedAt);
    line << QS("пауз ") + QString::number(m_timer.pauses);
    if (m_timer.pauses || m_timer.state == QS("pause")) line << QS("в паузе ") + hhmmss(pausedTotal());
    if (m_timer.finishedAt) line << QS("финиш ") + clockOf(m_timer.finishedAt);
    return line.join(QS(" · "));
}

void Controller::timerChangedInternal()
{
    saveState();
    Platform::keepAwake(m_timer.state == QS("run"));
    emit timerChanged();
    emit workChanged();
}

void Controller::timerToggle()
{
    if (!m_active) return;
    qint64 t = now();
    if (m_timer.state == QS("idle")) {
        m_timer.state = QS("run");
        m_timer.startedAt = t;
        m_timer.anchor = t;
    } else if (m_timer.state == QS("run")) {
        m_timer.accMs += t - m_timer.anchor;
        m_timer.anchor = 0;
        m_timer.pauseAnchor = t;
        m_timer.pauses++;
        m_timer.state = QS("pause");
    } else if (m_timer.state == QS("pause")) {
        m_timer.pausedMs += t - m_timer.pauseAnchor;
        m_timer.pauseAnchor = 0;
        m_timer.anchor = t;
        m_timer.state = QS("run");
    } else {
        return;
    }
    timerChangedInternal();
}

void Controller::requestFinish()
{
    if (m_timer.state != QS("run") && m_timer.state != QS("pause")) {
        emit toast(QS("Таймер ещё не запущен"));
        return;
    }
    int left = m_items.size() - m_marks.size();
    QString body = left ? QS("Не отмечено позиций: ") + QString::number(left) +
                          QS(". Таймер остановится, вернуться к отметкам будет нельзя.")
                        : QS("Весь товар отмечен. Таймер остановится.");
    emit confirm(QS("finish"), QS("Завершить утилизацию?"), body, QS("Завершить"));
}

void Controller::finish()
{
    qint64 t = now();
    if (m_timer.state == QS("run")) m_timer.accMs += t - m_timer.anchor;
    if (m_timer.state == QS("pause")) m_timer.pausedMs += t - m_timer.pauseAnchor;
    m_timer.anchor = 0;
    m_timer.pauseAnchor = 0;
    m_timer.state = QS("done");
    m_timer.finishedAt = t;
    m_sheetOpen = false;
    m_exportNote.clear();
    timerChangedInternal();
    emit summaryChanged();
    emit gridChanged();
    setScreen(QS("done"));
}

/* ------------------------------------------------------------ ввод номера */

void Controller::key(const QString &digit)
{
    if (!m_active || digit.size() != 1 || !digit.at(0).isDigit()) return;
    if (m_fresh) { m_typed.clear(); m_fresh = false; }
    if (m_typed.size() >= 12) return;
    m_typed += digit;
    m_find.clear();
    m_picks.clear();
    if (m_typed.size() >= 4) doFind(true);
    emit workChanged();
}

void Controller::backspace()
{
    m_fresh = false;
    if (m_typed.isEmpty()) return;
    m_typed.chop(1);
    m_find.clear();
    m_picks.clear();
    if (m_typed.size() >= 4) doFind(true);
    emit workChanged();
}

void Controller::clearInput()
{
    m_typed.clear();
    m_fresh = false;
    m_find.clear();
    m_picks.clear();
    emit workChanged();
}

void Controller::find()
{
    doFind(false);
    emit workChanged();
}

void Controller::doFind(bool autoMode)
{
    m_picks.clear();
    if (m_typed.size() < 2) {
        m_find = autoMode ? QString() : QS("short");
        if (!autoMode) Platform::vibrate(120);
        return;
    }
    QList<int> found = candidates(m_items, m_typed);
    if (found.isEmpty()) {
        m_find = QS("notfound");
        m_fresh = true;          // следующая цифра начнёт новый номер
        Platform::vibrate(150);
        return;
    }
    if (found.size() == 1) {
        m_find.clear();
        openItem(found.first());
        m_fresh = true;
        return;
    }
    m_find = QS("multi");
    m_picks = found;
}

void Controller::pick(int itemIndex)
{
    m_typed.clear();
    m_fresh = false;
    openItem(itemIndex);
    emit workChanged();
}

void Controller::keyPress(int code, const QString &text)
{
    if (m_screen != QS("work") && m_screen != QS("grid")) return;
    if (text.size() == 1 && text.at(0).isDigit()) {
        if (m_screen == QS("work")) key(text);
        return;
    }
    if (code == 0xF008 || code == 8) { if (m_screen == QS("work")) backspace(); return; }
    if (code == 0xF00D || code == 0xF08D || code == 13 || code == 10) { if (m_screen == QS("work")) find(); return; }
    if (code == 32 || text == QS(" ")) {
        if (m_screen == QS("work") || m_sheetOpen) mark();
        return;
    }
}

void Controller::openItem(int idx, bool fromGrid)
{
    Q_UNUSED(fromGrid);
    if (idx < 0 || idx >= m_items.size()) return;
    int old = m_cursor;
    m_cursor = idx;
    m_find.clear();
    m_picks.clear();
    m_detailsOpen = false;
    saveState();
    refreshGridItem(old);
    refreshGridItem(idx);
    emit workChanged();
}

QVariantMap Controller::card() const
{
    QVariantMap m;
    bool has = m_active && m_cursor >= 0 && m_cursor < m_items.size() && m_find.isEmpty();
    // пока набирается другой номер, старую карточку не показываем
    if (has && !m_typed.isEmpty() && !m_fresh) has = false;
    m.insert(QS("has"), has);
    if (!has) return m;

    const Item &it = m_items.at(m_cursor);
    int tail = (!m_typed.isEmpty() && it.knt.endsWith(m_typed)) ? m_typed.size() : qMin(4, it.knt.size());
    m.insert(QS("idx"), m_cursor);
    m.insert(QS("knt"), it.knt);
    m.insert(QS("head"), it.knt.left(it.knt.size() - tail));
    m.insert(QS("tail"), it.knt.right(tail));
    m.insert(QS("name"), it.name.isEmpty() ? QS("(наименование не заполнено)") : it.name);
    m.insert(QS("name2"), wrapLines(it.name.isEmpty() ? QS("(наименование не заполнено)") : it.name,
                                    NAME_PX, NAME_FONT, false, 2));
    QVariantList tags;
    QString code = cellOf(it, m_colCode), brand = cellOf(it, m_colBrand), price = cellOf(it, m_colPrice);
    if (!code.isEmpty()) tags << kv(QS("КОД"), code, QS("hi"));
    if (!brand.isEmpty()) tags << kv(QS("ТМ"), brand, QS("hi"));
    if (!price.isEmpty()) tags << kv(QS("КИС"), price);
    m.insert(QS("tags"), tags);
    bool marked = m_marks.contains(it.knt);
    m.insert(QS("marked"), marked);
    m.insert(QS("markedAt"), marked ? clockOf(m_marks.value(it.knt)) : QString());
    m.insert(QS("pos"), QString::number(m_cursor + 1) + QS("/") + QString::number(m_items.size()));
    QString action = marked ? QS("unmark") : (m_timer.state == QS("run") ? QS("mark") : QS("off"));
    m.insert(QS("action"), action);
    return m;
}

QVariantMap Controller::status() const
{
    QVariantMap m;
    QString kind, text, hint;
    QVariantMap c = card();
    if (m_find == QS("notfound")) {
        kind = QS("err"); text = m_typed + QS(" — НЕТ В ВЫБОРКЕ"); hint = QS("НАБЕРИТЕ ЗАНОВО");
    } else if (m_find == QS("multi")) {
        kind = QS("warn"); text = QS("СОВПАЛО ") + upperWords(plural(m_picks.size(), "позиция", "позиции", "позиций"));
        hint = QS("ВЫБЕРИТЕ");
    } else if (m_find == QS("short")) {
        kind = QS("err"); text = QS("ВВЕДИТЕ МИНИМУМ 2 ЦИФРЫ");
    } else if (c.value(QS("has")).toBool()) {
        if (c.value(QS("marked")).toBool()) {
            kind = QS("done"); text = QS("УТИЛИЗИРОВАН ") + c.value(QS("markedAt")).toString();
        } else if (m_timer.state == QS("idle")) {
            kind = QS("warn"); text = QS("СНАЧАЛА НАЖМИТЕ «СТАРТ»");
        } else if (m_timer.state == QS("pause")) {
            kind = QS("warn"); text = QS("ПАУЗА — ПРОДОЛЖИТЕ ТАЙМЕР");
        } else if (m_timer.state == QS("done")) {
            kind = QS("idle"); text = QS("ПРОЦЕСС ЗАВЕРШЁН");
        } else {
            kind = QS("ok"); text = QS("ГОТОВ К ОТМЕТКЕ"); hint = QS("ПРОБЕЛ — ОТМЕТИТЬ");
        }
    } else if (m_typed.isEmpty()) {
        kind = QS("idle"); text = QS("ВВЕДИТЕ 4 ПОСЛЕДНИЕ ЦИФРЫ КНТ");
    } else if (m_typed.size() < 4) {
        int n = 4 - m_typed.size();
        kind = QS("idle"); text = QS("ЕЩЁ ") + upperWords(plural(n, "цифра", "цифры", "цифр"));
        hint = QS("ENTER — ИСКАТЬ");
    } else {
        kind = QS("idle"); text = QS("ПОИСК…");
    }
    m.insert(QS("kind"), kind);
    m.insert(QS("text"), text);
    m.insert(QS("hint"), hint);
    return m;
}

QVariantList Controller::picks() const
{
    QVariantList out;
    for (int i = 0; i < m_picks.size(); ++i) {
        const Item &it = m_items.at(m_picks.at(i));
        QVariantMap m;
        m.insert(QS("idx"), m_picks.at(i));
        m.insert(QS("knt"), it.knt);
        m.insert(QS("name"), it.name);
        m.insert(QS("done"), m_marks.contains(it.knt));
        out << m;
    }
    return out;
}

QVariantList Controller::details() const
{
    QVariantList out;
    if (m_cursor < 0 || m_cursor >= m_items.size()) return out;
    const Item &it = m_items.at(m_cursor);
    for (int i = 0; i < m_headers.size() && i < it.cells.size(); ++i) {
        if (it.cells.at(i).isEmpty() || m_headers.at(i).isEmpty()) continue;
        out << kv(wrapLines(m_headers.at(i), DETAIL_PX, DETAIL_KEY_FONT, true, 3),
                  wrapLines(it.cells.at(i), DETAIL_PX, DETAIL_FONT, false, 60));
    }
    return out;
}

void Controller::toggleDetails()
{
    if (!card().value(QS("has")).toBool()) { m_detailsOpen = false; emit workChanged(); return; }
    m_detailsOpen = !m_detailsOpen;
    emit workChanged();
}

/* --------------------------------------------------------------- отметки */

void Controller::mark()
{
    if (m_cursor < 0 || m_cursor >= m_items.size()) return;
    const Item &it = m_items.at(m_cursor);
    if (m_timer.state != QS("run")) { Platform::vibrate(150); emit toast(QS("Таймер не запущен")); return; }
    if (m_marks.contains(it.knt)) { emit toast(QS("Уже утилизирован")); return; }

    qint64 ts = now();
    m_marks.insert(it.knt, ts);
    LogEntry e;
    e.ts = ts; e.knt = it.knt; e.name = it.name; e.type = QS("mark");
    m_log.prepend(e);
    saveState();
    m_typed.clear();
    m_fresh = false;
    m_find.clear();
    Platform::vibrate(60);
    if (m_marks.size() == m_items.size()) emit toast(QS("Весь товар из акта утилизирован — можно завершать"));
    else emit toast(QS("Отмечено · ") + (it.name.isEmpty() ? it.knt : it.name.left(46)));
    refreshGridItem(m_cursor);
    if (m_screen == QS("grid") && m_sheetOpen) {
        // в сетке шторка сама закрывается, чтобы сразу брать следующую плитку
        m_closeSeq = m_sheetSeq;
        QTimer::singleShot(700, this, SLOT(autoCloseSheet()));
    }
    emit progressChanged();
    emit workChanged();
}

void Controller::primary()
{
    QVariantMap c = card();
    if (!c.value(QS("has")).toBool()) return;
    if (c.value(QS("marked")).toBool()) requestUnmark();
    else mark();
}

void Controller::requestUnmark()
{
    if (m_cursor < 0 || m_cursor >= m_items.size()) return;
    const Item &it = m_items.at(m_cursor);
    if (!m_marks.contains(it.knt)) return;
    emit confirm(QS("unmark"), QS("Снять отметку?"),
                 QS("КНТ ") + it.knt + QS(" перестанет считаться утилизированным. Снятие попадёт в журнал и в итоговый отчёт."),
                 QS("Снять"));
}

void Controller::unmark()
{
    if (m_cursor < 0 || m_cursor >= m_items.size()) return;
    const Item &it = m_items.at(m_cursor);
    if (!m_marks.contains(it.knt)) return;
    m_marks.remove(it.knt);
    LogEntry e;
    e.ts = now(); e.knt = it.knt; e.name = it.name; e.type = QS("undo");
    m_log.prepend(e);
    saveState();
    emit toast(QS("Отметка снята"));
    refreshGridItem(m_cursor);
    emit progressChanged();
    emit workChanged();
}

void Controller::nextTodo()
{
    int n = m_items.size();
    if (!n) return;
    int start = m_cursor < 0 ? -1 : m_cursor;
    for (int step = 1; step <= n; ++step) {
        int idx = (start + step) % n;
        if (!m_marks.contains(m_items.at(idx).knt)) {
            m_typed.clear();
            m_fresh = false;
            openItem(idx);
            return;
        }
    }
    emit toast(QS("Неутилизированных позиций не осталось"));
}

void Controller::prev()
{
    int n = m_items.size();
    if (!n) return;
    int i = m_cursor < 0 ? 0 : m_cursor;
    m_typed.clear();
    m_fresh = false;
    openItem((i - 1 + n) % n);
}

void Controller::next()
{
    int n = m_items.size();
    if (!n) return;
    int i = m_cursor < 0 ? -1 : m_cursor;
    m_typed.clear();
    m_fresh = false;
    openItem((i + 1) % n);
}

/* ------------------------------------------------------------------- сетка */

QVariantMap Controller::gridEntry(int idx) const
{
    const Item &it = m_items.at(idx);
    QVariantMap m;
    m.insert(QS("idx"), idx);
    m.insert(QS("sn"), it.knt.right(4));   // на плитке всегда 4 последние цифры полного номера
    m.insert(QS("name"), it.name);
    QString st = QS("left");
    if (m_marks.contains(it.knt)) {
        st = QS("done");
    } else {
        for (int i = 0; i < m_log.size(); ++i) {
            if (m_log.at(i).knt == it.knt) { if (m_log.at(i).type == QS("undo")) st = QS("undo"); break; }
        }
    }
    m.insert(QS("st"), st);
    m.insert(QS("cur"), idx == m_cursor);
    return m;
}

void Controller::rebuildGrid()
{
    m_gridRows.clear();
    QVariantList entries;
    for (int i = 0; i < m_items.size(); ++i) {
        if (!m_gridShowDone && m_marks.contains(m_items.at(i).knt)) continue;
        m_gridRows << i;
        entries << gridEntry(i);
    }
#ifdef Q_OS_BLACKBERRY
    m_gridModel->clear();
    m_gridModel->append(entries);
#endif
    emit gridChanged();
}

void Controller::refreshGridItem(int idx)
{
    if (idx < 0 || idx >= m_items.size()) return;
    bool visible = m_gridShowDone || !m_marks.contains(m_items.at(idx).knt);
    int row = m_gridRows.indexOf(idx);
    if (row >= 0 && visible) {
#ifdef Q_OS_BLACKBERRY
        m_gridModel->replace(row, gridEntry(idx));
#endif
    } else if (row >= 0) {
        m_gridRows.removeAt(row);
#ifdef Q_OS_BLACKBERRY
        m_gridModel->removeAt(row);
#endif
    } else if (visible) {
        int pos = 0;
        while (pos < m_gridRows.size() && m_gridRows.at(pos) < idx) ++pos;
        m_gridRows.insert(pos, idx);
#ifdef Q_OS_BLACKBERRY
        m_gridModel->insert(pos, gridEntry(idx));
#endif
    }
    emit gridChanged();
}

QVariantList Controller::gridSnapshot() const
{
    QVariantList out;
    for (int i = 0; i < m_gridRows.size(); ++i) out << gridEntry(m_gridRows.at(i));
    return out;
}

void Controller::openGrid()
{
    if (!m_active) return;
    m_sheetOpen = false;
    m_detailsOpen = false;
    rebuildGrid();
    setScreen(QS("grid"));
}

void Controller::closeGrid()
{
    m_sheetOpen = false;
    emit gridChanged();
    emit workChanged();
    setScreen(QS("work"));
}

void Controller::gridToggleDone()
{
    m_gridShowDone = !m_gridShowDone;
    rebuildGrid();
}

void Controller::gridOpen(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= m_items.size()) return;
    m_typed.clear();
    m_fresh = false;
    openItem(itemIndex, true);
    m_sheetOpen = true;
    ++m_sheetSeq;
    emit gridChanged();
}

void Controller::autoCloseSheet()
{
    if (m_sheetOpen && m_closeSeq == m_sheetSeq) sheetClose();
}

void Controller::sheetClose()
{
    m_sheetOpen = false;
    emit gridChanged();
}

/* -------------------------------------------------------------------- итог */

QVariantList Controller::receipt() const
{
    QVariantList rows;
    if (!m_active) return rows;
    int total = m_items.size(), done = m_marks.size(), left = total - done;
    qint64 fin = m_timer.finishedAt ? m_timer.finishedAt : now();
    rows << kv(QS("УТИЛИЗАЦИЯ КНТ"), dateOf(fin), QS("head"));
    rows << kv(QS("АКТ"), m_metaAct, QS("dim"));
    rows << kv(QString(), QString(), QS("sep"));
    rows << kv(QS("К УТИЛИЗАЦИИ"), QString::number(total));
    rows << kv(QS("УТИЛИЗИРОВАНО"), QString::number(done), QS("amber"));
    rows << kv(QS("ОСТАЛОСЬ"), QString::number(left), left ? QS("red") : QString());
    rows << kv(QS("ЧИСТОЕ ВРЕМЯ"), hhmmss(elapsedMs()));
    rows << kv(QS("НАЧАЛО — КОНЕЦ"), (m_timer.startedAt ? clockOf(m_timer.startedAt).left(5) : QS("—")) + QS(" — ") +
                                  (m_timer.finishedAt ? clockOf(m_timer.finishedAt).left(5) : QS("—")));
    rows << kv(QS("ПАУЗ / В ПАУЗЕ"), QString::number(m_timer.pauses) + QS(" / ") + hhmmss(pausedTotal()));
    int undos = 0;
    for (int i = 0; i < m_log.size(); ++i) if (m_log.at(i).type == QS("undo")) ++undos;
    rows << kv(QS("СНЯТО ОТМЕТОК"), QString::number(undos));
    if (left) {
        rows << kv(QString(), QString(), QS("sep"));
        rows << kv(QS("НЕ ОТМЕЧЕНО:"), QString(), QS("dim"));
        for (int i = 0; i < m_items.size(); ++i)
            if (!m_marks.contains(m_items.at(i).knt)) rows << kv(m_items.at(i).knt, m_items.at(i).name, QS("rest"));
    }
    if (undos) {
        rows << kv(QString(), QString(), QS("sep"));
        rows << kv(QS("СНЯТЫЕ ОТМЕТКИ:"), QString(), QS("dim"));
        for (int i = 0; i < m_log.size(); ++i)
            if (m_log.at(i).type == QS("undo"))
                rows << kv(m_log.at(i).knt, clockOf(m_log.at(i).ts) + QS(" · ") + m_log.at(i).name, QS("undo"));
    }
    rows << kv(QString(), QString(), QS("sep"));
    return rows;
}

QString Controller::doneAsText() const
{
    int total = m_items.size(), done = m_marks.size();
    QStringList lines;
    lines << QS("Утилизация КНТ · ") + stampOf(m_timer.finishedAt ? m_timer.finishedAt : now())
          << QS("Акт: ") + m_metaAct
          << QS("Утилизировано: ") + QString::number(done) + QS(" из ") + QString::number(total)
          << QS("Чистое время: ") + hhmmss(elapsedMs()) + QS(" · пауз ") + QString::number(m_timer.pauses) +
                 QS(" · в паузе ") + hhmmss(pausedTotal());
    QStringList rest;
    for (int i = 0; i < m_items.size(); ++i)
        if (!m_marks.contains(m_items.at(i).knt)) rest << m_items.at(i).knt + QS(" — ") + m_items.at(i).name;
    if (!rest.isEmpty()) {
        lines << QString() << QS("Не отмечено (") + QString::number(rest.size()) + QS("):");
        lines << rest;
    }
    QStringList undos;
    for (int i = 0; i < m_log.size(); ++i)
        if (m_log.at(i).type == QS("undo"))
            undos << stampOf(m_log.at(i).ts) + QS(" · ") + m_log.at(i).knt + QS(" — ") + m_log.at(i).name;
    if (!undos.isEmpty()) {
        lines << QString() << QS("Снятые отметки (") + QString::number(undos.size()) + QS("):");
        lines << undos;
    }
    return lines.join(QS("\n"));
}

QString Controller::restText() const
{
    QStringList lines;
    int n = 0;
    for (int i = 0; i < m_items.size(); ++i)
        if (!m_marks.contains(m_items.at(i).knt)) { lines << m_items.at(i).knt + QS(" — ") + m_items.at(i).name; ++n; }
    lines.prepend(QS("Не утилизировано (") + QString::number(n) + QS("):"));
    return lines.join(QS("\n"));
}

void Controller::exportXlsx()
{
    if (!m_active) return;
    if (!Platform::ensureOutDir()) {
        m_exportNote = QS("НЕТ SD-КАРТЫ — ВСТАВЬТЕ КАРТУ");
        emit summaryChanged();
        emit toast(QS("SD-карта не найдена"));
        return;
    }
    const Timer &t = m_timer;
    QStringList headers = m_headers;
    headers << QS("Утилизирован") << QS("Время отметки");

    // итог процесса дублируется над таблицей: лист «Итог» открывают не всегда
    QList<QVariantList> aoa;
    aoa << (QVariantList() << QS("Утилизация КНТ") << m_metaAct);
    aoa << (QVariantList() << QS("Чистое время процесса") << hhmmss(elapsedMs()));
    aoa << (QVariantList() << QS("Утилизировано") << QString::number(m_marks.size()) + QS(" из ") + QString::number(m_items.size()));
    aoa << (QVariantList() << QS("Начало / окончание")
                           << (t.startedAt ? stampOf(t.startedAt) : QS("—")) + QS(" — ") + (t.finishedAt ? stampOf(t.finishedAt) : QS("—")));
    aoa << (QVariantList() << QS("Пауз / время в паузе") << QString::number(t.pauses) + QS(" · ") + hhmmss(pausedTotal()));
    aoa << QVariantList();
    QVariantList head;
    for (int i = 0; i < headers.size(); ++i) head << headers.at(i);
    aoa << head;
    QList<int> widths;
    for (int i = 0; i < headers.size(); ++i) widths << qMax(10, headers.at(i).size() + 2);
    for (int i = 0; i < m_items.size(); ++i) {
        const Item &it = m_items.at(i);
        QVariantList row;
        for (int c = 0; c < m_headers.size(); ++c) {
            QString v = c < it.cells.size() ? it.cells.at(c) : QString();
            row << v;
            if (c < widths.size() && v.size() + 2 > widths.at(c)) widths[c] = qMin(60, v.size() + 2);
        }
        bool marked = m_marks.contains(it.knt);
        row << (marked ? QS("+") : QString()) << (marked ? stampOf(m_marks.value(it.knt)) : QString());
        aoa << row;
    }
    widths[widths.size() - 1] = 21;

    QList<QVariantList> sum;
    sum << (QVariantList() << QS("Показатель") << QS("Значение"));
    sum << (QVariantList() << QS("Файл проверки") << m_metaCheck);
    sum << (QVariantList() << QS("Акт списания") << m_metaAct);
    sum << (QVariantList() << QS("К утилизации, позиций") << (double)m_items.size());
    sum << (QVariantList() << QS("Утилизировано") << (double)m_marks.size());
    sum << (QVariantList() << QS("Не отмечено") << (double)(m_items.size() - m_marks.size()));
    sum << (QVariantList() << QS("Начало") << (t.startedAt ? stampOf(t.startedAt) : QString()));
    sum << (QVariantList() << QS("Окончание") << (t.finishedAt ? stampOf(t.finishedAt) : QString()));
    sum << (QVariantList() << QS("Чистое время процесса") << hhmmss(elapsedMs()));
    sum << (QVariantList() << QS("Пауз") << (double)t.pauses);
    sum << (QVariantList() << QS("Время в паузе") << hhmmss(pausedTotal()));
    sum << QVariantList();
    bool anyUndo = false;
    for (int i = 0; i < m_log.size(); ++i) {
        if (m_log.at(i).type != QS("undo")) continue;
        if (!anyUndo) {
            sum << (QVariantList() << QS("Снятые отметки") << QString());
            sum << (QVariantList() << QS("Время") << QS("№ КНТ") << QS("Наименование"));
            anyUndo = true;
        }
        sum << (QVariantList() << stampOf(m_log.at(i).ts) << m_log.at(i).knt << m_log.at(i).name);
    }

    XlsxWriter w;
    w.addSheet(QS("Утилизация"), aoa, widths);
    w.addSheet(QS("Итог"), sum, QList<int>() << 26 << 34 << 40);
    QString name = QS("Утилизация_КНТ_") + fileStamp(now()) + QS(".xlsx");
    QString err;
    if (!w.save(Platform::outDir() + QS("/") + name, err)) {
        m_exportNote = QS("ОШИБКА ЗАПИСИ: ") + err;
        emit summaryChanged();
        emit toast(QS("Не удалось записать файл: ") + err);
        return;
    }
    m_exportNote = QS("ЗАПИСАНО: SD/utilization/") + name;
    emit summaryChanged();
    emit toast(QS("Файл выгружен: ") + name);
}

void Controller::exportText()
{
    if (!m_active) return;
    QString text = doneAsText();
    Platform::copyText(text);
    if (!Platform::ensureOutDir()) {
        m_exportNote = QS("ТЕКСТ СКОПИРОВАН · SD-КАРТЫ НЕТ");
        emit summaryChanged();
        emit toast(QS("Итог скопирован, но SD-карта не найдена"));
        return;
    }
    QString name = QS("Итог_КНТ_") + fileStamp(now()) + QS(".txt");
    if (!Platform::writeText(Platform::outDir() + QS("/") + name, text)) {
        m_exportNote = QS("ОШИБКА ЗАПИСИ ТЕКСТА");
        emit summaryChanged();
        emit toast(QS("Итог скопирован, но файл не записан"));
        return;
    }
    m_exportNote = QS("ЗАПИСАНО: SD/utilization/") + name + QS(" · СКОПИРОВАНО");
    emit summaryChanged();
    emit toast(QS("Итог сохранён и скопирован: ") + name);
}

/* ---------------------------------------------------------- подтверждения */

void Controller::requestRestart()
{
    emit confirm(QS("restart"), QS("Начать заново?"),
                 QS("Текущая выборка, отметки и таймер будут удалены. Отменить это нельзя."), QS("Начать заново"));
}

void Controller::confirmed(const QString &action)
{
    if (action == QS("unmark")) unmark();
    else if (action == QS("finish")) finish();
    else if (action == QS("restart")) restart();
}
