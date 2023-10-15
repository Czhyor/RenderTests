import QtQuick.Controls 2.12
import QtQuick 2.12

Rectangle {
    id: modelListRec
    color: "grey"
    width: 100
    height: 50

    signal objectChecked(obj: var)

    ListModel {
        id: models
    }

    ListView {
        id: modelListView
        anchors.fill: parent
        model: models
        focus: true
        delegate: Item {
            id: modelElement
            width: parent.width
            height: 20

            property var object: model.object
            Text {
                id: name
                text: model.name
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    modelListView.currentIndex = index
                }
            }
        }
        highlight: Rectangle {
            color: "lightsteelblue"
            border.width: 1
            border.color: "black"
        }

        onCurrentItemChanged: {
            console.log("currentItem changed, object:", currentItem.object)
            objectChecked(currentItem.object)
        }

        Connections {
            target: $Interface
            function onNodeAdded(node) {
                models.append({ "name": node.objectName, "object": node })
            }
            function onLightAdded(light) {
                models.append({ "name": light.objectName, "object": light })
            }
        }
    }
}
