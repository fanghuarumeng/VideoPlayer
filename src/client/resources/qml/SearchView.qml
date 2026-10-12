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

            TextField {
                id: query
                Layout.fillWidth: true
                placeholderText: "\u8f93\u5165\u5173\u952e\u8bcd\u641c\u7d22"
                color: "#FFFFFF"
                font.pixelSize: 14
                selectByMouse: true
                leftPadding: 12
                rightPadding: 12
                background: Rectangle {
                    color: Qt.rgba(1, 1, 1, 0.08)
                    border.color: query.activeFocus
                                  ? Qt.rgba(80/255, 134/255, 255/255, 0.7)
                                  : Qt.rgba(1, 1, 1, 0.12)
                    radius: 8
                    implicitHeight: 36
                }
                Keys.onReturnPressed: doSearch()
                Keys.onEnterPressed: doSearch()
            }

            Button {
                text: "\u641c\u7d22"
                onClicked: doSearch()
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
                    implicitHeight: 36
                    implicitWidth: 72
                }
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: player ? player.searchModel : null

            delegate: Item {
                width: list.width
                height: 72

                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: list.currentIndex === index
                           ? Qt.rgba(80/255, 134/255, 255/255, 0.20)
                           : (ma.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 12

                    Image {
                        id: thumb
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 54
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
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
                        elide: Text.ElideRight
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
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
                            player.playSearchAt(index)
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

        Button {
            text: "\u52a0\u5165\u64ad\u653e\u5217\u8868"
            enabled: list.currentIndex >= 0
            Layout.alignment: Qt.AlignLeft
            onClicked: if (player) player.addSearchAt(list.currentIndex)
            contentItem: Text {
                text: parent.text
                color: parent.enabled ? "#FFFFFF" : "#777"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.enabled
                       ? (parent.hovered ? Qt.rgba(1,1,1,0.18) : Qt.rgba(1,1,1,0.12))
                       : Qt.rgba(1,1,1,0.04)
                implicitHeight: 28
                implicitWidth: 120
            }
        }
    }

    function doSearch() {
        if (player)
            player.searchVideos(query.text)
    }
}
