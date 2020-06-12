import QtQuick 2.12
import QtQuick.Controls 2.12
import Theme 1.0

Item {
  signal valueChanged(var value, bool isNull)
  height: childrenRect.height

  TextField {
    id: textField
    height: textArea.height == 0 ? fontMetrics.height + 20: 0
    topPadding: 10
    bottomPadding: 10
    visible: height !== 0
    anchors.left: parent.left
    anchors.right: parent.right
    font: Theme.defaultFont
    color: value === undefined || !enabled ? 'gray' : 'black'

    text: value !== undefined ? value : ''

    validator: {
      if (field.isNumeric)
          if ( platformUtilities.fieldType( field ) === 'double')
          {
            doubleValidator;
          }
          else
          {
            intValidator;
          }
      else {
        null;
      }
    }

    IntValidator {
      id: intValidator
    }

    DoubleValidator {
      id: doubleValidator
    }

    inputMethodHints: field.isNumeric ? Qt.ImhFormattedNumbersOnly : Qt.ImhNone

    background: Rectangle {
      y: textField.height - height - textField.bottomPadding / 2
      implicitWidth: 120
      height: textField.activeFocus ? 2: 1
      color: textField.activeFocus ? "#4CAF50" : "#C8E6C9"
    }

    onTextChanged: {
      valueChanged( text, text == '' )
    }
  }

  TextArea {
    id: textArea
    height: config['IsMultiline'] === true ? undefined : 0
    visible: height !== 0
    anchors.left: parent.left
    anchors.right: parent.right
    wrapMode: Text.Wrap
    font: Theme.defaultFont

    text: value !== undefined ? value : ''
    textFormat: config['UseHtml'] ? TextEdit.RichText : TextEdit.PlainText

    onEditingFinished: {
      valueChanged( text, text == '' )
    }
  }

  FontMetrics {
    id: fontMetrics
    font: textField.font
  }
}
