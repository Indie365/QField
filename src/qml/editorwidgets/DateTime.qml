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
  If not, it will try to match the display_format with the best possible InputMask.
  A Date/Time object (Date in QML) is used even with text field as source (not DateTime)
  to allow a full flexibility of field and display formats.

 */

Item {
  signal valueChanged(var value, bool isNull)

  height: childrenRect.height

  ColumnLayout {
    id: main
    property bool isDateTimeType: field.isDateOrTime
    property bool fieldIsDate: platformUtilities.fieldType( field ) === 'QDate'
    property var currentValue: isDateTimeType ? value : Qt.formatDateTime(value, config['field_format'])

    anchors { right: parent.right; left: parent.left }

    Item {
      Layout.fillWidth: true
      Layout.minimumHeight: 48 * dp

      Rectangle {
        id: backgroundRect
        anchors.fill: parent
        border.color: label.activeFocus ? "#17a81a" : "#21be2b"
        border.width: label.activeFocus ? 2 : 1
        color: "transparent"
        radius: 2
        visible: enabled
      }

      TextField {
        id: label

        anchors.left: parent.left
        anchors.right: parent.right
        verticalAlignment: Text.AlignVCenter
        font.pointSize: 14
        height: label.font.height + 20 * dp

        inputMethodHints: Qt.ImhDigitsOnly

        // TODO[DR] generate input mask using regex
        inputMask:      if (config['display_format'] === "yyyy-MM-dd" ) { "9999-99-99;_" }
                   else if (config['display_format'] === "yyyy.MM.dd" ) { "9999.99.99;_" }
                   else if (config['display_format'] === "yyyy-MM-dd HH:mm:ss" ) { "9999-99-99 99:99:99;_" }
                   else if (config['display_format'] === "HH:mm:ss" ) { "99:99:99;_" }
                   else if (config['display_format'] === "HH:mm" ) { "99:99;_" }
                   else { "" }

        text: if ( value === undefined )
              {
                qsTr('(no date)')
              }
              else
              {
                if ( main.isDateTimeType )
                {
                  // if the field is a QDate, the automatic conversion to JS date [1]
                  // leads to the creation of date time object with the time zone.
                  // For instance shapefiles has support for dates but not date/time or time.
                  // So a date coming from a shapefile as 2001-01-01 will become 2000-12-31 19:00:00 -05 in QML/JS (in the carribeans).
                  // And when formatting this with the display format, this is shown as 2000-12-31.
                  // So we detect if the field is a date only and revert the time zone offset.
                  // [1] http://doc.qt.io/qt-5/qtqml-cppintegration-data.html#basic-qt-data-types
                  if (main.fieldIsDate) {
                    Qt.formatDateTime( new Date(value.getTime() + value.getTimezoneOffset() * 60000), config['display_format'])
                  } else {
                    Qt.formatDateTime(value, config['display_format'])
                  }
                }
                else
                {
                  var date = Date.fromLocaleString(Qt.locale(), value, config['field_format'])
                  Qt.formatDateTime(date, config['display_format'])
                }
              }

        color: value === undefined || !enabled ? 'gray' : 'black'

        background: Rectangle {
          y: label.height - height - label.bottomPadding / 2
          implicitWidth: 120 * dp
          height: label.activeFocus ? 2 * dp : 1 * dp
          color: label.activeFocus ? "#4CAF50" : "#C8E6C9"
        }

        MouseArea {
          enabled: config['calendar_popup']
          anchors.fill: parent
          onClicked: {
            popup.open()
          }
        }

        onTextEdited: {
          var newDate = Date.fromLocaleString(Qt.locale(), label.text, config['display_format'])
          if ( newDate.toLocaleString() !== "" )
          {
            if ( !main.isDateTimeType )
            {
              newDate = Qt.formatDateTime(newDate, config['field_format'])
            }
            valueChanged(newDate, newDate === undefined)
          }
          else
          {
            valueChanged(undefined, true)
          }
        }

        onActiveFocusChanged: {
          if (activeFocus) {
            // getting focus => placing cursor at proper position
            // TODO: make it work on empty value
            var mytext = label.text
            var cur = label.cursorPosition
            while ( cur > 0 )
            {
              if (!mytext.charAt(cur-1).match("[0-9]") )
                break
              cur--
            }
            label.cursorPosition = cur
          } else {
            // leaving field => if invalid, clear
            var newDate = Date.fromLocaleString(Qt.locale(), label.text, config['display_format'])
            if ( newDate.toLocaleString() === "" )
            {
              label.text = qsTr('(no date)')
            }
          }
        }

        Image {
          id: clearButton
          source: Style.getThemeIcon("ic_clear_black_18dp")
          anchors.right: parent.right
          anchors.verticalCenter: parent.verticalCenter
          anchors.verticalCenterOffset: -5 * dp
          visible: ( value !== undefined ) && config['allow_null'] && enabled

          MouseArea {
            anchors.fill: parent
            onClicked: {
              valueChanged(undefined, true)
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
      x: (parent.width - width) / 2
      y: (parent.height - height) / 2

      // TODO: fixme no signal when date is clicked on current
      ColumnLayout {
        Controls.Calendar {
          id: calendar
          weekNumbersVisible: true
          focus: false

          function resetDate() {
            selectedDate = main.currentValue ? main.isDateTimeType ? main.currentValue : Date.fromLocaleString(Qt.locale(), main.currentValue, config['field_format']) : new Date()
          }
        }

        RowLayout {
          Button {
            text: qsTr( "OK" )
            font.pointSize: 14
            Layout.fillWidth: true

            onClicked: {
              // weird, selectedDate seems to be set at time 12:00:00
              var newDate = calendar.selectedDate

              if ( main.isDateTimeType )
              {
                valueChanged(newDate, newDate === undefined)
              }
              else
              {
                var textDate = Qt.formatDateTime(newDate, config['field_format'])
                valueChanged(textDate, textDate === undefined)
              }

              popup.close()
            }
          }
        }
      }
    }

    onIsDateTimeTypeChanged: {
      calendar.resetDate()
    }

    onCurrentValueChanged: {
      calendar.resetDate()
    }
  }
}
