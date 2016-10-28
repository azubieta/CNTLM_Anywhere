import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2

Rectangle {
    signal dispose
    color: "#3F51B5"
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: style.marginLarge
        Text {
            text: "Acerca de CNTLM Anywhere"
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }
        Text {
            text: "CNTLM Anywhere es un empaquetado de cntlm para múltiples plataformas."
            color: "white"
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
        }

        Text {
            text: "Desarrollado por:\nAlexis López Zubieta"
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
