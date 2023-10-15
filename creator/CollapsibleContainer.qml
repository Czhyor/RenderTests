import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

ColumnLayout {
    id: modelListPanel
    implicitWidth: 200
    spacing: 0

    property Item content

    property bool collapsed: !showModelListViewBtn.checked
    onCollapsedChanged: {
        console.log("collapsed cahgned", collapsed)
        console.log("content:", content)
        if(content != null) {
            console.log("set content visible", !collapsed)
            content.visible = !collapsed
        }
    }

    property alias title: showModelListViewBtn.text

    Button {
        id: showModelListViewBtn
        height: 20
        Layout.fillWidth: true
        text: "title"
        onTextChanged: {
            console.log("text changed:", text)
        }

        checkable: true
        checked: true
        onCheckedChanged: {
            console.log("checked cahgned", checked)
        }

        background: Rectangle {
            anchors.fill: parent
            color: "skyblue"
        }
    }


    onContentChanged: {
        console.log("content chagned:", content)
        if(content != null) {
            console.log("set content parent")
            content.parent = modelListPanel
            content.width = Qt.binding(function(){ return modelListPanel.width })
        }
    }
}
