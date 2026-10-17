import bb.cascades 1.4

// Экран «Сетка»: вся выборка плитками по 4 последним цифрам, в порядке наименований.
// Утилизированные залиты синим и по умолчанию скрыты. Касание плитки открывает шторку
// с карточкой: здесь можно и отметить утилизацию, и снять отметку. Если у позиции есть
// однородные (тот же артикул в начале наименования), шторка выше и показывает их список.
Container {
    id: grid
    property variant c: app.card
    property variant s: app.status
    property variant g: app.group
    property variant tags: grid.c.tags ? grid.c.tags : []
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
    function refreshGroup() {
        groupModel.clear();
        var rows = app.group.rows;
        if (!rows) return;
        for (var i = 0; i < rows.length; i++) groupModel.append(rows[i]);
    }
    onCreationCompleted: {
        grid.refreshGroup();
        app.workChanged.connect(grid.refreshGroup);
    }
    attachedObjects: [
        ArrayDataModel {
            id: groupModel
        }
    ]

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layout: StackLayout {
        }

        // ---------- верхняя полоса
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: ui.du(12)
            leftPadding: ui.du(3.5)
            rightPadding: ui.du(3.5)
            topPadding: ui.du(1.5)
            bottomPadding: ui.du(1.5)
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            Key {
                preferredWidth: ui.du(19)
                kind: "fn"
                icon: "arrow_left"
                label: "НАЗАД"
                onClicked: app.closeGrid()
            }
            Container {
                leftMargin: ui.du(2.5)
                verticalAlignment: VerticalAlignment.Center
                layout: StackLayout {
                }
                Mono {
                    text: "СЕТКА"
                    tint: T.amber
                    size: 5.5
                }
                Container {
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    MonoB {
                        text: app.doneCount
                        tint: T.ink
                        size: 10
                    }
                    MonoB {
                        text: "/" + app.total
                        tint: T.ink2
                        size: 10
                    }
                }
            }
            Container {
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1
                }
            }
            // таймер: касание — старт/пауза/продолжить
            Container {
                verticalAlignment: VerticalAlignment.Fill
                leftPadding: ui.du(1.5)
                rightPadding: ui.du(1.5)
                background: Color.create(grid.st == "idle" ? T.amber : T.keyFn)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                ImageView {
                    verticalAlignment: VerticalAlignment.Center
                    imageSource: grid.st == "run" ? "asset:///images/pause.png" : "asset:///images/play.png"
                    filterColor: Color.create(grid.st == "idle" ? T.bg : (grid.st == "run" ? T.green : T.amber))
                    preferredWidth: ui.du(3)
                    preferredHeight: ui.du(3)
                }
                MonoB {
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: ui.du(1)
                    text: grid.st == "idle" ? "СТАРТ" : app.timerText
                    tint: grid.st == "idle" ? T.bg : T.amber
                    size: 8
                }
                gestureHandlers: [
                    TapHandler {
                        onTapped: app.timerToggle()
                    }
                ]
            }
            Key {
                leftMargin: ui.du(1.5)
                preferredWidth: ui.du(27)
                kind: "fn"
                icon: app.gridShowDone ? "eye_off" : "eye"
                label: app.gridShowDone ? "СКРЫТЬ УТИЛ." : "ПОКАЗАТЬ УТИЛ."
                labelSize: 5.5
                onClicked: app.gridToggleDone()
            }
        }
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 2
            background: Color.create(T.line)
        }

        // ---------- легенда
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            leftPadding: ui.du(3.5)
            rightPadding: ui.du(3.5)
            topPadding: ui.du(1.2)
            bottomPadding: ui.du(1.2)
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            Swatch {
                fill: T.key
                edge: T.line
                label: "ОСТАЛОСЬ " + (app.total - app.doneCount)
            }
            Swatch {
                leftMargin: ui.du(2.5)
                fill: T.blue
                edge: T.blue
                label: "УТИЛИЗИРОВАНО " + app.doneCount
            }
            Swatch {
                leftMargin: ui.du(2.5)
                fill: T.key
                edge: T.amber
                label: "ОТКРЫТА"
            }
            Swatch {
                leftMargin: ui.du(2.5)
                fill: T.key
                edge: T.red
                label: "СНЯТА ОТМЕТКА"
            }
        }

        // ---------- плитки
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            leftPadding: ui.du(3.5)
            rightPadding: ui.du(3.5)
            layout: DockLayout {
            }
            ListView {
                id: tiles
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                dataModel: app.gridModel
                layout: GridListLayout {
                    columnCount: 5
                    cellAspectRatio: 1.35
                    horizontalCellSpacing: ui.du(1)
                    verticalCellSpacing: ui.du(1)
                }
                onTriggered: {
                    var d = dataModel.data(indexPath);
                    app.gridOpen(d.idx);
                }
                listItemComponents: [
                    ListItemComponent {
                        type: ""
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Fill
                            layout: DockLayout {
                            }
                            background: Color.create(ListItemData.cur ? "#ffb000" : (ListItemData.st == "undo" ? "#ff6b5e" : (ListItemData.st == "done" ? "#2a62d4" : "#2a2d29")))
                            leftPadding: ListItemData.cur ? 6 : 2
                            rightPadding: ListItemData.cur ? 6 : 2
                            topPadding: ListItemData.cur ? 6 : 2
                            bottomPadding: ListItemData.cur ? 6 : 2
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                background: Color.create(ListItemData.st == "done" ? "#2a62d4" : "#161816")
                                leftPadding: 14
                                rightPadding: 10
                                topPadding: 10
                                bottomPadding: 8
                                layout: StackLayout {
                                }
                                MonoB {
                                    text: ListItemData.sn
                                    tint: ListItemData.st == "done" ? "#ffffff" : "#ffb000"
                                    size: 10.5
                                }
                                Sans {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    text: ListItemData.name
                                    tint: ListItemData.st == "done" ? "#dbe5ff" : "#9a978c"
                                    size: 5
                                }
                            }
                        }
                    }
                ]
            }
            // пусто: всё утилизировано и скрыто
            Container {
                visible: app.gridCount == 0
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                layout: StackLayout {
                }
                MonoB {
                    horizontalAlignment: HorizontalAlignment.Center
                    text: app.total > 0 ? "✓ ВСЁ УТИЛИЗИРОВАНО" : "ВЫБОРКА ПУСТА"
                    tint: T.green
                    size: 10
                }
                Mono {
                    horizontalAlignment: HorizontalAlignment.Center
                    text: "«ПОКАЗАТЬ УТИЛ.» — ЧТОБЫ СНЯТЬ ОТМЕТКУ"
                    tint: T.ink2
                    size: 5.5
                }
            }
        }
    }

    // ---------- затемнение под шторкой
    Container {
        visible: app.sheetOpen
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create("#000000")
        opacity: 0.6
        gestureHandlers: [
            TapHandler {
                onTapped: app.sheetClose()
            }
        ]
    }

    // ---------- шторка с карточкой
    Container {
        id: sheet
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Bottom
        preferredHeight: grid.g.has ? ui.du(100) : ui.du(60)
        translationY: app.sheetOpen ? 0 : (grid.g.has ? ui.du(104) : ui.du(64))
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
                Mono {
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    text: grid.c.has ? "№ КНТ · ПОЗ. " + grid.c.pos : "№ КНТ"
                    tint: T.ink2
                    size: 5.5
                }
                Key {
                    preferredWidth: ui.du(20)
                    preferredHeight: ui.du(8)
                    kind: "fn"
                    label: "× ЗАКРЫТЬ"
                    labelSize: 5.5
                    onClicked: app.sheetClose()
                }
            }
            Container {
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                MonoB {
                    text: grid.c.has ? grid.c.head : ""
                    tint: T.dim
                    size: 22
                }
                MonoB {
                    text: grid.c.has ? grid.c.tail : ""
                    tint: grid.c.marked ? T.green : T.amber
                    size: 22
                }
            }
            Sans {
                horizontalAlignment: HorizontalAlignment.Fill
                text: grid.c.has ? grid.c.name : ""
                tint: T.ink
                size: 8
            }
            Container {
                topMargin: ui.du(1)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                Tag {
                    visible: grid.tags.length > 0
                    label: grid.tags.length > 0 ? grid.tags[0].k + " " + grid.tags[0].v : ""
                    hi: grid.tags.length > 0 && grid.tags[0].c == "hi"
                }
                Tag {
                    visible: grid.tags.length > 1
                    leftMargin: ui.du(1.2)
                    label: grid.tags.length > 1 ? grid.tags[1].k + " " + grid.tags[1].v : ""
                    hi: grid.tags.length > 1 && grid.tags[1].c == "hi"
                }
                Tag {
                    visible: grid.tags.length > 2
                    leftMargin: ui.du(1.2)
                    label: grid.tags.length > 2 ? grid.tags[2].k + " " + grid.tags[2].v : ""
                    hi: grid.tags.length > 2 && grid.tags[2].c == "hi"
                }
            }
            MonoB {
                topMargin: ui.du(1)
                text: grid.s.text
                tint: grid.statusTint(grid.s.kind)
                size: 5.5
            }
            // однородный товар: галочки у неотмеченных соседей; если текущая уже
            // отмечена — неотмеченные соседи открывают свою карточку
            Container {
                visible: grid.g.has == true
                topMargin: ui.du(1.5)
                horizontalAlignment: HorizontalAlignment.Fill
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1
                }
                layout: StackLayout {
                }
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    preferredHeight: 2
                    background: Color.create(T.line)
                }
                Container {
                    topMargin: ui.du(1)
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Mono {
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                        text: grid.g.has ? "ОДНОРОДНЫЙ ТОВАР · " + grid.g.sku + " · ЕЩЁ " + grid.g.count : ""
                        tint: T.amber
                        size: 5.5
                    }
                    Key {
                        visible: grid.g.canPick == true
                        preferredWidth: ui.du(26)
                        preferredHeight: ui.du(8)
                        kind: "fn"
                        label: grid.g.allOn ? "СНЯТЬ ВЫБОР" : "ВЫБРАТЬ ВСЕ"
                        labelSize: 5.5
                        onClicked: app.groupAll()
                    }
                }
                ListView {
                    topMargin: ui.du(0.5)
                    horizontalAlignment: HorizontalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    dataModel: groupModel
                    onTriggered: {
                        var d = groupModel.data(indexPath);
                        app.groupTap(d.idx);
                    }
                    listItemComponents: [
                        ListItemComponent {
                            type: ""
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                background: Color.create(ListItemData.on ? "#231d0c" : "#111311")
                                leftPadding: 10
                                rightPadding: 10
                                topPadding: 12
                                bottomPadding: 12
                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }
                                ImageView {
                                    verticalAlignment: VerticalAlignment.Center
                                    imageSource: ListItemData.mode == "open" ? "asset:///images/arrow_right.png" : (ListItemData.mode == "done" || ListItemData.on ? "asset:///images/check.png" : "asset:///images/frame.png")
                                    filterColor: Color.create(ListItemData.mode == "done" ? "#5bd68a" : (ListItemData.mode == "open" || ListItemData.on ? "#ffb000" : "#9a978c"))
                                    preferredWidth: 44
                                    preferredHeight: 44
                                }
                                MonoB {
                                    verticalAlignment: VerticalAlignment.Center
                                    leftMargin: 24
                                    text: ListItemData.knt
                                    tint: ListItemData.mode == "done" ? "#5bd68a" : (ListItemData.on ? "#ffb000" : "#e8e4d8")
                                    size: 6.5
                                }
                                Sans {
                                    verticalAlignment: VerticalAlignment.Center
                                    leftMargin: 24
                                    layoutProperties: StackLayoutProperties {
                                        spaceQuota: 1
                                    }
                                    text: ListItemData.name
                                    tint: ListItemData.mode == "done" ? "#6b675d" : "#9a978c"
                                    size: 6
                                }
                                Mono {
                                    verticalAlignment: VerticalAlignment.Center
                                    leftMargin: 16
                                    text: ListItemData.mode == "done" ? "✓ УТИЛ." : (ListItemData.mode == "open" ? "ОТКРЫТЬ" : "")
                                    tint: ListItemData.mode == "done" ? "#5bd68a" : "#ffb000"
                                    size: 5.5
                                }
                            }
                        }
                    ]
                }
            }
            Container {
                visible: grid.g.has != true
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1
                }
            }
            // нет таймера — сначала запустить; иначе отметить или снять отметку
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: ui.du(13)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                Key {
                    visible: grid.c.action == "off" && grid.st != "done"
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    kind: "primary"
                    icon: "play"
                    label: grid.st == "pause" ? "ПРОДОЛЖИТЬ ТАЙМЕР" : "СТАРТ ТАЙМЕРА"
                    labelSize: 7
                    onClicked: app.timerToggle()
                }
                Key {
                    visible: grid.c.action != "off"
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    kind: grid.c.action == "unmark" ? "danger" : "primary"
                    icon: grid.c.action == "unmark" ? "close" : "check"
                    label: grid.c.action == "unmark" ? "СНЯТЬ ОТМЕТКУ" : (grid.g.batch > 1 ? "УТИЛИЗИРОВАН × " + grid.g.batch : "УТИЛИЗИРОВАН")
                    labelSize: 7.5
                    active: grid.c.has == true
                    onClicked: app.primary()
                }
            }
        }
    }
}
