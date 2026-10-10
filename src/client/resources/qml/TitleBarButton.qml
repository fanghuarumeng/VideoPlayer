import QtQuick 2.12
import QtQuick.Controls 2.12

ToolButton {
    id: root
    property bool danger: false
    flat: true
    focusPolicy: Qt.NoFocus
    implicitWidth: 46
    implicitHeight: 32

    contentItem: Text {
        text: root.text
        color: "#FFFFFF"
        font.pixelSize: 14
        font.family: "Segoe UI Symbol"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: root.enabled ? 1.0 : 0.4
    }

    background: Rectangle {
        color: {
            if (!root.hovered && !root.down)
                return "transparent"
            if (root.danger)
                return root.down ? "#c42b1c" : "#e81123"
            return root.down ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.10)
        }
    }
}
