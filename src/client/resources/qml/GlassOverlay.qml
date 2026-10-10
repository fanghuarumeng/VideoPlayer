import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Semi-transparent glass panel sitting on the player blur backdrop.
Item {
    id: root
    property string titleText: ""
    default property alias content: body.data
    signal closeRequested()

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.05, 0.06, 0.07, 0.55)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.closeRequested()
    }

    Rectangle {
        id: panel
        anchors.fill: parent
        anchors.margins: 28
        radius: 12
        color: Qt.rgba(0.10, 0.11, 0.13, 0.78)
        border.color: Qt.rgba(1, 1, 1, 0.10)
        clip: true

        MouseArea {
            anchors.fill: parent
            // Absorb clicks so dimmer behind doesn't close when clicking panel.
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Button {
                    text: "\u2190 \u8fd4\u56de\u64ad\u653e"
                    onClicked: root.closeRequested()
                    contentItem: Text {
                        text: parent.text
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Qt.rgba(1,1,1,0.14) : Qt.rgba(1,1,1,0.08)
                        radius: 6
                        implicitHeight: 32
                        implicitWidth: 110
                    }
                }

                Text {
                    text: root.titleText
                    color: "#FFFFFF"
                    font.pixelSize: 22
                    font.bold: true
                    font.family: "Segoe UI"
                    Layout.fillWidth: true
                }
            }

            Item {
                id: body
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
