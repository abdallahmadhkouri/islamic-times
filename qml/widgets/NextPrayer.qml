import QtQuick
import "." as W

// The smallest useful tile: which prayer, and how long. State changes the accent,
// never the layout, so the eye keeps its place at three metres.
Item {
    id: root
    property var day: typeof dayData !== "undefined" ? dayData : null
    property real base: Math.min(width, height) / 360

    W.Chrome {
        anchors.fill: parent
        base: root.base
        mosque: root.day && root.day.mosque ? root.day.mosque.name : ""
        hijri: root.day ? (root.day.hijri.day + " " + W.Theme.monthNames[root.day.hijri.month]) : ""
    }

    property var next: root.day ? root.day.next : null
    property color accent: !root.next ? W.Theme.inkFaint
                          : root.next.state === "waitingIqama" ? W.Theme.amber
                          : root.next.state === "inPrayer" ? W.Theme.teal
                          : W.Theme.gold

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 12 * root.base
        spacing: 2 * root.base

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.next ? W.Theme.prayerNames[root.next.key] : "—"
            color: W.Theme.ink
            font.family: W.Theme.display
            font.pixelSize: 60 * root.base
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: W.Theme.headline(root.next)
            color: root.accent
            font.family: W.Theme.ui
            font.weight: Font.ExtraBold
            font.pixelSize: 34 * root.base
            style: Text.Raised
            styleColor: "#33000000"
        }

        Item { width: 1; height: 10 * root.base }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.next
                  ? (W.Theme.stateNames[root.next.state] + (root.next.at ? " · " + root.next.at : ""))
                  : ""
            color: W.Theme.inkDim
            font.family: W.Theme.ui
            font.pixelSize: 15 * root.base
        }
    }

}
