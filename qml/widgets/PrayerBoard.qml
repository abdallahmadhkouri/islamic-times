import QtQuick
import "." as W

// The tile that holds everything: every prayer for the day, the Hijri date, the one
// being waited for with its countdown, and the next event underneath. Right-to-left,
// because that is how the list is read.
Item {
    id: root
    property var day: typeof dayData !== "undefined" ? dayData : null
    property real base: Math.min(width, height) / 360
    property var next: root.day ? root.day.next : null
    property bool hasIqama: {
        if (!root.day)
            return false
        for (var i = 0; i < root.day.prayers.length; ++i) {
            if (root.day.prayers[i].iqama)
                return true
        }
        return false
    }

    W.Chrome {
        anchors.fill: parent
        base: root.base
        mosque: root.day && root.day.mosque ? root.day.mosque.name : ""
        hijri: root.day ? (root.day.hijri.day + " " + W.Theme.monthNames[root.day.hijri.month] + " · " + root.day.hijri.year) : ""
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 52 * root.base
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 34 * root.base
        spacing: 0

        // the hero: what is being waited for, and how long
        Item {
            width: parent.width
            height: 72 * root.base

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 12 * root.base
                anchors.verticalCenter: parent.verticalCenter
                text: root.next ? W.Theme.prayerNames[root.next.key] : "—"
                color: W.Theme.ink
                font.family: W.Theme.display
                font.pixelSize: 34 * root.base
            }

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 12 * root.base
                anchors.verticalCenter: parent.verticalCenter
                spacing: -2 * root.base
                Text {
                    text: W.Theme.headline(root.next)
                    color: root.next && root.next.state === "waitingIqama" ? W.Theme.amber
                         : root.next && root.next.state === "inPrayer" ? W.Theme.teal : W.Theme.gold
                    font.family: W.Theme.ui
                    font.weight: Font.ExtraBold
                    font.pixelSize: 32 * root.base
                }
                Text {
                    text: root.next
                          ? (W.Theme.stateNames[root.next.state] + (root.next.at ? " · " + root.next.at : ""))
                          : ""
                    color: W.Theme.inkDim
                    font.family: W.Theme.ui
                    font.pixelSize: 12 * root.base
                }
            }
        }

        // the day itself
        Repeater {
            model: root.day ? root.day.prayers : []
            delegate: Item {
                required property var modelData
                width: parent ? parent.width : 0
                height: 31 * root.base

                readonly property bool isNext: root.day && root.day.next && root.day.next.key === modelData.key
                readonly property bool passed: modelData.state === "passed"

                Rectangle {
                    visible: isNext
                    anchors.fill: parent
                    radius: 8 * root.base
                    color: "#16DDB36B"
                }
                Rectangle {
                    visible: isNext
                    width: 3 * root.base
                    height: parent.height * 0.55
                    radius: 2 * root.base
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    color: W.Theme.gold
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 12 * root.base
                    text: W.Theme.prayerNames[modelData.key] || modelData.key
                    color: isNext ? W.Theme.gold : passed ? W.Theme.inkFaint : W.Theme.ink
                    font.family: W.Theme.display
                    font.pixelSize: 19 * root.base
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: (root.hasIqama ? 132 : 88) * root.base
                    text: W.Theme.time(modelData.adhan)
                    color: isNext ? W.Theme.ink : passed ? W.Theme.inkFaint : W.Theme.inkDim
                    font.family: W.Theme.ui
                    font.weight: isNext ? Font.Bold : Font.Medium
                    font.pixelSize: isNext ? 18 * root.base : 16 * root.base
                }

                Text {
                    visible: root.hasIqama
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 26 * root.base
                    text: modelData.iqama ? W.Theme.time(modelData.iqama) : "—"
                    color: isNext ? W.Theme.inkDim : W.Theme.inkFaint
                    font.family: W.Theme.ui
                    font.pixelSize: 13 * root.base
                }
            }
        }

        // the next event, so this one tile really does carry everything
        Item {
            width: parent.width
            height: 30 * root.base
            visible: root.day && root.day.event && root.day.event.nextId

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: "#14FFFFFF"
            }
            Text {
                anchors.centerIn: parent
                text: root.day && root.day.event && root.day.event.nextId
                      ? (W.Theme.eventNames[root.day.event.nextId] + " · " + W.Theme.days(root.day.event.daysAway))
                      : ""
                color: W.Theme.inkDim
                font.family: W.Theme.ui
                font.pixelSize: 12.5 * root.base
            }
        }
    }
}
