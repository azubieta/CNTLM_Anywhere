import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2

Rectangle {
    signal dispose
    color: "#3F51B5"
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        Text {
            text: "Acerca de CNTLM Anywhere"
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }

        Text {
            text: "Desarrollado por:\nAlexis López Zubieta <azubieta90[at]gmail[dot]com>"
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }

        Text {
            text: "Diseñado por:\nYulio Alemán Jimenez"
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }

        Text {
            text: "Código original de CNTLM copiright de David Kubicek"
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }
    }
    MouseArea {
        anchors.fill: parent
        onClicked: dispose()
    }
}
