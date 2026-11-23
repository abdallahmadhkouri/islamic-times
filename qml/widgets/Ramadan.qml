import QtQuick
import "." as W

// Ramadan is a span: the fast begins at imsak and ends at maghrib, so the card
// shows both ends and where the household is between them.
Item {
    id: root
    property var day: typeof dayData !== "undefined" ? dayData : null
    property real base: Math.min(width, height) / 360
    property var r: root.day ? root.day.ramadan : null

    W.Chrome {
        anchors.fill: parent
        base: root.base
        mosque: root.day && root.day.mosque ? root.day.mosque.name : ""
        hijri: root.day ? (root.day.hijri.day + " " + W.Theme.monthNames[root.day.hijri.month]) : ""
    }

    // right: the title
    Column {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 26 * root.base
        anchors.topMargin: 56 * root.base
        spacing: 0
        Text {
            anchors.right: parent.right
            text: "رمضان"
            color: W.Theme.ink
            font.family: W.Theme.display
            font.pixelSize: 46 * root.base
        }
        Text {
            anchors.right: parent.right
            text: root.r && root.r.day > 0 ? ("اليوم " + W.Theme.num(root.r.day)) : "—"
            color: W.Theme.gold
            font.family: W.Theme.ui
            font.weight: Font.Bold
            font.pixelSize: 18 * root.base
        }
        Text {
            anchors.right: parent.right
            topPadding: 6 * root.base
            text: root.r ? (root.r.toIftar > 0 ? ("الإفطار بعد " + W.Theme.countdown(root.r.toIftar))
                                              : "الإمساك بعد " + W.Theme.countdown(root.r.toImsak))
                         : ""
            color: W.Theme.inkDim
            font.family: W.Theme.ui
            font.weight: Font.Medium
            font.pixelSize: 19 * root.base
        }
    }

    // left: the two ends of the fast, stacked and large
    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 26 * root.base
        anchors.topMargin: 58 * root.base
        spacing: 10 * root.base

        Row {
            spacing: 12 * root.base
            Text {
                text: "الإمساك"
                color: W.Theme.inkDim
                font.family: W.Theme.ui
                font.pixelSize: 14 * root.base
                anchors.baseline: parent.baseline
            }
            Text {
                text: root.r ? W.Theme.time(root.r.imsak) : "—"
                color: W.Theme.ink
                font.family: W.Theme.ui
                font.weight: Font.Bold
                font.pixelSize: 32 * root.base
            }
        }
        Row {
            spacing: 12 * root.base
            Text {
                text: "الإفطار"
                color: W.Theme.inkDim
                font.family: W.Theme.ui
                font.pixelSize: 14 * root.base
                anchors.baseline: parent.baseline
            }
            Text {
                text: root.r ? W.Theme.time(root.r.iftar) : "—"
                color: W.Theme.gold
                font.family: W.Theme.ui
                font.weight: Font.ExtraBold
                font.pixelSize: 32 * root.base
            }
        }
    }

    // the day as a line: the lit part is what has already been fasted
    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 26 * root.base
        anchors.rightMargin: 26 * root.base
        anchors.bottomMargin: 22 * root.base
        height: 6 * root.base

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: "#22FFFFFF"
        }
        Rectangle {
            width: parent.width * (root.r ? Math.max(0, Math.min(1, root.r.progress)) : 0)
            height: parent.height
            radius: height / 2
            color: W.Theme.gold
        }
    }
}
