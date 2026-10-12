import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import QtGraphicalEffects 1.12
import VideoPlayer 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 600
    title: "Video Player"
    color: "#353A40"
    // Drop the native white title bar; chrome is drawn in-app.
    flags: Qt.Window | Qt.FramelessWindowHint

    property var player: playerBridge
    property bool isMaximized: visibility === Window.Maximized

    function toggleMaximize() {
        if (win.isMaximized)
            win.showNormal()
        else
            win.showMaximized()
    }

    // Full-window frame used only as blur source
    VideoFrameItem {
        id: blurSource
        anchors.fill: parent
        bridge: player
        fill: true
        visible: true
        opacity: 0.001
        z: -2
    }

    FastBlur {
        anchors.fill: parent
        source: blurSource
        radius: (player && player.hasFrame) ? 64 : 0
        transparentBorder: true
        visible: player && player.hasFrame
        z: -1
    }

    Rectangle {
        anchors.fill: parent
        z: 0
        color: (player && player.hasFrame) ? Qt.rgba(0.08, 0.09, 0.10, 0.72)
                                           : "#353A40"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: win.isMaximized ? 12 : 20
        spacing: 12
        z: 1

        // Custom title / nav bar (drag + transparent window buttons)
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 36

            MouseArea {
                id: titleDrag
                anchors.fill: parent
                anchors.rightMargin: 150
                property point pressPos
                onPressed: pressPos = Qt.point(mouse.x, mouse.y)
                onPositionChanged: {
                    if (!pressed || win.isMaximized)
                        return
                    win.x += mouse.x - pressPos.x
                    win.y += mouse.y - pressPos.y
                }
                onDoubleClicked: win.toggleMaximize()
            }

            RowLayout {
                anchors.fill: parent
                spacing: 12

                Text {
                    text: "Video Player"
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    font.bold: true
                    font.family: "Segoe UI"
                }

                Item { width: 8 }

                Button {
                    text: "\u89c6\u9891\u5e93"
                    onClicked: player.openLibrary()
                    contentItem: Text {
                        text: parent.text
                        color: player && player.overlay === "library" ? "#FFFFFF" : "#B0B3B8"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }
                    background: Rectangle {
                        color: player && player.overlay === "library"
                               ? Qt.rgba(1,1,1,0.14)
                               : (parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent")
                        radius: 6
                        implicitHeight: 28
                        implicitWidth: 72
                    }
                }
                Button {
                    text: "\u5386\u53f2"
                    onClicked: player.openHistory()
                    contentItem: Text {
                        text: parent.text
                        color: player && player.overlay === "history" ? "#FFFFFF" : "#B0B3B8"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }
                    background: Rectangle {
                        color: player && player.overlay === "history"
                               ? Qt.rgba(1,1,1,0.14)
                               : (parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent")
                        radius: 6
                        implicitHeight: 28
                        implicitWidth: 56
                    }
                }
                Button {
                    text: "\u641c\u7d22"
                    onClicked: player.openSearch()
                    contentItem: Text {
                        text: parent.text
                        color: player && player.overlay === "search" ? "#FFFFFF" : "#B0B3B8"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }
                    background: Rectangle {
                        color: player && player.overlay === "search"
                               ? Qt.rgba(1,1,1,0.14)
                               : (parent.hovered ? Qt.rgba(1,1,1,0.10) : "transparent")
                        radius: 6
                        implicitHeight: 28
                        implicitWidth: 56
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    radius: 14
                    color: player && player.liveConnected ? "#16301f" : "#3a1a1a"
                    border.color: player && player.liveConnected ? "#22c55e" : "#ef4444"
                    implicitHeight: 28
                    implicitWidth: liveLabel.implicitWidth + 24
                    Text {
                        id: liveLabel
                        anchors.centerIn: parent
                        text: player && player.liveConnected ? "Live" : "Offline"
                        color: player && player.liveConnected ? "#22c55e" : "#ef4444"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                Row {
                    spacing: 0
                    TitleBarButton {
                        text: "\u2013"
                        onClicked: win.showMinimized()
                    }
                    TitleBarButton {
                        text: win.isMaximized ? "\u2750" : "\u25A1"
                        onClicked: win.toggleMaximize()
                    }
                    TitleBarButton {
                        text: "\u2715"
                        danger: true
                        onClicked: win.close()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#1E2226"
                    clip: true

                    VideoFrameItem {
                        id: mainVideo
                        anchors.fill: parent
                        bridge: player
                        fill: true
                        visible: player && player.hasFrame
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: !(player && player.hasFrame)
                        text: "\u25B6"
                        color: Qt.rgba(1, 1, 1, 0.85)
                        font.pixelSize: 64
                    }

                    Text {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: 56
                        visible: !(player && player.hasFrame)
                        text: player ? player.hintText : ""
                        color: "#B0B3B8"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        width: parent.width * 0.7
                        wrapMode: Text.WordWrap
                    }

                    MouseArea {
                        anchors.fill: parent
                        onDoubleClicked: if (player) player.playSelectedOrResume()
                    }
                }

                Text {
                    text: player && player.title ? player.title : "\u9009\u62e9\u89c6\u9891\u5f00\u59cb\u64ad\u653e"
                    color: "#FFFFFF"
                    font.pixelSize: 20
                    font.bold: true
                    font.family: "Segoe UI"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                PlayerControls {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    player: win.player
                }
            }

            Rectangle {
                Layout.preferredWidth: 340
                Layout.maximumWidth: 380
                Layout.fillHeight: true
                color: Qt.rgba(1, 1, 1, 0.06)
                border.color: Qt.rgba(1, 1, 1, 0.10)

                UpNext {
                    anchors.fill: parent
                    player: win.player
                }
            }
        }

        Text {
            text: player ? player.statusText : ""
            color: "#B0B3B8"
            font.pixelSize: 12
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }

    // Glass overlays reuse the same blurred video backdrop as the player.
    GlassOverlay {
        anchors.fill: parent
        z: 20
        visible: player && player.overlay === "library"
        titleText: "\u89c6\u9891\u5e93"
        onCloseRequested: if (player) player.closeOverlay()
        LibraryView {
            anchors.fill: parent
            player: win.player
        }
    }
    GlassOverlay {
        anchors.fill: parent
        z: 20
        visible: player && player.overlay === "history"
        titleText: "\u5386\u53f2"
        onCloseRequested: if (player) player.closeOverlay()
        HistoryView {
            anchors.fill: parent
            player: win.player
        }
    }
    GlassOverlay {
        anchors.fill: parent
        z: 20
        visible: player && player.overlay === "search"
        titleText: "\u641c\u7d22"
        onCloseRequested: if (player) player.closeOverlay()
        SearchView {
            anchors.fill: parent
            player: win.player
        }
    }
}
