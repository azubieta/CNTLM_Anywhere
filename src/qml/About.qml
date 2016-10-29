import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4

Rectangle {
    signal dispose
    color: "#3F51B5"

    MouseArea {
        anchors.fill: parent
        onReleased: {
            dispose()
            mouse.accepted = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: style.marginLarge
        Text {
            text: qsTr("Acerca de CNTLM Anywhere")
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }
        Text {
            text: qsTr("CNTLM Anywhere es un empaquetado de cntlm para múltiples plataformas.")
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }

        Text {
            text: qsTr("Desarrollado por:\nAlexis López Zubieta\nDiseñado por:\nYulio Alemán Jimenez")
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }

        Text {
            text: qsTr("Código original de CNTLM copiright de David Kubicek")
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Image {
                id: gihub
                source: "qrc:/res/drawable/generic/github_mark.png"
                Layout.maximumWidth: style.iconSizeLarge
                Layout.maximumHeight: style.iconSizeLarge

                MouseArea {
                    anchors.fill: gihub
                    onPressed: {
                        mouse.accepted = true
                        print("github")
                        Qt.openUrlExternally(
                                    "https://github.com/azubieta/cntlm_anywhere")
                    }
                }
            }
        }
    }
}
