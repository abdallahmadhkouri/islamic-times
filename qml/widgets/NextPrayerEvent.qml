import QtQuick
import "." as W

// The wide tile: the prayer that is coming, and the feast that is further away.
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
    property var event: root.day ? root.day.event : null

    Row {
        anchors.fill: parent
        anchors.topMargin: 58 * root.base
        anchors.bottomMargin: 18 * root.base
        anchors.leftMargin: 22 * root.base
        anchors.rightMargin: 22 * root.base

        Item {
            width: parent.width * 0.56
            height: parent.height

            Column {
                anchors.centerIn: parent
                spacing: 0
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.next ? W.Theme.prayerNames[root.next.key] : "—"
                    color: W.Theme.ink
                    font.family: W.Theme.display
                    font.pixelSize: 44 * root.base
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: W.Theme.headline(root.next)
                    color: root.next && root.next.state === "waitingIqama" ? W.Theme.amber
                         : root.next && root.next.state === "inPrayer" ? W.Theme.teal : W.Theme.gold
                    font.family: W.Theme.ui
                    font.weight: Font.ExtraBold
                    font.pixelSize: 26 * root.base
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.next ? W.Theme.stateNames[root.next.state] : ""
                    color: W.Theme.inkFaint
                    font.family: W.Theme.ui
                    font.pixelSize: 11 * root.base
                }
            }
        }

        Rectangle {
            width: 1
            height: parent.height * 0.7
            anchors.verticalCenter: parent.verticalCenter
            color: "#1FFFFFFF"
        }

        Item {
            width: parent.width * 0.44 - 1
            height: parent.height

            Column {
                anchors.centerIn: parent
                spacing: 2 * root.base
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.event && root.event.nextId ? W.Theme.eventNames[root.event.nextId] : ""
                    color: W.Theme.ink
                    font.family: W.Theme.display
                    font.pixelSize: 30 * root.base
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.event && root.event.nextId ? W.Theme.days(root.event.daysAway) : ""
                    color: W.Theme.inkDim
                    font.family: W.Theme.ui
                    font.pixelSize: 15 * root.base
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.event && root.event.currentId ? ("الآن: " + W.Theme.eventNames[root.event.currentId]) : ""
                    color: W.Theme.inkDim
                    font.family: W.Theme.ui
                    font.pixelSize: 12 * root.base
                }
            }
        }
    }
}
