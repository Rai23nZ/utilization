import bb.cascades 1.4

// Большая плитка файла на экране загрузки: пустая (янтарная рамка), читается,
// загружена (залита янтарём) или с ошибкой (красная рамка). Касание — выбор на SD-карте.
// Номер прижат к верху, подписи — к низу: вторая строка имени файла занимает пустое
// поле под крупной цифрой и не меняет размер плитки.
Container {
    id: tile
    property string kind: "check"
    property variant info: tile.kind == "check" ? app.checkInfo : app.actInfo
    property string st: tile.info.state ? tile.info.state : "empty"
    property bool ok: tile.st == "ok"
    property bool down: false

    layout: DockLayout {
    }
    background: Color.create(tile.st == "err" ? T.red : (tile.ok ? T.amber : (tile.down ? T.amber : T.amberEdge)))
    leftPadding: 3
    rightPadding: 3
    topPadding: 3
    bottomPadding: 3

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(tile.ok ? (tile.down ? T.amberDk : T.amber) : (tile.down ? T.keyDown : T.panel))
        leftPadding: ui.du(2.5)
        rightPadding: ui.du(2.5)
        topPadding: ui.du(2)
        bottomPadding: ui.du(2)
        layout: DockLayout {
        }

        // номер и значок состояния
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Top
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            MonoB {
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1
                }
                text: tile.kind == "check" ? "1" : "2"
                tint: tile.ok ? T.bg : T.amber
                size: 24
            }
            ImageView {
                imageSource: tile.ok ? "asset:///images/check.png" : (tile.st == "err" ? "asset:///images/close.png" : "asset:///images/sdcard.png")
                filterColor: Color.create(tile.ok ? T.bg : (tile.st == "err" ? T.red : T.amber))
                preferredWidth: ui.du(5)
                preferredHeight: ui.du(5)
            }
        }

        // подписи
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Bottom
            layout: StackLayout {
            }
            MonoB {
                text: tile.kind == "check" ? "РЕЗУЛЬТАТ ПРОВЕРКИ" : "АКТ СПИСАНИЯ"
                tint: tile.ok ? T.bg : T.ink
                size: 7
            }
            Mono {
                preferredWidth: ui.du(50)
                maxWidth: ui.du(50)
                text: tile.st == "empty" ? "коснитесь — выбрать\nна SD-карте" : tile.info.fileLines
                tint: tile.ok ? T.bg : (tile.st == "err" ? T.red : T.ink2)
                size: 5.5
                multiline: true
                autoSize.maxLineCount: 2
            }
            Mono {
                visible: tile.st != "empty"
                text: tile.st == "busy" ? "ЧИТАЮ…" : tile.info.line
                tint: tile.ok ? T.bg : (tile.st == "err" ? T.red : T.amber)
                size: 5.5
            }
        }
    }

    onTouch: {
        if (event.isDown()) tile.down = true;
        else if (event.isUp() || event.isCancel()) tile.down = false;
    }
    gestureHandlers: [
        TapHandler {
            onTapped: app.browse(tile.kind)
        }
    ]
}
