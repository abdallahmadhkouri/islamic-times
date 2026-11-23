import QtQuick
import "." as W

// The calendar tile: today's Hijri date, and the next thing the year is leading to.
Item {
    id: root
    property var day: typeof dayData !== "undefined" ? dayData : null
    property real base: Math.min(width, height) / 360
    property var e: root.day ? root.day.event : null

    W.Chrome {
        anchors.fill: parent
        base: root.base
        mosque: ""
        hijri: ""
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -18 * root.base
        spacing: 0
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.day ? W.Theme.num(root.day.hijri.day) : "—"
            color: W.Theme.ink
            font.family: W.Theme.display
            font.pixelSize: 62 * root.base
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.day ? W.Theme.monthNames[root.day.hijri.month] : ""
            color: W.Theme.gold
            font.family: W.Theme.display
            font.pixelSize: 26 * root.base
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.day ? W.Theme.num(root.day.hijri.year) + " هـ" : ""
            color: W.Theme.inkFaint
            font.family: W.Theme.ui
            font.pixelSize: 12 * root.base
        }
    }

    Column {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 18 * root.base
        spacing: 1 * root.base
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.e && root.e.nextId ? W.Theme.eventNames[root.e.nextId] : ""
            color: W.Theme.ink
            font.family: W.Theme.display
            font.pixelSize: 22 * root.base
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.e && root.e.nextId ? W.Theme.days(root.e.daysAway) : ""
            color: W.Theme.inkDim
            font.family: W.Theme.ui
            font.pixelSize: 12 * root.base
        }
    }
}
