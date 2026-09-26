import bb.cascades 1.4

// Надпись моноширинным IBM Plex Mono Medium — основной шрифт «пульта».
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
            fontFamily: "PlexMono, monospace"
            rules: [
                FontFaceRule {
                    source: "asset:///fonts/PlexMono-Medium.ttf"
                    fontFamily: "PlexMono"
                }
            ]
        }
    ]
}
