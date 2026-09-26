import bb.cascades 1.4

// Метка в рамке: код товара, торговая марка, цена.
Container {
    id: tag
    property string label: ""
    property bool hi: false

    background: Color.create(tag.hi ? T.amber : T.line)
    leftPadding: 2
    rightPadding: 2
    topPadding: 2
    bottomPadding: 2
    Container {
        background: Color.create(T.panel)
        leftPadding: ui.du(1.2)
        rightPadding: ui.du(1.2)
        topPadding: ui.du(0.4)
        bottomPadding: ui.du(0.4)
        MonoB {
            text: tag.label
            tint: tag.hi ? T.amber : T.ink2
            size: 6.5
        }
    }
}
