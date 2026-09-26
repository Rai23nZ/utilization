import bb.cascades 1.4

// Экран «Выбор файла»: папки и таблицы SD-карты крупными строками.
Container {
    id: files
    property bool isCheck: app.browseKind == "check"
    property variant other: files.isCheck ? app.actInfo : app.checkInfo

    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    background: Color.create(T.bg)
    layout: StackLayout {
    }

    function refresh() {
        filesModel.clear();
        var items = app.browseItems;
        for (var i = 0; i < items.length; i++) filesModel.append(items[i]);
    }
    onCreationCompleted: {
        app.browseChanged.connect(files.refresh);
        files.refresh();
    }
    attachedObjects: [
        ArrayDataModel {
            id: filesModel
        }
    ]

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
            text: "ВЫБОР ФАЙЛА"
            tint: T.amber
            size: 6.5
        }
        Mono {
            verticalAlignment: VerticalAlignment.Center
            text: files.isCheck ? "1 · ПРОВЕРКА" : "2 · АКТ"
            tint: T.ink2
            size: 6
        }
    }
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 2
        background: Color.create(T.line)
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        leftPadding: ui.du(3.5)
        rightPadding: ui.du(3.5)
        topPadding: ui.du(2)
        layout: StackLayout {
        }
        Sans {
            text: files.isCheck ? "Результат проверки" : "Акт списания"
            tint: T.ink
            size: 11
        }
        Mono {
            text: "коснитесь файла · .xlsx  .xls  .csv"
            tint: T.ink2
            size: 5.5
        }
        // второй файл, если уже выбран
        Container {
            visible: files.other.state == "ok"
            topMargin: ui.du(1.5)
            horizontalAlignment: HorizontalAlignment.Fill
            background: Color.create(T.line)
            leftPadding: 2
            rightPadding: 2
            topPadding: 2
            bottomPadding: 2
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(T.panel)
                leftPadding: ui.du(1.8)
                rightPadding: ui.du(1.8)
                topPadding: ui.du(1)
                bottomPadding: ui.du(1)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                MonoB {
                    verticalAlignment: VerticalAlignment.Center
                    text: files.isCheck ? "2 · АКТ ✓" : "1 · ПРОВЕРКА ✓"
                    tint: T.green
                    size: 5.5
                }
                Mono {
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: ui.du(1.5)
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    text: files.other.file ? files.other.file : ""
                    tint: T.ink
                    size: 5.5
                }
            }
        }
        // путь
        Container {
            topMargin: ui.du(1.8)
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            ImageView {
                verticalAlignment: VerticalAlignment.Center
                imageSource: "asset:///images/sdcard.png"
                filterColor: Color.create(T.ink2)
                preferredWidth: ui.du(2.6)
                preferredHeight: ui.du(2.6)
            }
            Mono {
                verticalAlignment: VerticalAlignment.Center
                leftMargin: ui.du(1)
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1
                }
                text: app.browsePath
                tint: T.ink2
                size: 6
            }
            Mono {
                verticalAlignment: VerticalAlignment.Center
                text: app.browseNote
                tint: app.sdReady ? T.ink2 : T.red
                size: 5.5
            }
        }
    }
    Container {
        topMargin: ui.du(1)
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 2
        background: Color.create(T.lineSoft)
    }

    // ---------- список
    ListView {
        horizontalAlignment: HorizontalAlignment.Fill
        layoutProperties: StackLayoutProperties {
            spaceQuota: 1
        }
        dataModel: filesModel
        onTriggered: {
            app.browseOpen(indexPath[0]);
        }
        listItemComponents: [
            ListItemComponent {
                type: ""
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                    }
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 118
                        leftPadding: 42
                        rightPadding: 42
                        background: Color.create(ListItemData.sel != "" ? "#231d0c" : "#0a0b0a")
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }
                        ImageView {
                            verticalAlignment: VerticalAlignment.Center
                            imageSource: ListItemData.dir ? "asset:///images/folder.png" : "asset:///images/sheet.png"
                            filterColor: Color.create(ListItemData.dir ? "#9a978c" : "#ffb000")
                            preferredWidth: 40
                            preferredHeight: 40
                        }
                        MonoB {
                            verticalAlignment: VerticalAlignment.Center
                            leftMargin: 28
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            text: ListItemData.name
                            tint: ListItemData.sel != "" ? "#5bd68a" : (ListItemData.dir ? "#9a978c" : "#e8e4d8")
                            size: 6.5
                        }
                        Mono {
                            verticalAlignment: VerticalAlignment.Center
                            text: ListItemData.sel != "" ? "ВЫБРАН · " + ListItemData.sel : ListItemData.info
                            tint: ListItemData.sel != "" ? "#5bd68a" : "#9a978c"
                            size: 5
                        }
                    }
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 2
                        background: Color.create("#1d1f1c")
                    }
                }
            }
        ]
    }
    Container {
        visible: !app.sdReady
        horizontalAlignment: HorizontalAlignment.Fill
        leftPadding: ui.du(3.5)
        rightPadding: ui.du(3.5)
        bottomPadding: ui.du(2)
        Sans {
            horizontalAlignment: HorizontalAlignment.Fill
            text: "SD-карта не найдена. Вставьте карту с файлами и нажмите «Обновить»."
            tint: T.red
            size: 7
            multiline: true
        }
    }

    // ---------- клавиши
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: ui.du(14)
        leftPadding: ui.du(3.5)
        rightPadding: ui.du(3.5)
        topPadding: ui.du(1.5)
        bottomPadding: ui.du(2.5)
        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }
        Key {
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            kind: "fn"
            icon: "arrow_left"
            label: "НАЗАД"
            onClicked: app.browseCancel()
        }
        Key {
            leftMargin: ui.du(1)
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            kind: "fn"
            icon: "arrow_up"
            label: "ВВЕРХ ПАПКИ"
            active: app.browseCanUp
            onClicked: app.browseUp()
        }
        Key {
            leftMargin: ui.du(1)
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            kind: "fn"
            icon: "sdcard"
            label: "ОБНОВИТЬ"
            onClicked: app.refreshSd()
        }
    }
}
