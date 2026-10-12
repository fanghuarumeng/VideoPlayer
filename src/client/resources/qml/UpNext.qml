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
                        id: thumb
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 54
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        sourceSize.width: 96
                        sourceSize.height: 54
                        // ListView recycles delegates: clear then reload so an
                        // empty/slow URL never keeps the previous row's pixels.
                        property var itemId: id
                        property string thumbUrl: thumbnail_url || ""

                        function reload() {
                            var u = thumbUrl.length > 0 ? thumbUrl
                                                        : "qrc:/app/icons/film.svg"
                            source = ""
                            source = u
                        }
                        Component.onCompleted: reload()
                        onItemIdChanged: reload()
                        onThumbUrlChanged: reload()

                        Rectangle {
                            anchors.fill: parent
                            color: "#1E2226"
                            z: -1
                            visible: thumb.status !== Image.Ready
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                Layout.fillWidth: true
                text: "\u66f4\u591a"
                // Load more: open library to add videos (do not clear the list).
                onClicked: if (player) player.openLibrary()
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: 12
                    font.bold: true
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

            Button {
                Layout.fillWidth: true
                text: "\u6e05\u7a7a"
                onClicked: if (player) player.clearPlaylist()
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: 12
                    font.bold: true
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
}
