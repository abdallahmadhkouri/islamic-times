import QtQuick
import "." as W

// The whole day on one card, laid out right-to-left the way an Arabic list is read:
// the prayer on the right, the adhan beside it, the jamaa at the far left.
Item {
    id: root
    property var day: typeof dayData !== "undefined" ? dayData : null
    property real base: Math.min(width, height) / 360

    W.Chrome {
        anchors.fill: parent
        base: root.base
        mosque: root.day && root.day.mosque ? root.day.mosque.name : ""
        hijri: root.day ? (root.day.hijri.day + " " + W.Theme.monthNames[root.day.hijri.month] + " · " + root.day.hijri.year) : ""
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 66 * root.base
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 36 * root.base
        spacing: 2 * root.base

        Repeater {
            model: root.day ? root.day.prayers : []
            delegate: Item {
                required property var modelData
                width: parent.width
                height: 44 * root.base

                readonly property bool isNext: root.day && root.day.next && root.day.next.key === modelData.key
                readonly property bool passed: modelData.state === "passed"

                Rectangle {
                    visible: isNext
                    anchors.fill: parent
                    radius: 10 * root.base
                    color: "#16DDB36B"
                }

                Rectangle {
                    visible: isNext
                    width: 3 * root.base
                    height: parent.height * 0.6
                    radius: 2 * root.base
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    color: W.Theme.gold
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 14 * root.base
                    text: W.Theme.prayerNames[modelData.key] || modelData.key
                    color: isNext ? W.Theme.gold : passed ? W.Theme.inkFaint : W.Theme.ink
                    font.family: W.Theme.display
                    font.pixelSize: 23 * root.base
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 150 * root.base
                    text: W.Theme.time(modelData.adhan)
                    color: isNext ? W.Theme.ink : passed ? W.Theme.inkFaint : W.Theme.inkDim
                    font.family: W.Theme.ui
                    font.weight: isNext ? Font.Bold : Font.Medium
                    font.pixelSize: isNext ? 22 * root.base : 19 * root.base
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 30 * root.base
                    text: modelData.iqama ? W.Theme.time(modelData.iqama) : "—"
                    color: isNext ? W.Theme.inkDim : W.Theme.inkFaint
                    font.family: W.Theme.ui
                    font.pixelSize: 15 * root.base
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: "#10FFFFFF"
                }
            }
        }
    }
}
