import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: root
    property var player

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: player ? player.historyModel : null

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

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text {
                            Layout.fillWidth: true
                            text: title
                            color: "#FFFFFF"
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }
                        Text {
                            text: progress_text
                            color: "#B0B3B8"
                            font.pixelSize: 12
                        }
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
                            player.playHistoryAt(index)
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

        Text {
            visible: list.count === 0
            text: "\u6682\u65e0\u64ad\u653e\u5386\u53f2"
            color: "#B0B3B8"
            font.pixelSize: 13
        }

        Button {
            text: "\u6e05\u7a7a\u5386\u53f2"
            Layout.alignment: Qt.AlignLeft
            onClicked: if (player) player.clearHistory()
            contentItem: Text {
                text: parent.text
                color: "#FFFFFF"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.hovered ? Qt.rgba(1,1,1,0.18) : Qt.rgba(1,1,1,0.12)
                implicitHeight: 28
                implicitWidth: 96
            }
        }
    }
}
