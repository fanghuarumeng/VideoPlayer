import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: root
    property var player

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "\u4e0a\u4f20"
                onClicked: if (player) player.uploadVideos()
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.down ? "#3d6ad4"
                                       : (parent.hovered ? "#5a93ff" : "#5086FF")
                    radius: 6
                    implicitHeight: 32
                    implicitWidth: 72
                }
            }
            Button {
                text: "\u52a0\u5165\u64ad\u653e\u5217\u8868"
                enabled: list.currentIndex >= 0
                onClicked: if (player) player.addLibraryAt(list.currentIndex)
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "#FFFFFF" : "#777"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.enabled
                           ? (parent.hovered ? Qt.rgba(1,1,1,0.16) : Qt.rgba(1,1,1,0.10))
                           : Qt.rgba(1,1,1,0.04)
                    radius: 6
                    implicitHeight: 32
                    implicitWidth: 120
                }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "\u5237\u65b0"
                onClicked: if (player) player.refreshLibrary()
                contentItem: Text {
                    text: parent.text
                    color: "#B0B3B8"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1,1,1,0.12) : "transparent"
                    radius: 6
                    implicitHeight: 32
                    implicitWidth: 64
                }
            }
        }

        Text {
            visible: list.count === 0
            text: "\u6682\u65e0\u89c6\u9891\u3002\u53ef\u4e0a\u4f20\u672c\u5730\u6587\u4ef6\uff0c\u6216\u7b49\u5f85\u76ee\u5f55\u540c\u6b65\u3002"
            color: "#B0B3B8"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        GridView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: 176
            cellHeight: 156
            model: player ? player.libraryModel : null

            delegate: Item {
                width: list.cellWidth
                height: list.cellHeight

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 6
                    radius: 8
                    color: list.currentIndex === index
                           ? Qt.rgba(80/255, 134/255, 255/255, 0.22)
                           : (ma.containsMouse ? Qt.rgba(1,1,1,0.08) : Qt.rgba(1,1,1,0.04))
                    border.color: list.currentIndex === index
                                  ? Qt.rgba(80/255, 134/255, 255/255, 0.55)
                                  : Qt.rgba(1,1,1,0.06)

                    Column {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        Image {
                            width: parent.width
                            height: 90
                            fillMode: Image.PreserveAspectCrop
                            source: thumbnail_url || ""
                            asynchronous: true
                            Rectangle {
                                anchors.fill: parent
                                color: "#1E2226"
                                z: -1
                                radius: 4
                            }
                        }

                        Text {
                            width: parent.width
                            text: title
                            color: "#FFFFFF"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: list.currentIndex = index
                        onDoubleClicked: {
                            list.currentIndex = index
                            if (player)
                                player.playLibraryAt(index)
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 6
                    radius: 3
                    color: Qt.rgba(1, 1, 1, 0.25)
                }
            }
        }
    }
}
