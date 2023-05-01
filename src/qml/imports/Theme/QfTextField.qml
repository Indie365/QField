import QtQuick 2.14
import QtQuick.Controls 2.14

Item {
  id: textFieldWrapper

  property alias font: textField.font
  property alias text: textField.text
  property alias placeholderText: textField.placeholderText
  property alias horizontalAlignment: textField.horizontalAlignment
  property alias inputMethodHints: textField.inputMethodHints
  property alias inputMask: textField.inputMask

  property var echoMode: TextInput.Normal

  signal textEdited
  signal editingFinished
  signal returnPressed

  width: textField.width
  height: textField.height

  FontMetrics {
    id: fontMetrics
    font: Theme.defaultFont
  }

  TextField {
    id: textField
    echoMode: textFieldWrapper.echoMode
    width: textFieldWrapper.width
    font: Theme.defaultFont
    placeholderTextColor: Theme.accentLightColor
    rightPadding: showPasswordButton.visible ? 2 * showPasswordButton.width : 0
    leftPadding: rightPadding
    inputMethodHints: Qt.ImhNone

    background: Rectangle {
      y: textField.height - height - textField.bottomPadding / 2
      implicitWidth: 120
      height: textField.activeFocus ? 2: 1
      color: textField.activeFocus ? Theme.accentColor : Theme.accentLightColor
    }

    onActiveFocusChanged: {
      if ( !activeFocus ) {
        echoMode = textFieldWrapper.echoMode
      }
    }

    onTextEdited: {
        textFieldWrapper.textEdited()
    }

    onEditingFinished: {
        textFieldWrapper.editingFinished();
    }

    onFocusChanged: {
        if (focus) {
            Qt.inputMethod.show()
        }
    }

    Keys.onReturnPressed: {
        textFieldWrapper.returnPressed()
    }
  }

  QfToolButton {
    id: showPasswordButton
    z: 1
    visible: !!textFieldWrapper.echoMode && textFieldWrapper.echoMode !== TextInput.Normal
    iconSource: textField.echoMode === TextInput.Normal
                ? Theme.getThemeVectorIcon('ic_hide_green_48dp')
                : Theme.getThemeVectorIcon('ic_show_green_48dp')
    anchors.right: textField.right
    anchors.verticalCenter: textField.verticalCenter
    opacity: textField.text.length > 0 ? 1 : 0.25

    onClicked: {
      textField.echoMode = textField.echoMode === TextInput.Normal
                           ? textFieldWrapper.echoMode
                           : TextInput.Normal
    }
  }
}
