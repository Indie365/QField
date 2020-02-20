import QtQuick 2.0
import QtQuick.Controls 2.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import Theme 1.0
import "." as QField

Item {
  signal close()

  height: childrenRect.height
  width: parent.width

  GridLayout {
    id: mainGrid

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.margins: 20 * dp

    columns: 1

    Item {
        // top margin
        height: 20 * dp
    }

    Text {
      id: title
      text: qsTr( "What's new in the latest QField" )
      color: Theme.mainColor
      font: Theme.titleFont
      minimumPixelSize: 12


      fontSizeMode: Text.VerticalFit
      wrapMode: Text.WordWrap
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumHeight: contentHeight
      Layout.maximumHeight: contentHeight
    }

    Text {
      color: '#95000000'
      text: qsTr( "Changelog %1" ).arg( version )
      font: Theme.strongFont

      fontSizeMode: Text.VerticalFit
      wrapMode: Text.WordWrap
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumHeight: contentHeight
      Layout.maximumHeight: contentHeight
    }

    Rectangle{
      id: changelogBox

      Layout.fillWidth: true
      Layout.fillHeight: true //preferredHeight: Math.min( 3 * itemHeight, changesListView.count * itemHeight ) + 20 * dp
      Layout.minimumHeight: changesListView.count * 24 * dp
      border.color: '#30000000'
      border.width: 1 * dp

      //the model
      ListModel {
          id: changesListModel
          ListElement {
            type: "New Feature"
            description: qsTr("Value relation widget with multiple selection support")
          }
          ListElement {
            type: "New Feature"
            description: qsTr("Full snapping support providing snapping results and Z values of snapped feature")
          }
          ListElement {
            type: "New Feature"
            description: qsTr("Authentication dialog for layers")
          }
          ListElement {
            type: "Fix"
            description: qsTr("Fix checkbox widget")
          }
          ListElement {
            type: "Fix"
            description: qsTr("Other fixes (printing, locator)")
          }
          ListElement {
            type: "Fix"
            description: qsTr("Improved log")
          }
      }

      //the list
      ListView {
        id: changesListView
        model: changesListModel
        width: parent.width - 20 * dp
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height - 20 * dp //Math.min( 3 * changelogBox.itemHeight, changesListView.count * changelogBox.itemHeight )
        delegate: Rectangle{
            id: item
            x: 1 * dp
            width: parent.width - 2 * dp
            height: text.height + 10 * dp

            Text {
                id: dash
                text: " -"
                font: Theme.defaultFont
                Layout.minimumWidth: contentWidth
                fontSizeMode: Text.VerticalFit
                wrapMode: Text.WordWrap
                color: '#95000000'
            }
            Text {
                id: text
                text: description
                font: Theme.defaultFont
                Layout.minimumHeight: contentHeight
                Layout.maximumHeight: contentHeight
                width: parent.width - 20 * dp
                x: dash.width + 10 * dp
                fontSizeMode: Text.VerticalFit
                wrapMode: Text.WordWrap
                color: '#95000000'
            }
        }
        focus: true
        clip: true
        highlightRangeMode: ListView.StrictlyEnforceRange
      }
    }

    Text {
      color: '#90000000'
      text: qsTr( "Do you enjoy QField? Show some love and support the crowdfunding campaign. Before October 16." )
      font: Theme.strongFont

      fontSizeMode: Text.VerticalFit
      wrapMode: Text.WordWrap
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumHeight: contentHeight
      Layout.maximumHeight: contentHeight
    }

    Image {
      id: image
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.maximumHeight:  width / 3
      fillMode: Image.PreserveAspectFit
      source: 'qrc:/pictures/qfield-love.png'
    }


    Text {
      color: '#90000000'
      text: qsTr( "www.opengis.ch/projects/qfield-love/" )
      font: Theme.strongFont

      fontSizeMode: Text.VerticalFit
      wrapMode: Text.WordWrap
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumHeight: contentHeight
      Layout.maximumHeight: contentHeight
    }

    GridLayout{
      id: buttons
      columns: 1

      Layout.maximumHeight: 96 * dp
      Layout.preferredHeight: 96 * dp
      Layout.minimumHeight: 72 * dp
      Layout.fillWidth: true

      Button {
        id: closeButton
        Layout.fillWidth: true
        Layout.fillHeight: true

        text: qsTr( "Let's give love" )

        font: Theme.defaultFont

        contentItem: Text {
          text: closeButton.text
          font: closeButton.font
          color: 'white'
          horizontalAlignment: Text.AlignHCenter
          verticalAlignment: Text.AlignVCenter
          elide: Text.ElideRight
        }

        background: Rectangle {
          color: laterButton.down ? '#8080CC28' : Theme.mainColor
        }

        onClicked: {
            Qt.openUrlExternally("https://www.opengis.ch/projects/qfield-love/")
            settings.setValue( "/QField/CurrentVersion", versionCode )
            close()
        }
      }

      GridLayout{
          Button {
            id: laterButton
            Layout.fillWidth: true
            Layout.fillHeight: true

            text: qsTr( "Maybe later" )

            font: Theme.defaultFont

            contentItem: Text {
                text: laterButton.text
                font: laterButton.font
                color: 'white'
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                height: parent.height
                color: laterButton.down ? '#40000000' : '#60000000'
            }


            onClicked: {
              close()
            }
          }

          Button {
            id: noButton
            Layout.fillWidth: true
            Layout.fillHeight: true

            text: qsTr( "No" )

            font: Theme.defaultFont

            contentItem: Text {
                text: noButton.text
                font: noButton.font
                color: 'white'
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: laterButton.down ? '#40000000' : '#60000000'
            }

            onClicked: {
              settings.setValue( "/QField/CurrentVersion", versionCode )
              close()
            }
          }
      }
    }

    Item {
        // bottom
        height: 20 * dp
    }
  }
}
