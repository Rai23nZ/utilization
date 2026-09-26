#include "Logic.hpp"

#include <QDateTime>
#include <QSet>

using namespace BookUtil;

namespace Logic {

static const char *const ALIAS_KNT_FULL[] = { "№ кнт", "№кнт", "no кнт", "n кнт", "# кнт", "номер кнт",
                                              "номер кнт (полный)", 0 };
static const char *const ALIAS_KNT_SHORT[] = { "кнт", 0 };
static const char *const ALIAS_DECISION[] = { "решение", "решение комиссии", 0 };
const char *const ALIAS_NAME[] = { "наименование товара", "наименование", "товар", 0 };

QStringList aliases(const char *const *list)
{
    QStringList out;
    for (int i = 0; list[i]; ++i) out << QString::fromUtf8(list[i]);
    return out;
}

QStringList attrsCode() { return QStringList() << QS("код товара"); }
QStringList attrsBrand() { return QStringList() << QS("торговая марка"); }
QStringList attrsPrice() { return QStringList() << QS("цена по каталогу кис") << QS("цена кис"); }

/* «НЕ ДОПУЩЕН» не должен пройти по подстроке «ДОПУЩЕН» — проверяем начало. */
bool isAllowed(const QString &decision)
{
    QString up = decision;
    up.replace(QChar(0x00A0), QChar(' '));
    up = up.trimmed().toUpper();
    up.replace(QChar(0x0401), QChar(0x0415));   // Ё → Е
    if (up.isEmpty()) return false;
    QString flat;
    for (int i = 0; i < up.size(); ++i) if (!up.at(i).isSpace()) flat.append(up.at(i));
    if (flat.startsWith(QS("НЕДОПУЩ"))) return false;
    if (flat == QS("НЕТ")) return false;
    return flat.contains(QS("ДОПУЩ"));
}

int columnOf(const QStringList &norm, const QStringList &al)
{
    for (int i = 0; i < norm.size(); ++i) if (al.contains(norm.at(i))) return i;
    return -1;
}

struct Header { int index; QStringList headers; QStringList norm; };

/* Ищет строку шапки в первых 30 строках: каждый набор синонимов обязан найтись. */
static bool findHeader(const Rows &rows, const QList<QStringList> &need, Header &out)
{
    int limit = qMin(rows.size(), 30);
    for (int i = 0; i < limit; ++i) {
        QStringList norm, headers;
        const QVariantList &row = rows.at(i);
        for (int c = 0; c < row.size(); ++c) { norm << normHeader(row.at(c)); headers << cellText(row.at(c)); }
        bool ok = true;
        for (int k = 0; k < need.size() && ok; ++k) {
            bool found = false;
            for (int c = 0; c < norm.size() && !found; ++c) found = need.at(k).contains(norm.at(c));
            ok = found;
        }
        if (ok) { out.index = i; out.headers = headers; out.norm = norm; return true; }
    }
    return false;
}

static QString firstRowText(const Rows &rows)
{
    QStringList seen;
    if (!rows.isEmpty())
        for (int c = 0; c < rows.at(0).size(); ++c) {
            QString t = cellText(rows.at(0).at(c));
            if (!t.isEmpty()) seen << t;
        }
    return seen.isEmpty() ? QS("—") : seen.join(QS(" · "));
}

bool parseCheck(const Book &book, const QString &fileName, CheckFile &out, QString &error)
{
    QStringList full = aliases(ALIAS_KNT_FULL), shortA = aliases(ALIAS_KNT_SHORT), dec = aliases(ALIAS_DECISION);
    Header head;
    if (!findHeader(book.rows, QList<QStringList>() << dec << (full + shortA), head)) {
        error = QS("Не нашёл столбцы «Решение» и «№ КНТ». Заголовки первой строки: ") + firstRowText(book.rows);
        return false;
    }
    int cDec = columnOf(head.norm, dec);
    int cKnt = columnOf(head.norm, full);
    if (cKnt < 0) cKnt = columnOf(head.norm, shortA);
    int cShort = -1;
    for (int i = 0; i < head.norm.size(); ++i)
        if (i != cKnt && shortA.contains(head.norm.at(i))) { cShort = i; break; }
    int cName = columnOf(head.norm, aliases(ALIAS_NAME));

    out = CheckFile();
    out.fileName = fileName;
    out.sheet = book.sheet;
    out.headers = head.headers;
    out.headerRow = head.index + 1;
    out.empty = 0;
    out.noKnt = 0;
    for (int r = head.index + 1; r < book.rows.size(); ++r) {
        const QVariantList &row = book.rows.at(r);
        QString knt = digits(cKnt < row.size() ? cellText(row.at(cKnt)) : QString());
        QString decision = cDec < row.size() ? cellText(row.at(cDec)) : QString();
        if (knt.isEmpty()) {
            for (int c = 0; c < row.size(); ++c) if (!cellText(row.at(c)).isEmpty()) { ++out.noKnt; break; }
            continue;
        }
        Item it;
        it.knt = knt;
        for (int c = 0; c < head.headers.size(); ++c) it.cells << (c < row.size() ? cellText(row.at(c)) : QString());
        it.shortNo = cShort >= 0 ? digits(cShort < row.size() ? cellText(row.at(cShort)) : QString()) : knt.right(4);
        it.name = cName >= 0 && cName < row.size() ? cellText(row.at(cName)) : QString();
        it.decision = decision;
        if (isAllowed(decision)) out.allowed << it;
        else { out.denied << it; if (decision.isEmpty()) ++out.empty; }
    }
    out.total = out.allowed.size() + out.denied.size();
    return true;
}

bool parseAct(const Book &book, const QString &fileName, ActFile &out, QString &error)
{
    QStringList full = aliases(ALIAS_KNT_FULL), shortA = aliases(ALIAS_KNT_SHORT);
    Header head;
    if (!findHeader(book.rows, QList<QStringList>() << (full + shortA), head)) {
        error = QS("Не нашёл столбец «№ КНТ» в первых 30 строках. Заголовки первой строки: ") + firstRowText(book.rows);
        return false;
    }
    int cKnt = columnOf(head.norm, full);
    if (cKnt < 0) cKnt = columnOf(head.norm, shortA);

    out = ActFile();
    out.fileName = fileName;
    out.sheet = book.sheet;
    out.headerRow = head.index + 1;
    out.dupes = 0;
    QSet<QString> seen;
    for (int r = head.index + 1; r < book.rows.size(); ++r) {
        const QVariantList &row = book.rows.at(r);
        QString knt = digits(cKnt < row.size() ? cellText(row.at(cKnt)) : QString());
        if (knt.isEmpty()) continue;
        if (seen.contains(knt)) { ++out.dupes; continue; }
        seen.insert(knt);
        out.knt << knt;
    }
    if (out.knt.isEmpty()) { error = QS("В столбце «№ КНТ» акта нет ни одного номера"); return false; }
    return true;
}

MatchResult match(const CheckFile &check, const ActFile &act)
{
    QHash<QString, int> allowed, denied;
    for (int i = 0; i < check.allowed.size(); ++i) allowed.insert(check.allowed.at(i).knt, i);   // последний побеждает
    for (int i = 0; i < check.denied.size(); ++i)
        if (!denied.contains(check.denied.at(i).knt)) denied.insert(check.denied.at(i).knt, i);

    MatchResult res;
    for (int i = 0; i < act.knt.size(); ++i) {
        const QString &k = act.knt.at(i);
        if (allowed.contains(k)) {
            res.inWork << check.allowed.at(allowed.value(k));
        } else if (denied.contains(k)) {
            Extra e; e.knt = k; e.why = QS("НЕ ДОПУЩЕН"); e.name = check.denied.at(denied.value(k)).name; e.miss = false;
            res.extra << e;
        } else {
            Extra e; e.knt = k; e.why = QS("нет в списке проверки"); e.miss = true;
            res.extra << e;
        }
    }
    return res;
}

/* Первичный вес символа, близкий к порядку ICU: пробелы, знаки, цифры, латиница, кириллица. */
static uint weight(QChar c)
{
    QChar l = c.toLower();
    if (l.unicode() == 0x0451) l = QChar(0x0435);   // ё сравнивается как е
    ushort u = l.unicode();
    if (l.isSpace()) return 0x10000;
    if (u >= '0' && u <= '9') return 0x30000 + u;
    if (u < 0x80 && l.isLetter()) return 0x40000 + u;
    if (u >= 0x0400 && u <= 0x04FF) return 0x50000 + u;
    if (!l.isLetterOrNumber()) return 0x20000 + u;
    return 0x60000 + u;
}

int ruCompare(const QString &a, const QString &b)
{
    int n = qMin(a.size(), b.size());
    for (int i = 0; i < n; ++i) {
        uint wa = weight(a.at(i)), wb = weight(b.at(i));
        if (wa != wb) return wa < wb ? -1 : 1;
    }
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    // равны без учёта регистра и ё: е раньше ё, строчные раньше прописных
    for (int i = 0; i < n; ++i) {
        ushort ca = a.at(i).unicode(), cb = b.at(i).unicode();
        if (ca == cb) continue;
        bool ya = (ca == 0x0451 || ca == 0x0401), yb = (cb == 0x0451 || cb == 0x0401);
        if (ya != yb) return ya ? 1 : -1;
        bool la = a.at(i).isLower(), lb = b.at(i).isLower();
        if (la != lb) return la ? -1 : 1;
    }
    return 0;
}

bool itemLess(const Item &a, const Item &b)
{
    int c = ruCompare(a.name, b.name);
    if (c != 0) return c < 0;
    return ruCompare(a.knt, b.knt) < 0;
}

QList<int> candidates(const QList<Item> &items, const QString &query)
{
    QList<int> out;
    QString d = digits(query);
    if (d.isEmpty()) return out;
    for (int i = 0; i < items.size(); ++i) {
        const Item &it = items.at(i);
        if (d.size() >= it.knt.size()) { if (it.knt == d) out << i; }
        else if (it.knt.endsWith(d) || it.shortNo == d) out << i;
    }
    return out;
}

QString plural(int n, const char *one, const char *few, const char *many)
{
    int a = qAbs(n) % 100, b = a % 10;
    const char *w = many;
    if (a > 10 && a < 20) w = many;
    else if (b > 1 && b < 5) w = few;
    else if (b == 1) w = one;
    return QString::number(n) + QS(" ") + QString::fromUtf8(w);
}

static QString p2(int n) { return (n < 10 ? QS("0") : QString()) + QString::number(n); }

QString hhmmss(qint64 ms)
{
    qint64 s = ms < 0 ? 0 : ms / 1000;
    return p2((int)(s / 3600)) + QS(":") + p2((int)((s / 60) % 60)) + QS(":") + p2((int)(s % 60));
}

QString clockOf(qint64 ts)
{
    QTime t = QDateTime::fromMSecsSinceEpoch(ts).time();
    return p2(t.hour()) + QS(":") + p2(t.minute()) + QS(":") + p2(t.second());
}

QString dateOf(qint64 ts)
{
    QDate d = QDateTime::fromMSecsSinceEpoch(ts).date();
    return p2(d.day()) + QS(".") + p2(d.month()) + QS(".") + QString::number(d.year());
}

QString stampOf(qint64 ts)
{
    return dateOf(ts) + QS(" ") + clockOf(ts);
}

QString fileStamp(qint64 ts)
{
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts);
    return p2(dt.date().day()) + QS("_") + p2(dt.date().month()) + QS("_") + QString::number(dt.date().year()) +
           QS("_") + p2(dt.time().hour()) + p2(dt.time().minute());
}

}  // namespace Logic
