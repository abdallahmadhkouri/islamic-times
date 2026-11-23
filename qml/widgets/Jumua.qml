import QtQuick
import "." as W

// Friday has its own shape: the khutbah is the appointment, not the adhan. One
// centred composition, so the eye never has to pair a label with a value across
// the width of a wide tile.
Item {
    id: root
    property var day: typeof dayData !== "undefined" ? dayData : null
    property real base: Math.min(width, height) / 360
    property var j: root.day ? root.day.jumuah : null

    W.Chrome {
        anchors.fill: parent
        base: root.base
        mosque: root.day && root.day.mosque ? root.day.mosque.name : ""
        hijri: root.day ? (root.day.hijri.day + " " + W.Theme.monthNames[root.day.hijri.month]) : ""
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 8 * root.base
        spacing: 0

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "الجمعة"
            color: W.Theme.ink
            font.family: W.Theme.display
            font.pixelSize: 46 * root.base
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.j && root.j.isToday && root.j.khutbah ? W.Theme.time(root.j.khutbah) : "—"
            color: W.Theme.gold
            font.family: W.Theme.ui
            font.weight: Font.ExtraBold
            font.pixelSize: root.j && root.j.isToday ? 40 * root.base : 26 * root.base
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.j && root.j.isToday
                  ? ("الخطبة · بعد " + W.Theme.countdown(root.j.seconds))
                  : ("بعد " + (root.j ? root.j.daysAway : 0) + " أيام")
            color: W.Theme.inkDim
            font.family: W.Theme.ui
            font.pixelSize: 16 * root.base
        }
    }
}
