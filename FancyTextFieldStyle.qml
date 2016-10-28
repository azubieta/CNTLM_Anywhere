import QtQuick 2.0
import QtQuick.Controls.Styles 1.4

TextFieldStyle {
    property bool selected: false;
    property bool wrong: false;
    property bool dark: true;

    textColor: dark ? "white" : "black"
    placeholderTextColor: "lightgray";

    background: Rectangle {
        color: "transparent"

        Rectangle {
            anchors.bottom: parent.bottom;
            anchors.left:  parent.left;
            anchors.right: parent.right;
            anchors.leftMargin: 6;
            anchors.rightMargin: 6;
            property string iddleColor: dark ? "black" : "lightgray";
            property string activeColor: dark ? "white" : "black";
            property string wrongColor: "red";

            height:  selected || wrong ? 1.6 : 1.2;
            color: wrong ? wrongColor : selected ? activeColor : iddleColor;
        }
    }
}

