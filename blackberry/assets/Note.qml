import bb.cascades 1.4

// Заметка сверки: заголовок цветом и пояснение.
Container {
    id: note
    property string title: ""
    property string body: ""
    property string tint: "#9a978c"

    horizontalAlignment: HorizontalAlignment.Fill
    layout: StackLayout {
    }
    MonoB {
        text: (note.tint == T.green ? "✓ " : "! ") + note.title
        tint: note.tint
        size: 5.8
    }
    Sans {
        horizontalAlignment: HorizontalAlignment.Fill
        text: note.body
        tint: T.ink2
        size: 6
        multiline: true
    }
}
