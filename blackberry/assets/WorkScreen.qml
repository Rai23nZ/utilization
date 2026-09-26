import bb.cascades 1.4

// Экран «Утилизация»: полоса таймера, табло с карточкой, цифровой пульт.
Container {
    id: work
    property variant c: app.card
    property variant s: app.status
    property variant tags: work.c.tags ? work.c.tags : []
    property string st: app.timerState

    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    layout: DockLayout {
    }
    background: Color.create(T.bg)

    function statusTint(kind) {
        if (kind == "ok" || kind == "done") return T.green;
        if (kind == "warn") return T.amber;
        if (kind == "err") return T.red;
        return T.ink2;
    }
    function statusMark(kind) {
        if (kind == "ok") return "→ ";
        if (kind == "done") return "✓ ";
        if (kind == "err") return "× ";
        if (kind == "warn") return "! ";
        return "";
    }
    function refreshLists() {
        picksModel.clear();
        var p = app.picks;
        for (var i = 0; i < p.length; i++) picksModel.append(p[i]);
        detailsModel.clear();
        if (app.detailsOpen) {
            var d = app.details;
            for (var j = 0; j < d.length; j++) detailsModel.append(d[j]);
        }
    }
    function paintSegments() {
        var n = app.total > 0 ? Math.round(app.doneCount * 30 / app.total) : 0;
        for (var i = 0; i < segs.count(); i++) {
            segs.at(i).background = Color.create(i < n ? T.amber : T.segOff);
        }
    }
    onCreationCompleted: {
        for (var i = 0; i < 30; i++) segs.add(segDef.createObject());
        work.paintSegments();
        app.progressChanged.connect(work.paintSegments);
        app.workChanged.connect(work.refreshLists);
    }
    attachedObjects: [
        ComponentDefinition {
            id: segDef
            Container {
                preferredHeight: ui.du(1)
                leftMargin: 5
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1
                }
            }
        },
        ArrayDataModel {
            id: picksModel
        },
        ArrayDataModel {
            id: detailsModel
        }
    ]

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layout: StackLayout {
        }

        // ---------- верхняя полоса: состояние, время, счётчик
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: ui.du(10)
            leftPadding: ui.du(3.5)
            rightPadding: ui.du(3.5)
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            ImageView {
                verticalAlignment: VerticalAlignment.Center
                imageSource: "asset:///images/dot.png"
                filterColor: Color.create(work.st == "run" ? T.green : (work.st == "pause" ? T.amber : T.ink2))
                preferredWidth: ui.du(2.2)
                preferredHeight: ui.du(2.2)
            }
            MonoB {
                verticalAlignment: VerticalAlignment.Center
                leftMargin: ui.du(1)
                text: work.st == "run" ? "ИДЁТ" : (work.st == "pause" ? "ПАУЗА" : (work.st == "done" ? "ЗАВЕРШЕНА" : "НЕ НАЧАТА"))
                tint: work.st == "run" ? T.green : (work.st == "pause" ? T.amber : T.ink2)
                size: 6
            }
            MonoB {
                verticalAlignment: VerticalAlignment.Center
                leftMargin: ui.du(2)
                text: app.timerText
                tint: T.amber
                size: 12
            }
            Container {
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1
                }
            }
            MonoB {
                verticalAlignment: VerticalAlignment.Center
                text: app.doneCount
                tint: T.ink
                size: 12
            }
            MonoB {
                verticalAlignment: VerticalAlignment.Center
                text: "/" + app.total
                tint: T.ink2
                size: 12
            }
            Mono {
                verticalAlignment: VerticalAlignment.Center
                leftMargin: ui.du(1.5)
                text: "ОСТ. " + (app.total - app.doneCount)
                tint: T.ink2
                size: 5.5
            }
        }
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 2
            background: Color.create(T.line)
        }

        // ---------- сегменты прогресса
        Container {
            id: segs
            horizontalAlignment: HorizontalAlignment.Fill
            leftPadding: ui.du(3.5)
            rightPadding: ui.du(3.5)
            topPadding: ui.du(1.2)
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
        }

        // ---------- табло
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            leftPadding: ui.du(3.5)
            rightPadding: ui.du(3.5)
            topPadding: ui.du(1.2)
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(T.line)
                leftPadding: 2
                rightPadding: 2
                topPadding: 2
                bottomPadding: 2
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    preferredHeight: ui.du(47)
                    background: Color.create(T.panel)
                    leftPadding: ui.du(2.2)
                    rightPadding: ui.du(2.2)
                    topPadding: ui.du(1.5)
                    bottomPadding: ui.du(1.5)
                    clipContentToBounds: true
                    layout: StackLayout {
                    }

                    // строка заголовка табло
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }
                        Mono {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            text: work.c.has ? "№ КНТ" : (app.picks.length > 0 ? "СОВПАДЕНИЯ" : "НАБОР НОМЕРА")
                            tint: T.ink2
                            size: 5.5
                        }
                        Mono {
                            text: work.c.has ? "ПОЗ. " + work.c.pos : ""
                            tint: T.ink2
                            size: 5.5
                        }
                    }

                    // основное поле: карточка, набранные цифры или список совпадений
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        layout: DockLayout {
                        }

                        // карточка: номер прижат к верху, наименование и метки — к низу.
                        // Длинное наименование уходит на вторую строку вверх, в пустое поле
                        // под крупными цифрами, и не сдвигает остальные элементы табло.
                        Container {
                            visible: work.c.has == true
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Fill
                            layout: DockLayout {
                            }
                            Container {
                                verticalAlignment: VerticalAlignment.Top
                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }
                                MonoB {
                                    text: work.c.has ? work.c.head : ""
                                    tint: T.dim
                                    size: 26
                                }
                                MonoB {
                                    text: work.c.has ? work.c.tail : ""
                                    tint: work.c.marked ? T.green : T.amber
                                    size: 26
                                }
                            }
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Bottom
                                layout: StackLayout {
                                }
                                Sans {
                                    preferredWidth: ui.du(108.3)
                                    maxWidth: ui.du(108.3)
                                    text: work.c.has ? work.c.name2 : ""
                                    tint: T.ink
                                    size: 8.5
                                    multiline: true
                                    autoSize.maxLineCount: 2
                                }
                                Container {
                                    topMargin: ui.du(1)
                                    layout: StackLayout {
                                        orientation: LayoutOrientation.LeftToRight
                                    }
                                    Tag {
                                        visible: work.tags.length > 0
                                        label: work.tags.length > 0 ? work.tags[0].k + " " + work.tags[0].v : ""
                                        hi: work.tags.length > 0 && work.tags[0].c == "hi"
                                    }
                                    Tag {
                                        visible: work.tags.length > 1
                                        leftMargin: ui.du(1.2)
                                        label: work.tags.length > 1 ? work.tags[1].k + " " + work.tags[1].v : ""
                                        hi: work.tags.length > 1 && work.tags[1].c == "hi"
                                    }
                                    Tag {
                                        visible: work.tags.length > 2
                                        leftMargin: ui.du(1.2)
                                        label: work.tags.length > 2 ? work.tags[2].k + " " + work.tags[2].v : ""
                                        hi: work.tags.length > 2 && work.tags[2].c == "hi"
                                    }
                                }
                            }
                        }

                        // штамп «утилизирован»
                        Container {
                            visible: work.c.has == true && work.c.marked == true
                            horizontalAlignment: HorizontalAlignment.Right
                            verticalAlignment: VerticalAlignment.Top
                            topMargin: ui.du(3)
                            rotationZ: -9
                            background: Color.create(T.green)
                            leftPadding: 4
                            rightPadding: 4
                            topPadding: 4
                            bottomPadding: 4
                            Container {
                                background: Color.create(T.panel)
                                leftPadding: ui.du(1.5)
                                rightPadding: ui.du(1.5)
                                topPadding: ui.du(0.6)
                                bottomPadding: ui.du(0.6)
                                layout: StackLayout {
                                }
                                MonoB {
                                    horizontalAlignment: HorizontalAlignment.Center
                                    text: "УТИЛИЗИРОВАН"
                                    tint: T.green
                                    size: 8
                                }
                                Mono {
                                    horizontalAlignment: HorizontalAlignment.Center
                                    text: work.c.has ? work.c.markedAt : ""
                                    tint: T.green
                                    size: 5.5
                                }
                            }
                        }

                        // набираемый номер
                        Container {
                            visible: work.c.has != true && app.picks.length == 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Center
                            layout: StackLayout {
                            }
                            MonoB {
                                horizontalAlignment: HorizontalAlignment.Center
                                text: app.typed.length >= 4 ? app.typed : app.typed + "____".substring(app.typed.length)
                                tint: work.s.kind == "err" ? T.red : T.amber
                                size: 30
                            }
                            Mono {
                                horizontalAlignment: HorizontalAlignment.Center
                                text: "ЦИФРЫ — С ПУЛЬТА ИЛИ КЛАВИАТУРЫ · ENTER — ИСКАТЬ"
                                tint: T.dim
                                size: 5
                            }
                        }

                        // совпадения: выбрать нужную позицию
                        ListView {
                            id: picksList
                            visible: app.picks.length > 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Fill
                            dataModel: picksModel
                            onTriggered: {
                                var d = picksModel.data(indexPath);
                                app.pick(d.idx);
                            }
                            listItemComponents: [
                                ListItemComponent {
                                    type: ""
                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        topPadding: 14
                                        bottomPadding: 14
                                        layout: StackLayout {
                                            orientation: LayoutOrientation.LeftToRight
                                        }
                                        MonoB {
                                            verticalAlignment: VerticalAlignment.Center
                                            text: ListItemData.knt
                                            tint: ListItemData.done ? "#5bd68a" : "#ffb000"
                                            size: 8
                                        }
                                        Sans {
                                            verticalAlignment: VerticalAlignment.Center
                                            leftMargin: 30
                                            layoutProperties: StackLayoutProperties {
                                                spaceQuota: 1
                                            }
                                            text: ListItemData.name
                                            tint: "#e8e4d8"
                                            size: 6.5
                                        }
                                        Mono {
                                            verticalAlignment: VerticalAlignment.Center
                                            text: ListItemData.done ? "✓ УТИЛ." : ""
                                            tint: "#5bd68a"
                                            size: 5.5
                                        }
                                    }
                                }
                            ]
                        }
                    }

                    // строка состояния
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        topMargin: ui.du(0.8)
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }
                        MonoB {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            text: work.statusMark(work.s.kind) + work.s.text
                            tint: work.statusTint(work.s.kind)
                            size: 5.5
                        }
                        Mono {
                            text: work.s.hint
                            tint: T.ink2
                            size: 5
                        }
                    }
                }
            }
        }

        // ---------- пульт
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            leftPadding: ui.du(3.5)
            rightPadding: ui.du(3.5)
            topPadding: ui.du(1.5)
            bottomPadding: ui.du(3)
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            // цифры
            Container {
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 3
                }
                verticalAlignment: VerticalAlignment.Fill
                layout: StackLayout {
                }
                KeyRow {
                    a: "1"
                    b: "2"
                    c: "3"
                }
                KeyRow {
                    topMargin: ui.du(1)
                    a: "4"
                    b: "5"
                    c: "6"
                }
                KeyRow {
                    topMargin: ui.du(1)
                    a: "7"
                    b: "8"
                    c: "9"
                }
                Container {
                    topMargin: ui.du(1)
                    horizontalAlignment: HorizontalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Key {
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        kind: "fn"
                        icon: "backspace"
                        sub: "ДОЛГО — СБРОС"
                        onClicked: app.backspace()
                        onLongClicked: app.clearInput()
                    }
                    Key {
                        leftMargin: ui.du(1)
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        label: "0"
                        onClicked: app.key("0")
                    }
                    Key {
                        leftMargin: ui.du(1)
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        kind: "fn"
                        icon: "arrow_right"
                        label: "К НЕОТМ."
                        active: app.total > 0
                        onClicked: app.nextTodo()
                    }
                }
            }

            // функции и главная клавиша
            Container {
                leftMargin: ui.du(1)
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 2
                }
                verticalAlignment: VerticalAlignment.Fill
                layout: StackLayout {
                }
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Key {
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        kind: work.st == "idle" ? "primary" : "fn"
                        icon: work.st == "run" ? "pause" : "play"
                        label: work.st == "run" ? "ПАУЗА" : (work.st == "pause" ? "ПРОДОЛЖИТЬ" : "СТАРТ")
                        labelSize: work.st == "idle" ? 7 : 6
                        active: work.st != "done"
                        onClicked: app.timerToggle()
                    }
                    Key {
                        leftMargin: ui.du(1)
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        kind: "fn"
                        icon: "grid"
                        label: "СЕТКА"
                        onClicked: app.openGrid()
                    }
                }
                Container {
                    topMargin: ui.du(1)
                    horizontalAlignment: HorizontalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Key {
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        kind: "fn"
                        icon: "info"
                        label: "ПОДРОБНО"
                        active: work.c.has == true
                        onClicked: app.toggleDetails()
                    }
                    Key {
                        leftMargin: ui.du(1)
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        kind: "fn"
                        icon: "stop"
                        label: "ЗАВЕРШИТЬ"
                        active: work.st == "run" || work.st == "pause"
                        onClicked: app.requestFinish()
                    }
                }
                Key {
                    topMargin: ui.du(1)
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 2.08
                    }
                    kind: work.c.action == "unmark" ? "danger" : "primary"
                    icon: work.c.action == "unmark" ? "close" : "check"
                    label: work.c.action == "unmark" ? "СНЯТЬ ОТМЕТКУ" : "УТИЛИЗИРОВАН"
                    labelSize: 8
                    sub: work.c.action == "mark" ? "ИЛИ ПРОБЕЛ" : ""
                    active: work.c.has == true && work.c.action != "off"
                    onClicked: app.primary()
                }
            }
        }
    }

    // ---------- «подробно»: все столбцы строки
    Container {
        visible: app.detailsOpen
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layout: DockLayout {
        }
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create("#000000")
            opacity: 0.65
            gestureHandlers: [
                TapHandler {
                    onTapped: app.toggleDetails()
                }
            ]
        }
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Bottom
            preferredHeight: ui.du(86)
            background: Color.create(T.amber)
            topPadding: 3
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(T.panel)
                leftPadding: ui.du(3.5)
                rightPadding: ui.du(3.5)
                topPadding: ui.du(2)
                bottomPadding: ui.du(3)
                layout: StackLayout {
                }
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Container {
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        layout: StackLayout {
                        }
                        Mono {
                            text: "ПОДРОБНО О ТОВАРЕ"
                            tint: T.ink2
                            size: 5.5
                        }
                        MonoB {
                            text: work.c.has ? work.c.knt : ""
                            tint: T.amber
                            size: 11
                        }
                    }
                    Key {
                        preferredWidth: ui.du(22)
                        preferredHeight: ui.du(9)
                        kind: "fn"
                        icon: "close"
                        label: "ЗАКРЫТЬ"
                        onClicked: app.toggleDetails()
                    }
                }
                Container {
                    topMargin: ui.du(1.5)
                    horizontalAlignment: HorizontalAlignment.Fill
                    preferredHeight: 2
                    background: Color.create(T.line)
                }
                ListView {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    dataModel: detailsModel
                    listItemComponents: [
                        ListItemComponent {
                            type: ""
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topPadding: 12
                                bottomPadding: 12
                                layout: StackLayout {
                                }
                                Mono {
                                    text: ListItemData.k
                                    tint: "#9a978c"
                                    size: 5
                                }
                                Sans {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    text: ListItemData.v
                                    tint: "#e8e4d8"
                                    size: 7
                                    multiline: true
                                }
                            }
                        }
                    ]
                }
            }
        }
    }
}
