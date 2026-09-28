import bb.cascades 1.4

// Надпись IBM Plex Sans SemiBold — наименования товара и пояснения.
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
            fontFamily: "PlexSans, sans-serif"
            rules: [
                FontFaceRule {
                    source: "asset:///fonts/PlexSans-SemiBold.ttf"
                    fontFamily: "PlexSans"
                }
            ]
        }
    ]
}
