import bb.cascades 1.4

// Экран «Работа остановлена»: в акте есть товар, не допущенный к утилизации.
Container {
    id: block
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    background: Color.create(T.bg)
    layout: StackLayout {
    }

    function refresh() {
        extraModel.clear();
        var list = app.extraList;
        for (var i = 0; i < list.length; i++) extraModel.append(list[i]);
    }
    onCreationCompleted: {
        app.blockChanged.connect(block.refresh);
        block.refresh();
    }
    attachedObjects: [
        ArrayDataModel {
            id: extraModel
        }
    ]

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
            text: "× РАБОТА ОСТАНОВЛЕНА"
            tint: T.red
            size: 6.5
        }
    }
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 3
        background: Color.create(T.red)
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        leftPadding: ui.du(3.5)
        rightPadding: ui.du(3.5)
        topPadding: ui.du(2)
        layout: StackLayout {
        }
        Sans {
            text: "Акт нужно исправить"
            tint: T.ink
            size: 11
        }
        Sans {
            horizontalAlignment: HorizontalAlignment.Fill
            text: "Акт списания содержит товар, не допущенный к утилизации. Продолжить нельзя, пока акт не будет исправлен."
            tint: T.ink2
            size: 6.5
            multiline: true
        }
        Sans {
            visible: app.extraNote != ""
            topMargin: ui.du(1)
            horizontalAlignment: HorizontalAlignment.Fill
            text: app.extraNote
            tint: T.amber
            size: 6.5
            multiline: true
        }
        MonoB {
            topMargin: ui.du(1.8)
            text: "ЛИШНИЕ КНТ · " + app.extraList.length
            tint: T.ink2
            size: 6
        }
    }
    Container {
        topMargin: ui.du(1)
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 2
        background: Color.create(T.lineSoft)
    }

    ListView {
        horizontalAlignment: HorizontalAlignment.Fill
        layoutProperties: StackLayoutProperties {
            spaceQuota: 1
        }
        dataModel: extraModel
        listItemComponents: [
            ListItemComponent {
                type: ""
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                    }
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 96
                        leftPadding: 42
                        rightPadding: 42
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }
                        MonoB {
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }
                            text: ListItemData.knt
                            tint: "#e8e4d8"
                            size: 8
                        }
                        Mono {
                            verticalAlignment: VerticalAlignment.Center
                            text: ListItemData.why
                            tint: ListItemData.miss ? "#ffb000" : "#ff6b5e"
                            size: 6
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
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: ui.du(15)
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
            icon: "sheet"
            label: "КОПИРОВАТЬ СПИСОК"
            sub: "И СОХРАНИТЬ .TXT В UTILIZATION"
            labelSize: 7
            onClicked: app.copyExtra()
        }
        Key {
            leftMargin: ui.du(1)
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1.2
            }
            kind: "fn"
            icon: "folder"
            label: "ДРУГОЙ АКТ"
            onClicked: app.browse("act")
        }
        Key {
            leftMargin: ui.du(1)
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            kind: "fn"
            icon: "arrow_left"
            label: "НАЗАД"
            onClicked: app.blockBack()
        }
    }
}
