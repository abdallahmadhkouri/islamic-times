import QtQuick
import "." as W

// The card every widget sits in: one gradient, one hairline, one glow, one
// crescent mark. Widgets differ in what they put inside it, never in how it is lit.
Item {
    id: root
    property real base: Math.min(width, height) / 360
    property bool compact: width < height * 1.2
    property string mosque: ""
    property string hijri: ""

    Rectangle {
        anchors.fill: parent
        radius: 26 * root.base
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#122033" }
            GradientStop { position: 0.55; color: W.Theme.bg1 }
            GradientStop { position: 1.0; color: W.Theme.bg0 }
        }
        border.width: 1
        border.color: W.Theme.line
    }

    // a soft warm bloom from the top-right, like lamplight on the card edge
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        width: parent.width * 0.7
        height: parent.height * 0.7
        radius: width / 2
        opacity: 0.5
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#2ADDB36B" }
            GradientStop { position: 1.0; color: "#00DDB36B" }
        }
    }

    // the crescent mark, drawn as a disc with a cut — no shader, no Canvas
    Item {
        id: moon
        width: 30 * root.base
        height: 30 * root.base
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 18 * root.base
        opacity: 0.9
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: W.Theme.gold
        }
        Rectangle {
            width: parent.width * 0.92
            height: parent.height * 0.92
            radius: width / 2
            x: parent.width * 0.28
            y: -parent.height * 0.10
            color: "#101B2B"
        }
    }

    Text {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 18 * root.base
        text: root.hijri
        color: W.Theme.inkDim
        font.family: W.Theme.ui
        font.weight: Font.Medium
        font.pixelSize: 13 * root.base
    }

    Text {
        anchors.left: moon.right
        anchors.top: parent.top
        anchors.topMargin: 20 * root.base
        anchors.leftMargin: 10 * root.base
        text: root.mosque.length ? root.mosque : (root.hijri.length ? "حساب فلكي" : "")
        color: W.Theme.inkDim
        font.family: W.Theme.ui
        font.pixelSize: 14 * root.base
        width: parent.width * 0.52
        elide: Text.ElideLeft
    }
}
