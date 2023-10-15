import QtQuick 2.0
import QtQuick.Controls 2.12

ScrollView {
    id: propertyList
    width: parent.width
    height: 200

    Column {
        anchors.fill: parent
        
        Rectangle {
            color: "green"
            width: parent.width
            height: 18
            border.width: 1
            border.color: "black"
            Text {
                text: qsTr("backgroudColor")
            }

            Switch {
                width: 50
                height: parent.height
                anchors.right: parent.right
                checkable: true
                checked: false
                onCheckedChanged: {
                    console.log("switch backgroudColor")
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
                text: qsTr("enableDeferedRendering")
            }

            Switch {
                width: 50
                height: parent.height
                anchors.right: parent.right
                checkable: true
                checked: false
                onCheckedChanged: {
                    console.log("switch enableDeferedRendering")
                    if (checked) {
                        $Interface.setDeferredRendering();
                    } else {
                        $Interface.setForwardRendering();
                    }
                }
            }
        }

        Button {
            background: Rectangle {
                color: "lightgrey"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("enableViewDatum")
            }

            onClicked: {
                $Interface.enableViewDatum();
            }
        }

        Button {
            background: Rectangle {
                color: "lightgrey"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("enableIntersect")
            }
            checkable: false
            checked: false
            onClicked: {
                $Interface.enableIntersect(checked);
            }
        }

        Button {
            background: Rectangle {
                color: parent.checked ? "green" : "grey"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("usePreViewMaterial")
            }
            onClicked: {
                $Interface.setShaderMode(0);
            }
        }

        Button {
            background: Rectangle {
                color: parent.checked ? "green" : "grey"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("usePBR")
            }

            onClicked: {
                $Interface.setShaderMode(2);
            }
        }

        Button {
            background: Rectangle {
                color: parent.checked ? "green" : "grey"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("usePhong")
            }

            onClicked: {
                $Interface.setShaderMode(1);
            }
        }

        Button {
            background: Rectangle {
                color: "lightgrey"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("showFrustum")
            }
            checkable: false
            checked: true
            onCheckedChanged: {
                $Interface.showFrustum();
            }
        }
        
        Button {
            background: Rectangle {
                color: parent.checked ? "green" : "grey"
                border.width: 1
                border.color: "black"
            }

            width: parent.width
            height: 18

            Text {
                text: qsTr("useShadow")
            }
            checkable: true
            checked: true
            onCheckedChanged: {
                $Interface.useShadow(checked);
            }
        }
    }
}

