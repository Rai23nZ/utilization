import bb.cascades 1.4

// Экран «Итог»: чек процесса и выгрузка на SD-карту в папку utilization.
Container {
    id: done
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    background: Color.create(T.bg)
    layout: StackLayout {
    }

    function refresh() {
        receiptModel.clear();
        var rows = app.receipt;
        for (var i = 0; i < rows.length; i++) receiptModel.append(rows[i]);
    }
    onCreationCompleted: {
        app.summaryChanged.connect(done.refresh);
        done.refresh();
    }
    attachedObjects: [
        ArrayDataModel {
            id: receiptModel
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
        ImageView {
            verticalAlignment: VerticalAlignment.Center
            imageSource: "asset:///images/stop.png"
            filterColor: Color.create(T.ink2)
            preferredWidth: ui.du(2)
            preferredHeight: ui.du(2)
        }
        MonoB {
            verticalAlignment: VerticalAlignment.Center
            leftMargin: ui.du(1)
            text: "ЗАВЕРШЕНА"
            tint: T.ink2
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
    }
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 2
        background: Color.create(T.line)
    }

    // ---------- чек
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        layoutProperties: StackLayoutProperties {
            spaceQuota: 1
        }
        leftPadding: ui.du(3.5)
        rightPadding: ui.du(3.5)
        topPadding: ui.du(2)
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(T.line)
            leftPadding: 2
            rightPadding: 2
            topPadding: 2
            bottomPadding: 2
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(T.panel)
                leftPadding: ui.du(2.5)
                rightPadding: ui.du(2.5)
                topPadding: ui.du(1.5)
                bottomPadding: ui.du(1.5)
                ListView {
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    dataModel: receiptModel
                    listItemComponents: [
                        ListItemComponent {
                            type: ""
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout {
                                }
                                // разделитель чека
                                Container {
                                    visible: ListItemData.c == "sep"
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topMargin: 16
                                    bottomMargin: 16
                                    preferredHeight: 2
                                    background: Color.create("#3a3d38")
                                }
                                // строка «показатель — значение»
                                Container {
                                    visible: ListItemData.c != "sep" && ListItemData.c != "rest" && ListItemData.c != "undo"
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topPadding: 4
                                    bottomPadding: 4
                                    layout: StackLayout {
                                        orientation: LayoutOrientation.LeftToRight
                                    }
                                    Mono {
                                        layoutProperties: StackLayoutProperties {
                                            spaceQuota: 1
                                        }
                                        text: ListItemData.k
                                        tint: ListItemData.c == "dim" ? "#9a978c" : "#e8e4d8"
                                        size: ListItemData.c == "head" ? 7 : 6.5
                                    }
                                    MonoB {
                                        text: ListItemData.v
                                        tint: ListItemData.c == "amber" ? "#ffb000" : (ListItemData.c == "red" ? "#ff6b5e" : (ListItemData.c == "dim" ? "#9a978c" : "#e8e4d8"))
                                        size: ListItemData.c == "head" ? 7 : 6.5
                                    }
                                }
                                // неотмеченная позиция или снятая отметка
                                Container {
                                    visible: ListItemData.c == "rest" || ListItemData.c == "undo"
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topPadding: 4
                                    bottomPadding: 4
                                    layout: StackLayout {
                                        orientation: LayoutOrientation.LeftToRight
                                    }
                                    MonoB {
                                        text: ListItemData.k
                                        tint: ListItemData.c == "rest" ? "#ff6b5e" : "#ffb000"
                                        size: 6.5
                                    }
                                    Sans {
                                        leftMargin: 30
                                        layoutProperties: StackLayoutProperties {
                                            spaceQuota: 1
                                        }
                                        text: ListItemData.v
                                        tint: "#e8e4d8"
                                        size: 6
                                    }
                                }
                            }
                        }
                    ]
                }
            }
        }
    }

    // ---------- строка выгрузки
    Container {
        visible: app.exportNote != ""
        horizontalAlignment: HorizontalAlignment.Fill
        leftPadding: ui.du(3.5)
        rightPadding: ui.du(3.5)
        topPadding: ui.du(1.2)
        MonoB {
            horizontalAlignment: HorizontalAlignment.Fill
            text: (app.exportNote.indexOf("ОШИБКА") >= 0 || app.exportNote.indexOf("НЕТ SD") >= 0 ? "× " : "✓ ") + app.exportNote
            tint: app.exportNote.indexOf("ОШИБКА") >= 0 || app.exportNote.indexOf("НЕТ SD") >= 0 ? T.red : T.green
            size: 5.2
            multiline: true
        }
    }

    // ---------- клавиши
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: ui.du(16)
        leftPadding: ui.du(3.5)
        rightPadding: ui.du(3.5)
        topPadding: ui.du(1.5)
        bottomPadding: ui.du(2.5)
        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }
        Key {
            layoutProperties: StackLayoutProperties {
                spaceQuota: 2
            }
            kind: "primary"
            label: "ВЫГРУЗИТЬ .XLSX"
            sub: "В SD-КАРТА / UTILIZATION"
            labelSize: 7.5
            onClicked: app.exportXlsx()
        }
        Key {
            leftMargin: ui.du(1)
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            kind: "fn"
            label: "ТЕКСТ .TXT"
            sub: "+ КОПИЯ В БУФЕР"
            onClicked: app.exportText()
        }
        Key {
            leftMargin: ui.du(1)
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            kind: "fn"
            label: "ЗАНОВО"
            sub: "НОВАЯ ВЫБОРКА"
            onClicked: app.requestRestart()
        }
    }
}
