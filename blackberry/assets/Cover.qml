import bb.cascades 1.4

// Active Frame: свёрнутое приложение показывает таймер и прогресс.
Container {
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    background: Color.create(T.bg)
    leftPadding: 24
    rightPadding: 24
    topPadding: 24
    bottomPadding: 24
    layout: StackLayout {
    }
    MonoB {
        text: "УТИЛИЗАЦИЯ"
        tint: T.amber
        size: 6.5
    }
    MonoB {
        topMargin: 20
        text: app.timerText
        tint: T.ink
        size: 11
    }
    Mono {
        text: app.timerState == "run" ? "ИДЁТ" : (app.timerState == "pause" ? "ПАУЗА" : (app.timerState == "done" ? "ЗАВЕРШЕНА" : "НЕ НАЧАТА"))
        tint: app.timerState == "run" ? T.green : T.ink2
        size: 6
    }
    MonoB {
        topMargin: 20
        text: app.total > 0 ? app.doneCount + " / " + app.total : "—"
        tint: T.amber
        size: 11
    }
    Mono {
        text: app.total > 0 ? "ОСТАЛОСЬ " + (app.total - app.doneCount) : "ФАЙЛЫ НЕ ВЫБРАНЫ"
        tint: T.ink2
        size: 6
    }
}
