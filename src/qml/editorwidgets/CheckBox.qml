import QtQuick 2.12
import QtQuick.Controls 2.12

import Theme 1.0

import "."

EditorWidgetBase {
  height: childrenRect.height

  anchors {
    right: parent.right
    left: parent.left
  }

  Label {
      id: checkValue
      height: fontMetrics.height + 20
      anchors {
          left: parent.left
          right: checkBox.left
      }

      topPadding: 10
      bottomPadding: 10
      font: Theme.defaultFont
      color: isEnabled ? 'black' : 'gray'

      text: config['TextDisplayMethod'] === 1
            ? checkBox.checked ? config['CheckedState'] : config['UncheckedState']
            : checkBox.checked ? qsTr('True') : qsTr('False')

      MouseArea {
          id: checkArea
          enabled: isEnabled
          anchors.fill: parent

          onClicked: {
              checkBox.checked = !checkBox.checked
              checkBox.forceActiveFocus();
          }
      }
  }

  QfSwitch {
    id: checkBox
    enabled: isEnabled
    visible: isEnabled
    width: implicitContentWidth
    small: true

    anchors {
      right: parent.right
      verticalCenter: checkValue.verticalCenter
    }

    //if the field type is boolean, ignore the configured 'CheckedState' and 'UncheckedState' values and work with true/false always
    readonly property bool isBool: field.type == 1 //needs type coercion

    checked: {
        if( isBool ) {
            return value !== undefined ? value : false;
        } else {
            return String(value) === config['CheckedState']
        }
    }

    onCheckedChanged: {
        valueChanged( isBool ? checked : checked ? config['CheckedState'] : config['UncheckedState'], false )
        forceActiveFocus()
    }
  }

  Rectangle {
      id: backgroundRect
      anchors.left: parent.left
      anchors.right: parent.right
      y: checkValue.height - height - checkValue.bottomPadding / 2
      implicitWidth: 120
      height: checkBox.activeFocus || checkBox.pressed || checkArea.containsPress ? 2: 1
      color: checkBox.activeFocus || checkBox.pressed || checkArea.containsPress ? "#4CAF50" : "#C8E6C9"
  }

  FontMetrics {
    id: fontMetrics
    font: checkValue.font
  }
}
