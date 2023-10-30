import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs
import QtQuick.Layouts 1.12
import Canvas3D 1.0
//import "MyButton.qml"
import Engine.Node 1.0

ApplicationWindow {
	visible: true
    x: 50
    y: 50
    width: 1280
    height: 960

    FileDialog {
        id: fileDialog
        onSelectedFileChanged: {
            console.log("selected file:", selectedFile)
        }
    }

    menuBar: MenuBar {
        //Menu {
        //    title: qsTr("Project")
        //    Action {
        //        text: qsTr("New")
        //        onCheckedChanged: {
        //            console.log("checked changed", checked)
        //        }
        //        onTriggered: {
        //            console.log("action triggered")
        //        }
        //    }
        //    Action {
        //        text: qsTr("open")
        //    }
        //    Action {
        //        text: qsTr("save")
        //    }
        //    Action {
        //        text: qsTr("run")
        //    }
        //}
        Menu {

            title: qsTr("Edit")

            Menu {
                title: qsTr("add")
                //Action {
                //    text: qsTr("map")
                //}
                MenuItem {
                    text: qsTr("model")
                    onTriggered: {
                        con.target = fileDialog
                        fileDialog.open();
                    }

                    Connections {
                        id: con
                        target: null
                        function onSelectedFileChanged() {
                            con.target = null;
                            console.log("call addModel:", fileDialog.selectedFile)
                            $Interface.addModel(fileDialog.selectedFile);
                        }
                    }
                }
                Action {
                    text: qsTr("customized")
                    onTriggered: {
                        $Interface.addCustomized();
                    }
                }
                Action {
                    text: qsTr("billboard")
                    onTriggered: {
                        $Interface.addBillboard(fileDialog.selectedFile);
                    }
                }

                Action {
                    text: qsTr("cubemap")
                    onTriggered: {
                        $Interface.addCubeMap();
                    }
                }
                Action {
                    text: qsTr("directionalLight")
                    onTriggered: {
                        $Interface.addDirectionalLight();
                    }
                }
                 Action {
                    text: qsTr("pointLight")
                    onTriggered: {
                        $Interface.addPointLight();
                    }
                }
                Action {
                    text: qsTr("spotLight")
                    onTriggered: {
                        $Interface.addSpotLight();
                    }
                }
            }

            //Action {
            //    text: qsTr("object")
            //}
            //
            //Action {
            //    text: qsTr("show")
            //}
            //Action {
            //    text: qsTr("hide")
            //}
        }
        //Menu {
        //    title: qsTr("View")
        //    Action {
        //        text: qsTr("tool")
        //    }
        //}
        //
        //Menu {
        //    title: qsTr("Debug")
        //    Action {
        //        text: qsTr("run")
        //        onTriggered: {
        //            $Interface.run();
        //        }
        //    }
        //    Action {
        //        text: qsTr("stop")
        //        onTriggered: {
        //            $Interface.stop();
        //        }
        //    }
        //
        //    Action {
        //        text: qsTr("reset")
        //        onTriggered: {
        //            $Interface.reset();
        //        }
        //    }
        //}
    }

	Canvas3d {
        id: canvas
		anchors.fill: parent
        onWidthChanged: {
            console.log("canvas width changed:", width)
        }
        onHeightChanged: {
            console.log("canvas height changed:", height)
        }
        Component.onCompleted: {
            console.log("canvas compeleted, description:", canvas.description);
            var renderInfo = canvas.renderInfo;
            console.log("canvas.renderInfo:", renderInfo);
            $Interface.setRenderInfo(renderInfo);
        }
	}

    CollapsibleContainer {
        id: modelListContainer
        anchors.right: parent.right
        anchors.top: menuBar.bottom
        title: "show/hide model list"
        content: ModelListPanel {
            id: modelListPanel
            height: 200
        }
    }

    TabBar {
        id: propertyBar
        width: modelListContainer.width
        anchors.top: modelListContainer.bottom
        anchors.right: modelListContainer.right

        TabButton {
            text: qsTr("SceneProperty")
        }
        TabButton {
            text: qsTr("ModelProperty")
        }
    }

    SwipeView {
        width: modelListContainer.width
        anchors.top: propertyBar.bottom
        anchors.right: propertyBar.right
        currentIndex: propertyBar.currentIndex
        clip: true

        CollapsibleContainer {
            title: "show/hide scene list"
            content: ScenePropertyPanel {
                height: 300
            }
        }

        CollapsibleContainer {
            title: "show/hide property list"
            content: ModelPropertyPanel {
                id: modelPropertyPanel
                height: 300
                Connections {
                    target: modelListPanel
                    function onObjectChecked(obj) {
                        modelPropertyPanel.obj = obj;
                    }
                }
            }
        }
    }


}
