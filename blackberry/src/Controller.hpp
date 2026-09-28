/* ==========================================================================
   Контроллер приложения: состояние сессии, таймер, отметки, файлы на SD-карте.
   В QML доступен как «app». Экран интерфейса выбирается свойством screen:
   load → files → block → work ⇄ grid → done.
   ========================================================================== */
#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "Logic.hpp"

#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariant>

#ifdef Q_OS_BLACKBERRY
#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/DataModel>
#endif

class Controller : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString screen READ screen NOTIFY screenChanged)
    Q_PROPERTY(bool sdReady READ sdReady NOTIFY sdChanged)
    Q_PROPERTY(QString outDirText READ outDirText NOTIFY sdChanged)

    // загрузка
    Q_PROPERTY(QVariantMap checkInfo READ checkInfo NOTIFY loadChanged)
    Q_PROPERTY(QVariantMap actInfo READ actInfo NOTIFY loadChanged)
    Q_PROPERTY(QVariantMap matchInfo READ matchInfo NOTIFY loadChanged)
    Q_PROPERTY(QString loadError READ loadError NOTIFY loadChanged)
    Q_PROPERTY(QVariantMap resumeInfo READ resumeInfo NOTIFY resumeChanged)

    // выбор файла
    Q_PROPERTY(QString browseKind READ browseKind NOTIFY browseChanged)
    Q_PROPERTY(QString browsePath READ browsePath NOTIFY browseChanged)
    Q_PROPERTY(QString browseNote READ browseNote NOTIFY browseChanged)
    Q_PROPERTY(QVariantList browseItems READ browseItems NOTIFY browseChanged)
    Q_PROPERTY(bool browseCanUp READ browseCanUp NOTIFY browseChanged)

    // блокировка
    Q_PROPERTY(QVariantList extraList READ extraList NOTIFY blockChanged)
    Q_PROPERTY(QString extraNote READ extraNote NOTIFY blockChanged)

    // утилизация
    Q_PROPERTY(QString timerState READ timerState NOTIFY timerChanged)
    Q_PROPERTY(QString timerText READ timerText NOTIFY timerChanged)
    Q_PROPERTY(QString timerLine READ timerLine NOTIFY timerChanged)
    Q_PROPERTY(int total READ total NOTIFY progressChanged)
    Q_PROPERTY(int doneCount READ doneCount NOTIFY progressChanged)
    Q_PROPERTY(QString typed READ typed NOTIFY workChanged)
    Q_PROPERTY(QVariantMap card READ card NOTIFY workChanged)
    Q_PROPERTY(QVariantMap status READ status NOTIFY workChanged)
    Q_PROPERTY(QVariantList picks READ picks NOTIFY workChanged)
    Q_PROPERTY(bool detailsOpen READ detailsOpen NOTIFY workChanged)
    Q_PROPERTY(QVariantList details READ details NOTIFY workChanged)

    // сетка
    Q_PROPERTY(bool gridShowDone READ gridShowDone NOTIFY gridChanged)
    Q_PROPERTY(bool sheetOpen READ sheetOpen NOTIFY gridChanged)
    Q_PROPERTY(int gridCount READ gridCount NOTIFY gridChanged)
#ifdef Q_OS_BLACKBERRY
    Q_PROPERTY(bb::cascades::DataModel *gridModel READ gridModel CONSTANT)
#endif

    // итог
    Q_PROPERTY(QVariantList receipt READ receipt NOTIFY summaryChanged)
    Q_PROPERTY(QString exportNote READ exportNote NOTIFY summaryChanged)

public:
    explicit Controller(QObject *parent = 0);

    QString screen() const { return m_screen; }
    bool sdReady() const;
    QString outDirText() const;

    QVariantMap checkInfo() const;
    QVariantMap actInfo() const;
    QVariantMap matchInfo() const;
    QString loadError() const { return m_loadError; }
    QVariantMap resumeInfo() const;

    QString browseKind() const { return m_browseKind; }
    QString browsePath() const;
    QString browseNote() const { return m_browseNote; }
    QVariantList browseItems() const { return m_browseItems; }
    bool browseCanUp() const;

    QVariantList extraList() const;
    QString extraNote() const;

    QString timerState() const { return m_timer.state; }
    QString timerText() const;
    QString timerLine() const;
    int total() const { return m_items.size(); }
    int doneCount() const { return m_marks.size(); }
    QString typed() const { return m_typed; }
    QVariantMap card() const;
    QVariantMap status() const;
    QVariantList picks() const;
    bool detailsOpen() const { return m_detailsOpen; }
    QVariantList details() const;

    bool gridShowDone() const { return m_gridShowDone; }
    bool sheetOpen() const { return m_sheetOpen; }
    int gridCount() const { return m_gridRows.size(); }
#ifdef Q_OS_BLACKBERRY
    bb::cascades::DataModel *gridModel() const { return m_gridModel; }
#endif

    QVariantList receipt() const;
    QString exportNote() const { return m_exportNote; }

    /* для тестов на хосте */
    Q_INVOKABLE QVariantList gridSnapshot() const;
    Q_INVOKABLE void loadFile(const QString &kind, const QString &path);

public slots:
    // загрузка и файлы
    void refreshSd();
    void browse(const QString &kind);
    void browseOpen(int index);
    void browseUp();
    void browseCancel();
    void start();
    void resume();

    // блокировка
    void copyExtra();
    void blockBack();

    // утилизация
    void key(const QString &digit);
    void backspace();
    void clearInput();
    void find();
    void keyPress(int code, const QString &text);
    void pick(int itemIndex);
    void timerToggle();
    void mark();
    void primary();          // большая клавиша: отметить или снять отметку
    void nextTodo();
    void prev();
    void next();
    void toggleDetails();

    // сетка
    void openGrid();
    void closeGrid();
    void gridToggleDone();
    void gridOpen(int itemIndex);
    void sheetClose();

    // итог
    void exportXlsx();
    void exportText();

    // подтверждения
    void requestUnmark();
    void requestFinish();
    void requestRestart();
    void confirmed(const QString &action);

signals:
    void screenChanged();
    void sdChanged();
    void loadChanged();
    void resumeChanged();
    void browseChanged();
    void blockChanged();
    void timerChanged();
    void progressChanged();
    void workChanged();
    void gridChanged();
    void summaryChanged();

    void toast(const QString &text);
    void confirm(const QString &action, const QString &title, const QString &body, const QString &yes);

private slots:
    void tick();
    void parsePending();
    void autoCloseSheet();

private:
    struct LogEntry { qint64 ts; QString knt, name, type; };
    struct Timer {
        QString state;      // idle, run, pause, done
        qint64 startedAt, anchor, accMs, pausedMs, pauseAnchor, finishedAt;
        int pauses;
    };

    void setScreen(const QString &s);
    void runMatch();
    void listDir();
    QString rootForBrowse() const;

    qint64 now() const;
    qint64 elapsedMs() const;
    qint64 pausedTotal() const;
    void timerChangedInternal();

    void openItem(int idx, bool fromGrid = false);
    void doFind(bool autoMode);
    void unmark();
    void finish();
    void restart();

    void resetSession();
    void computeColumns();
    QString cellOf(const Item &it, int col) const;

    QString statePath() const;
    QString itemsPath() const;
    void saveState();
    void saveItems();
    bool loadSession();
    void dropSession();

    void rebuildGrid();
    void refreshGridItem(int itemIdx);
    QVariantMap gridEntry(int itemIdx) const;

    QString doneAsText() const;
    QString restText() const;
    QString extraAsText() const;

    QString m_screen, m_prevScreen;

    // загрузка
    bool m_hasCheck, m_hasAct;
    CheckFile m_check;
    ActFile m_act;
    QString m_checkState, m_actState, m_checkLine, m_actLine, m_checkName, m_actName;
    QString m_loadError;
    MatchResult m_match;
    bool m_matchReady;
    QString m_pendingKind, m_pendingPath;

    // выбор файла
    QString m_browseKind, m_browseDir, m_browseNote;
    QHash<QString, QString> m_lastDir;
    QVariantList m_browseItems;

    // сессия
    bool m_active;
    QString m_sessionId, m_metaCheck, m_metaAct;
    qint64 m_createdAt;
    QStringList m_headers;
    QList<Item> m_items;
    QHash<QString, qint64> m_marks;
    QList<LogEntry> m_log;
    Timer m_timer;
    int m_cursor;
    int m_colCode, m_colBrand, m_colPrice;
    bool m_resumeAvailable;

    // ввод
    QString m_typed, m_find;
    bool m_fresh;
    QList<int> m_picks;
    bool m_detailsOpen;

    // сетка
    bool m_gridShowDone, m_sheetOpen;
    int m_sheetSeq, m_closeSeq;
    QList<int> m_gridRows;
#ifdef Q_OS_BLACKBERRY
    bb::cascades::ArrayDataModel *m_gridModel;
#endif

    QString m_exportNote;
    QTimer m_tick;
};

#endif
