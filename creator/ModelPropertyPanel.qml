import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
ScrollView {
    id: propertyList
    implicitWidth: 200
    implicitHeight: 200

    property var obj: null
    onObjChanged: {
        console.log("ModelPropertyPanel onObjChanged:", obj)
    }

    Column {
        anchors.fill: parent

        Rectangle {
            color: "green"
            width: parent.width
            height: 18
            border.width: 1
            border.color: "black"
            Text {
                text: qsTr("enableGravity")
            }

            Switch {
                width: 50
                height: parent.height
                anchors.right: parent.right
                checkable: true
                checked: (obj != null) ? obj.gravityEnabled : false
                onCheckedChanged: {
                    obj.gravityEnabled = checked
                }
            }
        }

        Rectangle {
            color: "green"
            width: parent.width
            height: 18
            border.width: 1
            border.color: "black"
            Text {
                text: qsTr("showLine")
            }

            Switch {
                width: 50
                height: parent.height
                anchors.right: parent.right
                checkable: true
                checked: (obj != null) ? obj.isShowLine : false
                onCheckedChanged: {
                    obj.isShowLine = checked
                }
            }
        }

        Button {
            background: Rectangle {
                color: "green"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("enableManipulator")
            }

            onClicked: {
                $Interface.enableManipulate(obj);
            }
        }

        Button {
            background: Rectangle {
                color: "green"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("enableDrag")
            }

            onClicked: {
                $Interface.enableDrag(obj);
            }
        }

        Slider {
            from: 0.0
            value: 0
            to: 1.0
            stepSize: 0.01
            onValueChanged: {
                obj.metallic = value;
            }
        }
        Slider {
            from: 0.0
            value: 1
            to: 1.0
            stepSize: 0.01
            onValueChanged: {
                obj.roughness = value;
            }
        }
    }
}

