import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: root
    property var player

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        Text {
            text: "接下来"
            color: "#FFFFFF"
            font.pixelSize: 28
            font.family: "Segoe UI"
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: player ? player.playlistModel : null

            delegate: Item {
                width: list.width
                height: 72

                Rectangle {
                    anchors.fill: parent
                    color: list.currentIndex === index
                           ? Qt.rgba(80/255, 134/255, 255/255, 0.20)
                           : (ma.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 10

                    Image {
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 54
                        fillMode: Image.PreserveAspectCrop
                        source: thumbnail_url || ""
                        sourceSize.width: 96
                        sourceSize.height: 54

                        Rectangle {
                            anchors.fill: parent
                            color: "#1E2226"
                            z: -1
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: title
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.family: "Segoe UI"
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
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
                            player.playPlaylistIndex(index)
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.05)
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

        Button {
            Layout.fillWidth: true
            text: "MORE"
            // Load more: open library to add videos (do not clear the list).
            onClicked: if (player) player.openLibrary()
            contentItem: Text {
                text: parent.text
                color: "#FFFFFF"
                font.pixelSize: 12
                font.bold: true
                font.letterSpacing: 1
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.down ? Qt.rgba(1,1,1,0.15)
                                   : (parent.hovered ? Qt.rgba(1,1,1,0.30)
                                                     : Qt.rgba(1,1,1,0.20))
                implicitHeight: 28
            }
        }
    }
}
