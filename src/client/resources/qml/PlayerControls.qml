import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: root
    property var player
    property real seekRatio: player && player.durationSec > 0
                              ? player.positionSec / player.durationSec : 0

    function formatTime(sec) {
        if (sec === undefined || sec < 0 || !isFinite(sec))
            sec = 0
        var t = Math.floor(sec + 0.5)
        var m = Math.floor(t / 60)
        var s = t % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Text {
            text: formatTime(player ? player.positionSec : 0) + " / "
                  + formatTime(player ? player.durationSec : 0)
            color: "#FFFFFF"
            font.pixelSize: 12
            font.family: "Segoe UI"
        }

        Item {
            Layout.fillWidth: true
            height: 16

            // Track
            Rectangle {
                id: track
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 4
                radius: 2
                color: Qt.rgba(1, 1, 1, 0.20)

                Rectangle {
                    width: parent.width * Math.min(1, Math.max(0, player ? player.bufferRatio : 0))
                    height: parent.height
                    radius: 2
                    color: Qt.rgba(1, 1, 1, 0.35)
                }
                Rectangle {
                    width: parent.width * Math.min(1, Math.max(0, root.seekRatio))
                    height: parent.height
                    radius: 2
                    color: "#5086FF"
                }
            }

            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: "#FFFFFF"
                anchors.verticalCenter: parent.verticalCenter
                x: Math.max(0, Math.min(parent.width - width,
                                        parent.width * root.seekRatio - width / 2))
            }

            MouseArea {
                anchors.fill: parent
                onPressed: {
                    if (player)
                        player.setScrubbing(true)
                }
                onReleased: {
                    if (!player)
                        return
                    var r = Math.max(0, Math.min(1, mouse.x / width))
                    player.seekRatio(r)
                    player.setScrubbing(false)
                }
                onPositionChanged: {
                    if (pressed && player && player.durationSec > 0) {
                        var r = Math.max(0, Math.min(1, mouse.x / width))
                        // visual only via seek while dragging
                        player.seekRatio(r)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ToolButton {
                text: "▶"
                flat: true
                onClicked: if (player) player.playSelectedOrResume()
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent"
                }
            }
            ToolButton {
                text: player && player.paused ? "▶" : "❚❚"
                flat: true
                onClicked: if (player) player.pauseToggle()
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent"
                }
            }
            ToolButton {
                text: "■"
                flat: true
                onClicked: if (player) player.stop()
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent"
                }
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                text: "⇩"
                flat: true
                onClicked: if (player) player.downloadCurrent()
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent"
                }
            }
        }
    }
}
