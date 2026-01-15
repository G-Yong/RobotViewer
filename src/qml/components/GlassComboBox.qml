import QtQuick 2.15
import QtQuick.Controls 2.15
import "."

// 游戏风格下拉选择框
Item {
    id: root
    
    property var model: []
    property string currentValue: ""
    property string placeholder: qsTr("请选择")
    property int popupWidth: 250
    property int popupMaxHeight: 200
    
    signal valueChanged(string value)
    
    implicitHeight: 28
    implicitWidth: 150
    
    Rectangle {
        id: background
        anchors.fill: parent
        radius: 6
        color: "#15ffffff"
        border.color: comboBox.popup.visible ? "#00ff88" : "#30ffffff"
        border.width: 1
        
        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
    }
    
    ComboBox {
        id: comboBox
        anchors.fill: parent
        model: root.model
        currentIndex: root.model ? root.model.indexOf(root.currentValue) : -1
        
        background: Rectangle { color: "transparent" }
        
        // // 下拉箭头指示器
        // indicator: Text {
        //     x: comboBox.width - width - 10
        //     anchors.verticalCenter: parent.verticalCenter
        //     text: comboBox.popup.visible ? "▲" : "▼"
        //     color: "#60ffffff"
        //     font.pixelSize: FontConfig.tiny
        // }
        
        contentItem: Text {
            text: comboBox.displayText || root.placeholder
            color: comboBox.displayText ? "#ffffff" : "#60ffffff"
            font.pixelSize: FontConfig.small
            verticalAlignment: Text.AlignVCenter
            leftPadding: 10
            rightPadding: 25
            elide: Text.ElideRight
        }
        
        popup: Popup {
            y: comboBox.height
            width: Math.max(comboBox.width, 250)
            implicitHeight: contentItem.implicitHeight
            padding: 1

            contentItem: ListView {
                clip: true
                implicitHeight: Math.min(contentHeight, 200)
                model: comboBox.popup.visible ? comboBox.delegateModel : null
                currentIndex: comboBox.highlightedIndex

                ScrollBar.vertical: ScrollBar { }
            }

            background: Rectangle {
                color: "#1a1a2e"
                border.color: "#40ffffff"
                radius: 4
            }
        }

        delegate: ItemDelegate {
            width: comboBox.width
            contentItem: Text {
                text: modelData
                color: "#ffffff"
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: highlighted ? "#30ffffff" : "transparent"
            }
            highlighted: comboBox.highlightedIndex === index
        }

        onCurrentTextChanged: {
            if (currentText && currentText !== root.currentValue) {
                root.valueChanged(currentText)
            }
        }
    }
    
    // 当外部 currentValue 变化时更新 ComboBox
    onCurrentValueChanged: {
        if (model) {
            var idx = model.indexOf(currentValue)
            if (idx !== comboBox.currentIndex) {
                comboBox.currentIndex = idx
            }
        }
    }
    
    // 当外部 model 变化时更新 ComboBox
    onModelChanged: {
        if (model && currentValue) {
            comboBox.currentIndex = model.indexOf(currentValue)
        }
    }
}
