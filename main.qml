import QtQuick 2.4
import QtQuick.Window 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2

import cu.uci.cntlm 1.0

Window {
    visible: true

    title: "Cntlm Plus"
    height: content.implicitHeight + 40
    width: content.implicitWidth + 40

    ColumnLayout {
        id: content
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20

        Label {
            text: qsTr("Cntlm Plus")
            font.pixelSize: 22

        }
        Label {
            text: qsTr("One CNTLM to rull them all..")
            color: "steelblue"
            font.italic: true

        }

        Label {
            text: qsTr("User")
        }
        TextField {
            id: userTextField
            text: Cntlm.user
            onEditingFinished: Cntlm.user = text
            placeholderText: qsTr("user@domain")
            enabled: !Cntlm.running
        }

        Label {
            text: qsTr("Password")
        }
        TextField {
            id: passwordTextField
            text: Cntlm.password
            onEditingFinished: Cntlm.password = text
            placeholderText: qsTr("Your Password")
            echoMode: TextInput.Password
            enabled: !Cntlm.running
        }

        Label {
            text: qsTr("Proxy")
        }
        TextField {
            id: proxyTextField
            text: Cntlm.proxy
            onEditingFinished: Cntlm.proxy = text
            placeholderText: qsTr("10.0.0.1:8080")
            enabled: !Cntlm.running
        }
        Button {
            id: controlButton
            style: ButtonStyle {
                  background: Rectangle {
                      implicitWidth: 100
                      implicitHeight: 25
                      border.width: control.activeFocus ? 2 : 1
                      border.color: "#888"
                      radius: 4
                      gradient: Gradient {
                          GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
                          GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
                      }

                      Rectangle {
                          implicitWidth: 6
                          implicitHeight: 18
                          radius: 4
                          border.color: "black"
                          border.width: 1
                          anchors {
                            right: parent.right
                            rightMargin: 12
                            verticalCenter: parent.verticalCenter
                          }
                          color: Cntlm.running ? "lightgreen" : "transparent"
                      }
                  }
              }

            height: 60
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            checked: Cntlm.running
            text: Cntlm.running ? qsTr("Stop") : qsTr("Start")
            onClicked: {
                if (!Cntlm.running)
                    Cntlm.start()
                else
                    Cntlm.stop()
                checked = Cntlm.running
            }
        }

        Label {
            id: statusMessage

            text:  qsTr("CNTLM is running in background. Setup your system proxy to \"" + Cntlm.listen + "\".");
            visible: Cntlm.running
            Layout.maximumWidth: parent.width;
            wrapMode: Text.WordWrap
        }
    }


}

