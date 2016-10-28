import QtQuick 2.0

Rectangle {
    id: details
    property bool expanded: false
    property alias content: contentLoader.sourceComponent
    property bool fitInScreen: false

    onFitInScreenChanged: {
        if (fitInScreen)
            expanded = true
        else if (expanded)
            expanded = false
    }

    anchors.left: parent.left
    anchors.right: parent.right

    y: parent.height - 50
    height: parent.height

    color: "#EEEEEE"
    Image {
        id: detailsController
        width: 64
        height: 32
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        source: "res/ic_btn_show.png"

        transform: [
            Rotation {
                id: xRot
                origin.x: 32
                origin.y: 16
                angle: 180
                axis {
                    x: 1
                    y: 0
                    z: 0
                }
            }
        ]

        states: State {
            name: "rotated"
            PropertyChanges {
                target: xRot
                angle: 0
            }

            when: details.expanded
        }

        transitions: Transition {
            NumberAnimation {
                target: xRot
                property: "angle"
                duration: 500
            }
        }
    }

    MouseArea {
        anchors.top: parent.top
        anchors.bottom: detailsController.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        onClicked: details.expanded = !details.expanded
    }

    states: State {
        name: "expanded"
        PropertyChanges {
            target: details
            y: (details.fitInScreen ? mainControls.y + mainControls.height : header.y
                                      + header.height) + 18
        }

        when: details.expanded
    }

    transitions: Transition {
        id: translationTransition
        reversible: true
        PropertyAnimation {
            properties: "y"
            easing.type: Easing.InOutQuad
            duration: 500
        }
    }

    Loader {
        id: contentLoader
        anchors.top: detailsController.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        visible: translationTransition.running || expanded
        sourceComponent: content
    }
}
