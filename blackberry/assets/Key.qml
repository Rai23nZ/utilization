import bb.cascades 1.4

// Клавиша «пульта»: цифра, функция, главная (янтарная) или опасная (снять отметку).
Container {
    id: key
    property string label: ""
    property string sub: ""
    property string icon: ""
    property string kind: "digit"
    property bool active: true
    property bool down: false
    property real labelSize: key.kind == "digit" ? 16 : 6
    signal clicked()
    signal longClicked()

    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    layout: DockLayout {
    }
    background: Color.create(key.kind == "primary" ? T.amber : (key.kind == "danger" ? T.red : T.line))
    leftPadding: 2
    rightPadding: 2
    topPadding: 2
    bottomPadding: 2
    opacity: key.active ? 1.0 : 0.35

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layout: DockLayout {
        }
        background: Color.create(key.kind == "primary" ? (key.down ? T.amberDk : T.amber) : (key.down ? T.keyDown : (key.kind == "digit" ? T.key : T.keyFn)))

        Container {
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            layout: StackLayout {
            }
            ImageView {
                visible: key.icon != ""
                horizontalAlignment: HorizontalAlignment.Center
                imageSource: key.icon != "" ? "asset:///images/" + key.icon + ".png" : "asset:///images/dot.png"
                filterColor: Color.create(key.kind == "primary" ? T.bg : (key.kind == "danger" ? T.red : (key.kind == "digit" ? T.ink : T.ink2)))
                preferredWidth: ui.du(key.kind == "primary" ? 6.5 : 4)
                preferredHeight: ui.du(key.kind == "primary" ? 6.5 : 4)
                scalingMethod: ScalingMethod.AspectFit
                bottomMargin: ui.du(0.8)
            }
            MonoB {
                visible: key.label != ""
                horizontalAlignment: HorizontalAlignment.Center
                text: key.label
                size: key.labelSize
                align: "center"
                tint: key.kind == "primary" ? T.bg : (key.kind == "danger" ? T.red : (key.kind == "digit" ? T.ink : T.ink2))
            }
            Mono {
                visible: key.sub != ""
                horizontalAlignment: HorizontalAlignment.Center
                text: key.sub
                size: 4.5
                align: "center"
                tint: key.kind == "primary" ? T.bg : T.dim
            }
        }
    }

    onTouch: {
        if (!key.active) {
            key.down = false;
            return;
        }
        if (event.isDown()) {
            key.down = true;
        } else if (event.isUp() || event.isCancel()) {
            key.down = false;
        }
    }
    gestureHandlers: [
        TapHandler {
            onTapped: {
                if (key.active) key.clicked();
            }
        },
        LongPressHandler {
            onLongPressed: {
                if (key.active) key.longClicked();
            }
        }
    ]
}
