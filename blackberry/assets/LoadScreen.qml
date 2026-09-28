import bb.cascades 1.4

// Экран «Загрузка»: две большие плитки файлов с SD-карты, сверка, старт.
Container {
    id: load
    property variant m: app.matchInfo
    property variant notes: load.m.notes ? load.m.notes : []
    property variant resume: app.resumeInfo

    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    background: Color.create(T.bg)
    layout: StackLayout {
    }

    function noteTint(kind) {
        return kind == "ok" ? T.green : T.amber;
    }

    // ---------- верхняя полоса
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: ui.du(10)
        leftPadding: ui.du(3.5)
        rightPadding: ui.du(3.5)
        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }
        MonoB {
            verticalAlignment: VerticalAlignment.Center
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            text: "УТИЛИЗАЦИЯ КНТ"
            tint: T.amber
            size: 6.5
        }
        ImageView {
            verticalAlignment: VerticalAlignment.Center
            imageSource: "asset:///images/sdcard.png"
            filterColor: Color.create(app.sdReady ? T.green : T.red)
            preferredWidth: ui.du(3)
            preferredHeight: ui.du(3)
        }
        Mono {
            verticalAlignment: VerticalAlignment.Center
            leftMargin: ui.du(1)
            text: app.sdReady ? "SD-КАРТА" : "НЕТ SD-КАРТЫ"
            tint: app.sdReady ? T.green : T.red
            size: 5.5
        }
    }
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 2
        background: Color.create(T.line)
    }

    ScrollView {
        horizontalAlignment: HorizontalAlignment.Fill
        layoutProperties: StackLayoutProperties {
            spaceQuota: 1
        }
        scrollViewProperties.scrollMode: ScrollMode.Vertical
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            leftPadding: ui.du(3.5)
            rightPadding: ui.du(3.5)
            topPadding: ui.du(2.5)
            bottomPadding: ui.du(3)
            layout: StackLayout {
            }

            Sans {
                text: "Выберите два файла"
                tint: T.ink
                size: 12
            }
            Mono {
                text: "результат проверки и акт списания · с SD-карты"
                tint: T.ink2
                size: 5.5
            }

            // ---------- незавершённая сессия
            Container {
                visible: load.resume.has == true
                topMargin: ui.du(2.5)
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(T.amber)
                leftPadding: 3
                rightPadding: 3
                topPadding: 3
                bottomPadding: 3
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: Color.create(T.amberSel)
                    leftPadding: ui.du(2.2)
                    rightPadding: ui.du(2.2)
                    topPadding: ui.du(1.8)
                    bottomPadding: ui.du(1.8)
                    layout: StackLayout {
                    }
                    MonoB {
                        text: "НЕЗАВЕРШЁННАЯ СЕССИЯ"
                        tint: T.amber
                        size: 6
                    }
                    Sans {
                        horizontalAlignment: HorizontalAlignment.Fill
                        text: load.resume.has ? load.resume.text : ""
                        tint: T.ink
                        size: 7
                        multiline: true
                    }
                    Container {
                        topMargin: ui.du(1.5)
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: ui.du(13)
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }
                        Key {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            kind: "primary"
                            icon: "play"
                            label: "ПРОДОЛЖИТЬ"
                            labelSize: 7
                            onClicked: app.resume()
                        }
                        Key {
                            leftMargin: ui.du(1)
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            kind: "fn"
                            icon: "close"
                            label: "НАЧАТЬ ЗАНОВО"
                            onClicked: app.requestRestart()
                        }
                    }
                }
            }

            // ---------- две плитки файлов
            Container {
                topMargin: ui.du(2.5)
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: ui.du(36)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                FileTile {
                    verticalAlignment: VerticalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    kind: "check"
                }
                FileTile {
                    leftMargin: ui.du(2)
                    verticalAlignment: VerticalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    kind: "act"
                }
            }

            // ---------- ошибка чтения
            Container {
                visible: app.loadError != ""
                topMargin: ui.du(2)
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(T.red)
                leftPadding: 3
                rightPadding: 3
                topPadding: 3
                bottomPadding: 3
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: Color.create(T.panel)
                    leftPadding: ui.du(2)
                    rightPadding: ui.du(2)
                    topPadding: ui.du(1.2)
                    bottomPadding: ui.du(1.2)
                    layout: StackLayout {
                    }
                    MonoB {
                        text: "× ОШИБКА ЧТЕНИЯ"
                        tint: T.red
                        size: 6
                    }
                    Sans {
                        horizontalAlignment: HorizontalAlignment.Fill
                        text: app.loadError
                        tint: T.ink
                        size: 6.5
                        multiline: true
                    }
                }
            }

            // ---------- сверка
            Container {
                visible: load.m.ready == true
                topMargin: ui.du(2)
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(T.line)
                leftPadding: 2
                rightPadding: 2
                topPadding: 2
                bottomPadding: 2
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: Color.create(T.panel)
                    leftPadding: ui.du(2.2)
                    rightPadding: ui.du(2.2)
                    topPadding: ui.du(1.6)
                    bottomPadding: ui.du(1.6)
                    layout: StackLayout {
                    }
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }
                        Stat {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            value: load.m.ready ? load.m.inWork : ""
                            label: "К УТИЛИЗАЦИИ"
                            tint: T.amber
                        }
                        Stat {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            value: load.m.ready ? load.m.act : ""
                            label: "В АКТЕ"
                        }
                        Stat {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            value: load.m.ready ? load.m.denied : ""
                            label: "НЕ ДОПУЩЕНО"
                            tint: load.m.denied > 0 ? T.red : T.ink
                        }
                        Stat {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            value: load.m.ready ? load.m.empty : ""
                            label: "ПУСТОЕ РЕШЕНИЕ"
                            tint: load.m.empty > 0 ? T.red : T.ink
                        }
                    }
                    Container {
                        topMargin: ui.du(1.2)
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 2
                        background: Color.create(T.lineSoft)
                    }
                    Note {
                        visible: load.notes.length > 0
                        topMargin: ui.du(1)
                        title: load.notes.length > 0 ? load.notes[0].k : ""
                        body: load.notes.length > 0 ? load.notes[0].v : ""
                        tint: load.notes.length > 0 ? load.noteTint(load.notes[0].c) : T.ink2
                    }
                    Note {
                        visible: load.notes.length > 1
                        topMargin: ui.du(1)
                        title: load.notes.length > 1 ? load.notes[1].k : ""
                        body: load.notes.length > 1 ? load.notes[1].v : ""
                        tint: load.notes.length > 1 ? load.noteTint(load.notes[1].c) : T.ink2
                    }
                    Note {
                        visible: load.notes.length > 2
                        topMargin: ui.du(1)
                        title: load.notes.length > 2 ? load.notes[2].k : ""
                        body: load.notes.length > 2 ? load.notes[2].v : ""
                        tint: load.notes.length > 2 ? load.noteTint(load.notes[2].c) : T.ink2
                    }
                }
            }

            // ---------- старт
            Key {
                topMargin: ui.du(2.5)
                preferredHeight: ui.du(12)
                kind: "primary"
                icon: "arrow_right"
                label: load.m.ready ? "НАЧАТЬ УТИЛИЗАЦИЮ · " + load.m.inWork : "НАЧАТЬ УТИЛИЗАЦИЮ"
                labelSize: 7.5
                active: load.m.ready == true
                onClicked: app.start()
            }

            Container {
                topMargin: ui.du(1.8)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                ImageView {
                    verticalAlignment: VerticalAlignment.Center
                    imageSource: "asset:///images/folder.png"
                    filterColor: Color.create(T.ink2)
                    preferredWidth: ui.du(2.5)
                    preferredHeight: ui.du(2.5)
                }
                Mono {
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: ui.du(1)
                    text: "ОТЧЁТ: " + app.outDirText
                    tint: T.ink2
                    size: 5.2
                }
            }
        }
    }
}
