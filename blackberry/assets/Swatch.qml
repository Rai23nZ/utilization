import bb.cascades 1.4

// Образец цвета для легенды сетки.
Container {
    id: sw
    property string fill: "#161816"
    property string edge: "#2a2d29"
    property string label: ""

    layout: StackLayout {
        orientation: LayoutOrientation.LeftToRight
    }
    Container {
        verticalAlignment: VerticalAlignment.Center
        preferredWidth: ui.du(2.6)
        preferredHeight: ui.du(2.6)
        background: Color.create(sw.edge)
        leftPadding: 4
        rightPadding: 4
        topPadding: 4
        bottomPadding: 4
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(sw.fill)
        }
    }
    Mono {
        verticalAlignment: VerticalAlignment.Center
        leftMargin: ui.du(0.8)
        text: sw.label
        tint: T.ink2
        size: 4.8
    }
}
