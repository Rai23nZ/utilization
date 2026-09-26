import bb.cascades 1.4

// Показатель сверки: число и подпись.
Container {
    id: stat
    property string value: ""
    property string label: ""
    property string tint: "#e8e4d8"

    layout: StackLayout {
    }
    MonoB {
        text: stat.value
        tint: stat.tint
        size: 13
    }
    Mono {
        text: stat.label
        tint: T.ink2
        size: 4.8
    }
}
