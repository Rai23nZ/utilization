import bb.cascades 1.4
import bb.system 1.2

// Утилизация КНТ для BlackBerry 10 (Passport, 1440×1440).
// Экраны переключает контроллер: app.screen = load | files | block | work | grid | done.
Page {
    id: page
    actionBarVisibility: ChromeVisibility.Hidden

    keyListeners: [
        KeyListener {
            onKeyPressed: {
                app.keyPress(event.key, event.unicode);
            }
        }
    ]

    content: Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(T.bg)
        layout: DockLayout {
        }
        LoadScreen {
            visible: app.screen == "load"
        }
        FilesScreen {
            visible: app.screen == "files"
        }
        BlockScreen {
            visible: app.screen == "block"
        }
        WorkScreen {
            visible: app.screen == "work"
        }
        GridScreen {
            visible: app.screen == "grid"
        }
        DoneScreen {
            visible: app.screen == "done"
        }
    }

    attachedObjects: [
        SystemToast {
            id: toast
        },
        SystemDialog {
            id: dlg
            property string action: ""
            onFinished: {
                if (value == SystemUiResult.ConfirmButtonSelection) app.confirmed(dlg.action);
            }
        },
        SceneCover {
            id: cover
            content: Cover {
            }
        }
    ]

    function showToast(text) {
        toast.body = text;
        toast.show();
    }
    function askConfirm(action, title, body, yes) {
        dlg.action = action;
        dlg.title = title;
        dlg.body = body;
        dlg.confirmButton.label = yes;
        dlg.cancelButton.label = "Отмена";
        dlg.show();
    }
    onCreationCompleted: {
        Application.setCover(cover);
        app.toast.connect(page.showToast);
        app.confirm.connect(page.askConfirm);
    }
}
