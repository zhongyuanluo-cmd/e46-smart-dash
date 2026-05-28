// QML: Red rectangle filling the entire display
// Confirms Qt eglfs rendering pipeline works on Core3566

import QtQuick

Rectangle {
    width: 800
    height: 480
    color: "#CC0000"

    Text {
        anchors.centerIn: parent
        text: "E46 Smart Dash\nQt 6 eglfs OK"
        color: "white"
        font.pixelSize: 48
        horizontalAlignment: Text.AlignHCenter
    }
}
