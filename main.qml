import QtQuick 2.4
import QtQuick.Window 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2

import cu.uci.cntlm 1.0

Window {
    visible: true

    title: "Cntlm Anywhere"
    height: content.implicitHeight + 70
    width: content.width + 40
    color: "#3F51B5"

    ColumnLayout {
        id: content
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 18
        width: 250

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            id: header

            Image {
                id: logo
                Layout.alignment: Qt.AlignLeft
                Layout.maximumWidth: 64
                Layout.maximumHeight: 64
                source: "res/Icon.png"
            }
            Label {
                text: qsTr("Cntlm Anywhere")
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                font.pixelSize: 22
                color: "white"
            }
            Image {
                id: helpButton

                visible: false
                Layout.maximumWidth: 44
                Layout.maximumHeight: 44
                Layout.alignment: Qt.AlignRight
                source: "res/ic_help_white_48dp.png"
            }
        }

        Label {
            text: qsTr("Usuario:")
            Layout.leftMargin: 6
            font.pixelSize: 16
            color: "white"
        }

        TextField {
            id: userTextField
            Layout.fillWidth: true
            font.pixelSize: 16
            style: FancyTextFieldStyle {
                selected: userTextField.activeFocus
                wrong: !userTextField.acceptableInput;
            }

            text: Cntlm.user
            onEditingFinished: Cntlm.user = text
            placeholderText: qsTr("user@domain.com")
            enabled: !Cntlm.running
        }

        Label {
            text: qsTr("Contraseña:")
            Layout.topMargin: 18
            Layout.leftMargin: 6
            font.pixelSize: 16
            color: "white"
        }

        Item {
            Layout.fillWidth: true
            height: passwordTextField.implicitHeight
            width: passwordTextField.implicitWidth
            TextField {
                id: passwordTextField

                anchors.fill: parent
                font.pixelSize: 16
                style: FancyTextFieldStyle {
                    selected: passwordTextField.activeFocus
                    wrong: !passwordTextField.acceptableInput;
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
                height: 24
                width: 24
                source: passwordTextField.echoMode
                        == TextInput.Password ? "res/ic_pass_show.png" : "res/ic_pass_hide.png"

                MouseArea {
                    anchors.fill: parent
                    onPressed: passwordTextField.echoMode = TextInput.Normal
                    onReleased: passwordTextField.echoMode = TextInput.Password
                }
            }
        }

        Button {
            id: controlButton
            height: 40
            Layout.topMargin: 18
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            style: ButtonStyle {
                background: Rectangle {
                    implicitWidth: 220
                    implicitHeight: 34
                    radius: 25
                    color: Cntlm.running ? "#4CAE50" : "#9E9E9E"
                }
                label: Text {
                    renderType: Text.NativeRendering
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.family: "Helvetica"
                    font.pointSize: 14
                    color: "white"
                    text: control.text
                }
            }

            enabled: !details.expanded
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
    }

    Component {
        id: detailsFields
        ColumnLayout {
            width: content.width
            Label {
                text: qsTr("Proxy")
                Layout.leftMargin: 6
                font.pixelSize: 16
            }
            TextField {
                id: proxyTextField
                style: FancyTextFieldStyle {
                    selected: proxyTextField.activeFocus
                    wrong: !proxyTextField.acceptableInput;
                    dark: false
                }
                inputMask: "000.000.000.000:00000"
                Layout.fillWidth: true
                font.pixelSize: 16
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
                Layout.topMargin: 18
                Layout.maximumWidth: parent.width
                wrapMode: Text.WordWrap
            }
        }
    }
}
