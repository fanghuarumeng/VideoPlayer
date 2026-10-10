import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: root
    property var player
    // Drag only moves the knob; FFmpeg seek runs once on release.
    property bool scrubbing: false
    property real scrubRatio: 0
    property real seekRatio: {
        if (scrubbing)
            return scrubRatio
        if (player && player.durationSec > 0)
            return player.positionSec / player.durationSec
        return 0
    }
    property real displayPositionSec: {
        if (scrubbing && player)
            return scrubRatio * player.durationSec
        return player ? player.positionSec : 0
    }

    function formatTime(sec) {
        if (sec === undefined || sec < 0 || !isFinite(sec))
            sec = 0
        var t = Math.floor(sec + 0.5)
        var m = Math.floor(t / 60)
        var s = t % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

    function ratioAt(mouseX, barWidth) {
        if (barWidth <= 0)
            return 0
        return Math.max(0, Math.min(1, mouseX / barWidth))
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Text {
            text: formatTime(root.displayPositionSec) + " / "
                  + formatTime(player ? player.durationSec : 0)
            color: "#FFFFFF"
            font.pixelSize: 12
            font.family: "Segoe UI"
        }

        Item {
            Layout.fillWidth: true
            height: 16

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
                    if (!player || !(player.durationSec > 0))
                        return
                    root.scrubbing = true
                    root.scrubRatio = root.ratioAt(mouse.x, width)
                    player.setScrubbing(true)
                }
                onPositionChanged: {
                    // Visual only — continuous seek here caused decoder stalls.
                    if (pressed && player && player.durationSec > 0)
                        root.scrubRatio = root.ratioAt(mouse.x, width)
                }
                onReleased: {
                    if (!player)
                        return
                    if (player.durationSec > 0) {
                        root.scrubRatio = root.ratioAt(mouse.x, width)
                        player.seekRatio(root.scrubRatio)
                    }
                    root.scrubbing = false
                    player.setScrubbing(false)
                }
                onCanceled: {
                    root.scrubbing = false
                    if (player)
                        player.setScrubbing(false)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            ToolButton {
                flat: true
                hoverEnabled: true
                implicitWidth: 36
                implicitHeight: 36
                ToolTip.visible: hovered
                ToolTip.delay: 400
                ToolTip.text: "播放"
                onClicked: if (player) player.playSelectedOrResume()
                contentItem: Item {
                    implicitWidth: 22
                    implicitHeight: 22
                    Image {
                        anchors.fill: parent
                        source: "qrc:/app/icons/play.svg"
                        sourceSize: Qt.size(22, 22)
                        fillMode: Image.PreserveAspectFit
                    }
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent"
                    radius: 4
                }
            }
            ToolButton {
                flat: true
                hoverEnabled: true
                implicitWidth: 36
                implicitHeight: 36
                ToolTip.visible: hovered
                ToolTip.delay: 400
                ToolTip.text: player && player.paused ? "继续" : "暂停"
                onClicked: if (player) player.pauseToggle()
                contentItem: Item {
                    implicitWidth: 22
                    implicitHeight: 22
                    Image {
                        anchors.fill: parent
                        source: player && player.paused
                                ? "qrc:/app/icons/play.svg"
                                : "qrc:/app/icons/pause.svg"
                        sourceSize: Qt.size(22, 22)
                        fillMode: Image.PreserveAspectFit
                    }
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent"
                    radius: 4
                }
            }
            ToolButton {
                flat: true
                hoverEnabled: true
                implicitWidth: 36
                implicitHeight: 36
                ToolTip.visible: hovered
                ToolTip.delay: 400
                ToolTip.text: "停止"
                onClicked: if (player) player.stop()
                contentItem: Item {
                    implicitWidth: 22
                    implicitHeight: 22
                    Image {
                        anchors.fill: parent
                        source: "qrc:/app/icons/stop.svg"
                        sourceSize: Qt.size(22, 22)
                        fillMode: Image.PreserveAspectFit
                    }
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent"
                    radius: 4
                }
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                flat: true
                hoverEnabled: true
                implicitWidth: 36
                implicitHeight: 36
                ToolTip.visible: hovered
                ToolTip.delay: 400
                ToolTip.text: "下载"
                onClicked: if (player) player.downloadCurrent()
                contentItem: Item {
                    implicitWidth: 22
                    implicitHeight: 22
                    Image {
                        anchors.fill: parent
                        source: "qrc:/app/icons/download.svg"
                        sourceSize: Qt.size(22, 22)
                        fillMode: Image.PreserveAspectFit
                    }
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent"
                    radius: 4
                }
            }
        }
    }
}
