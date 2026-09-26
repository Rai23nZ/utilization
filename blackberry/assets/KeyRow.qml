import bb.cascades 1.4

// Ряд из трёх цифровых клавиш.
Container {
    id: row
    property string a: ""
    property string b: ""
    property string c: ""

    horizontalAlignment: HorizontalAlignment.Fill
    layoutProperties: StackLayoutProperties {
        spaceQuota: 1
    }
    layout: StackLayout {
        orientation: LayoutOrientation.LeftToRight
    }
    Key {
        layoutProperties: StackLayoutProperties {
            spaceQuota: 1
        }
        label: row.a
        onClicked: app.key(row.a)
    }
    Key {
        leftMargin: ui.du(1)
        layoutProperties: StackLayoutProperties {
            spaceQuota: 1
        }
        label: row.b
        onClicked: app.key(row.b)
    }
    Key {
        leftMargin: ui.du(1)
        layoutProperties: StackLayoutProperties {
            spaceQuota: 1
        }
        label: row.c
        onClicked: app.key(row.c)
    }
}
