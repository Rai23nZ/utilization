import bb.cascades 1.4

// Надпись IBM Plex Mono SemiBold — цифры, клавиши, заголовки.
Label {
    id: txt
    property string tint: "#e8e4d8"
    property real size: 7
    property string align: "left"
    textStyle.base: ts.style
    textStyle.color: Color.create(txt.tint)
    textStyle.fontSize: FontSize.PointValue
    textStyle.fontSizeValue: txt.size
    textStyle.textAlign: txt.align == "center" ? TextAlign.Center : (txt.align == "right" ? TextAlign.Right : TextAlign.Left)
    attachedObjects: [
        TextStyleDefinition {
            id: ts
            fontFamily: "PlexMonoSB, monospace"
            rules: [
                FontFaceRule {
                    source: "asset:///fonts/PlexMono-SemiBold.ttf"
                    fontFamily: "PlexMonoSB"
                }
            ]
        }
    ]
}
