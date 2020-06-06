import QtQuick 2.12
import QtQuick.Controls 2.12

import org.qgis 1.0
import org.qfield 1.0
import Theme 1.0
import "."

Item {
  id: locatorItem
  property bool searching: false
  height: childrenRect.height

  onSearchingChanged: {
      searchField.focus = searching
  }

  LocatorModelSuperBridge {
    id: locator
    mapSettings: mapCanvas.mapSettings
    locatorHighlightGeometry: locatorHighlightItem.geometryWrapper
    keepScale: qfieldSettings.locatorKeepScale

    featureListController: featureForm.extentController

    onMessageEmitted: {
      displayToast(text)
    }
  }

  TextField {
    id: searchField
    height: fontMetrics.height + 20
    placeholderText: qsTr("Search…")
    onTextEdited: locator.performSearch(searchField.text)
    width: parent.width
    anchors.top: parent.top
    anchors.right: parent.right
    visible: locatorItem.searching
    padding: 5
    //inputMethodHints: Qt.ImhNoPredictiveText  // see https://forum.qt.io/topic/12147/solved-textfield-textinput-do-not-emit-textchanged-signal
    font: Theme.secondaryTitleFont
    selectByMouse: true
    verticalAlignment: TextInput.AlignBottom

    background: Rectangle {
      height: searchField.height - 5
      radius: 2
      border.color: "#333"
      border.width: 1
    }

    Keys.onReleased: {
      if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
        locatorItem.searching = false
      }
    }
  }

  FontMetrics {
    id: fontMetrics
    font: searchField.font
  }

  BusyIndicator {
    id: busyIndicator
    running: locator.isRunning
    anchors.right: searchField.right
    anchors.verticalCenter: searchField.verticalCenter
    anchors.margins: 4
    height: searchField.height - 10
  }

  Button {
    id: searchButton
    anchors { right: parent.right; top: parent.top; }
    visible: !locatorItem.searching

    iconSource: Theme.getThemeIcon( "ic_baseline_search_white" )
    round: true
    bgcolor: Theme.mainColor

    onClicked: {
      locatorItem.searching = true
    }
  }

  Rectangle {
    id: resultsBox
    width: parent.width
    height: childrenRect.height+2*border.width
    border.color: "darkslategray"
    border.width: resultsList.count ? 1: 0
    radius: 2
    anchors.top: searchField.bottom
    color: "white"
    visible: locatorItem.searching

  ListView {
    id: resultsList
    anchors.centerIn: parent
    model: locator.proxyModel()
    width: parent.width-2*resultsBox.border.width
    height: Math.min( childrenRect.height, 200, mainWindow.height - searchField.height )
    clip: true

    delegate: Rectangle {
      id: delegateRect
      anchors.margins: 10
      height: if (visible) { if(isGroup){ 25} else { Math.max(childrenRect.height+8, 40) }} else { 0 }
      width: parent.width
      visible: model.ResultType !== 0 // remove filter name
      property bool isGroup: model.ResultFilterGroupSorting === 0
      property int resultIndex: index
      color: isGroup ? "#eee" : "#fff"
      opacity: 0.95
      border.width: 1
      border.color: "#bbb"

      Text {
        id: textCell
        text: model.Text
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: actionsRow.left
        leftPadding: 5
        font.italic: delegateRect.isGroup ? true : false
        font.pointSize: Theme.secondaryTitleFont.pointSize
        wrapMode: Text.Wrap
      }

      Row {
        id: actionsRow
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height
        anchors.rightMargin: 1

        Repeater {
          model: locator.contextMenuActionsModel( index )
          Button {
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height
            width:  36
            bgcolor: Theme.mainColor
            Image {
              anchors.fill: parent
              source: Theme.getThemeIcon( model.iconPath )
              fillMode: Image.Pad
            }
            MouseArea {
              anchors.fill: parent
              onClicked: {
                locator.triggerResultAtRow(delegateRect.resultIndex, model.id)
              }
            }
          }
        }
      }

      MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: actionsRow.left

        onClicked: {
          locator.triggerResultAtRow(index)
        }
      }
    }
  }
  }
}

