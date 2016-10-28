import QtQuick 2.4
import QtQuick.Window 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2

import cu.uci.cntlmanywhere 1.0

Window {
    id: root
    visible: true
    title: "Cntlm Anywhere"
    color: "#3F51B5"

    Component.onCompleted: {
        if (Qt.platform.os == "linux" || Qt.platform.os == "windows") {
            root.height = 600;
            root.width = 300;
        }
    }

    Item {
        id: style
        property int scaleFactor: {
            if (Qt.platform.os != "android" && Qt.platform.os != "ios"
                    && Qt.platform.os != "blackberry")
                return 1

            if (Screen.PixelDensity < 40)
                return 1
            else if (Screen.PixelDensity > 300)
                return 4
            else
                return 2
        }

        property int contentWidth: 200 * scaleFactor
        property int fontPointSize: 16 * scaleFactor
        property int fontPointSizeSmall: 12 * scaleFactor
        property int iconSizeMedium: 24 * scaleFactor
        property int iconSizeLarge: 64 * scaleFactor
        property int marginSmall: 12 * scaleFactor
        property int marginLarge: 18 * scaleFactor
    }

    RowLayout {
        id: header
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: style.marginLarge

        Image {
            id: logo
            Layout.alignment: Qt.AlignLeft
            Layout.maximumWidth: style.iconSizeLarge
            Layout.maximumHeight: style.iconSizeLarge
            source: "res/Icon.png"

            MouseArea {
                anchors.fill: parent
                onClicked: aboutDialog.visible = !aboutDialog.visible
            }
        }
        Label {
            text: qsTr("Cntlm Anywhere")
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft
            font.pixelSize: style.fontPointSize
            color: "white"
        }
        Image {
            id: helpButton

            visible: false
            Layout.maximumWidth: style.iconSizeLarge
            Layout.maximumHeight: style.iconSizeLarge
            Layout.alignment: Qt.AlignRight
            source: "res/ic_help_white_48dp.png"
        }
    }

    ColumnLayout {
        id: mainControls
        anchors.top: header.bottom

        anchors.horizontalCenter: parent.horizontalCenter
        width: style.contentWidth
        spacing: style.marginLarge

        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Usuario:")
                //            Layout.leftMargin: 6
                font.pixelSize: style.fontPointSize
                color: "white"
            }

            TextField {
                id: userTextField
                Layout.fillWidth: true
                font.pixelSize: style.fontPointSize
                style: FancyTextFieldStyle {
                    selected: userTextField.activeFocus
                    wrong: !userTextField.acceptableInput
                }

                text: Cntlm.user
                onEditingFinished: Cntlm.user = text
                placeholderText: qsTr("user@domain.com")
                enabled: !Cntlm.running
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Contraseña:")
                //             Layout.topMargin: 18
                //             Layout.leftMargin: 6
                font.pixelSize: style.fontPointSize
                color: "white"
            }

            Item {
                Layout.fillWidth: true
                height: passwordTextField.implicitHeight
                // width: passwordTextField.implicitWidth
                TextField {
                    id: passwordTextField

                    anchors.fill: parent
                    font.pixelSize: style.fontPointSize
                    style: FancyTextFieldStyle {
                        selected: passwordTextField.activeFocus
                        wrong: !passwordTextField.acceptableInput
                    }
                    text: Cntlm.password
                    onEditingFinished: Cntlm.password = text
                    placeholderText: qsTr("Your Password")
                    echoMode: TextInput.Password
                    enabled: !Cntlm.running
                }

                Image {
                    id: showPasswordImage
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    height: style.iconSizeMedium
                    width: style.iconSizeMedium
                    source: passwordTextField.echoMode
                            == TextInput.Password ? "res/ic_pass_show.png" : "res/ic_pass_hide.png"

                    MouseArea {
                        anchors.fill: parent
                        onPressed: passwordTextField.echoMode = TextInput.Normal
                        onReleased: passwordTextField.echoMode = TextInput.Password
                    }
                }
            }
        }

        Button {
            id: controlButton
            height: 40
            //            Layout.topMargin: 18
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            style: ButtonStyle {
                background: Rectangle {
                    implicitWidth: style.contentWidth - (15 * style.scaleFactor)
                    implicitHeight: implicitWidth / 6
                    radius: 25
                    color: Cntlm.running ? "#4CAE50" : "#9E9E9E"
                }
                label: Text {
                    renderType: Text.NativeRendering
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.family: "Helvetica"
                    //font.pointSize: style.fontPointSize
                    color: "white"
                    text: control.text
                }
            }

            // enabled: !details.expanded
            checked: Cntlm.running
            text: Cntlm.running ? qsTr("DETENER") : qsTr("INICIAR")
            onClicked: {
                if (!Cntlm.running)
                    Cntlm.start()
                else
                    Cntlm.stop()
                checked = Cntlm.running
            }
        }
    }

    BottomExpandablePanel {
        id: details
        content: detailsFields
        fitInScreen: mainControls.height * 2 < root.height
    }

    Component {
        id: detailsFields
        ScrollView {
            implicitWidth: mainControls.width
            ColumnLayout {
                width: mainControls.width
                Label {
                    text: qsTr("Proxy")
                    //                Layout.leftMargin: 6
                    //font.pixelSize: style.fontPointSize
                }
                TextField {
                    id: proxyTextField
                    style: FancyTextFieldStyle {
                        selected: proxyTextField.activeFocus
                        wrong: !proxyTextField.acceptableInput
                        dark: false
                    }
                    inputMask: "000.000.000.000:00000"
                    Layout.fillWidth: true
                    //font.pixelSize: style.fontPointSize
                    text: Cntlm.proxy
                    onEditingFinished: Cntlm.proxy = text
                    placeholderText: qsTr("10.0.0.1:8080")
                    enabled: !Cntlm.running
                }

                Label {
                    id: statusMessage

                    text: qsTr("CNTLM en ejecución, establezca como proxy para su sistema: \""
                               + Cntlm.listen + "\"")
                    visible: Cntlm.running
                    //                Layout.topMargin: 18
                    Layout.maximumWidth: parent.width
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Dialog {
        id: errorDialog
        property alias message: innerText.text
        visible: false
        title: "Error"
        contentItem: Rectangle {
            color: "#FF4081"

            implicitHeight: root.height - (root.height / 4)
            implicitWidth: root.width - (root.width / 4)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: style.marginLarge
                spacing: style.marginLarge
                Text {
                    color: "white"
                    text: "Ups, algo fue mal ... "
                    font.pixelSize: style.fontPointSize
                    Layout.maximumWidth: parent.width
                    wrapMode: Text.WordWrap
                }

                Text {
                    id: innerText
                    color: "white"
                    font.pixelSize: style.fontPointSizeSmall
                    Layout.maximumWidth: parent.width
                    Layout.fillHeight: true;
                    wrapMode: Text.WordWrap
                }

                Text {
                    color: "white"
                    text: "Inténtelo nuevamente ;)"
                    font.pixelSize: style.fontPointSize
                    Layout.maximumWidth: parent.width
                    wrapMode: Text.WordWrap
                }
                Text {
                    color: "white"
                    text: "Si el problema periste reporte el error."
                    font.pixelSize: style.fontPointSizeSmall
                    Layout.maximumWidth: parent.width
                    wrapMode: Text.WordWrap
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: errorDialog.visible = false
            }
        }

        Connections {
            target: Cntlm
            onError: {
                errorDialog.message = msg
                errorDialog.visible = true
            }
        }
    }

    Dialog {
        id: aboutDialog
        title: "Acerca de Cntlm Anywhere"

        contentItem: About {
            implicitHeight: root.height - (root.height / 4)
            implicitWidth: root.width - (root.width / 4)

            onDispose: aboutDialog.visible = false
        }
    }
}
