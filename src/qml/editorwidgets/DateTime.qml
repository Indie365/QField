import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls 1.4 as Controls
import QtQuick.Layouts 1.1
import "../js/style.js" as Style

/*
  Config:
  * field_format
  * display_format
  * calendar_popup
  * allow_null

  If the calendar_popup is enabled, no direct editing is possible in the TextField.
  If not, it will try to match the display_format with the best possible InputMethodHints.
  If the field_format matches a time, it will not construct a Date object for the display.
  A validator is also set for common types.

 */


Item {
  signal valueChanged(var value, bool isNull)

  height: childrenRect.height
  anchors { right: parent.right; left: parent.left }

  ColumnLayout {
    id: main
    property var currentValue: value

    anchors { right: parent.right; left: parent.left }

    Item {
      anchors { right: parent.right; left: parent.left }
      Layout.minimumHeight: 48 * dp

      Rectangle {
        anchors.fill: parent
        id: backgroundRect
        border.color: comboBox.pressed ? "#17a81a" : "#21be2b"
        border.width: comboBox.visualFocus ? 2 : 1
        color: "#dddddd"
        radius: 2
      }

      TextField {
        id: label

        anchors.fill: parent
        verticalAlignment: Text.AlignVCenter
        font.pointSize: 16

        Component.onCompleted: {
          if (config['display_format'] === QgsDateFormat)
          {
            label.inputMethodHints = Qt.ImhDate
            label.inputMask = "0000-09-09;_"
            label.validator = new RegExpValidator( /^[0-9]{4}:([0-5][0-9]):[0-5][0-9]$ /)
          }
          else if (config['display_format'] === QgsTimeFormat)
          {
            label.inputMethodHints = Qt.ImhTime
            label.inputMask = "09:09:00; "
            label.validator = new RegExpValidator( /^([0-1]?[0-9]|2[0-3]):([0-5][0-9]):[0-5][0-9]$ / )
          }
          else
          {
            label.inputMethodHints = Qt.ImhDigitsOnly
          }
        }

        text: value === undefined ? qsTr('(no date)') :
                                    config['field_format'] === QgsTimeFormat ?
                                      value :
                                      new Date(value).toLocaleString(Qt.locale(), config['display_format'] )
        color: value === undefined ? 'gray' : 'black'


        MouseArea {
          enabled: config['calendar_popup']
          anchors.fill: parent
          onClicked: {
            popup.open()
          }
        }

        Image {
          source: Style.getThemeIcon("ic_clear_black_18dp")
          anchors.right: parent.right
          anchors.verticalCenter: parent.verticalCenter
          visible: ( value !== undefined ) && config['allow_null']

          MouseArea {
            anchors.fill: parent
            onClicked: {
              main.currentValue = undefined
            }
          }
        }
      }
    }

    Popup {
      id: popup
      modal: true
      focus: true
      closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
      parent: ApplicationWindow.overlay

      ColumnLayout {
        Controls.Calendar {
          id: calendar
          selectedDate: currentValue
          weekNumbersVisible: true
          focus: false

          onSelectedDateChanged: {
            main.currentValue = selectedDate
          }
        }

        RowLayout {
          Button {
            text: qsTr( "Ok" )
            Layout.fillWidth: true

            onClicked: popup.close()
          }
        }
      }
    }

    onCurrentValueChanged: {
      valueChanged(currentValue, main.currentValue === undefined)
    }
  }
}
