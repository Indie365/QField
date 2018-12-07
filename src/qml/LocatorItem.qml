import QtQuick 2.11
import QtQuick.Controls 2.4 as Controls
import "js/style.js" as Style



Column {
  id: locatorItem
  property bool searching: false

      states: [
          State { when: locatorItem.searching;
                  PropertyChanges { target: searchButton; opacity: 0.0 }
                  PropertyChanges { target: searchField; opacity: 1.0 }},
          State { when: !locatorItem.searching;
                  PropertyChanges { target: searchButton; opacity: 1.0 }
                  PropertyChanges { target: searchField; opacity: 0.0 }}
      ]

  Row {
    id: topRow
    width: parent.width

    Controls.TextField {
      id: searchField
      placeholderText: qsTr("Search…")
      onTextChanged: locator.performSearch(searchField.text)
      width: parent.width
      anchors.right: parent.right

      visible: opacity > 0

      transitions: Transition {
        SequentialAnimation {
          NumberAnimation {
            target: searchField
            property: "opacity"
            duration: 400
          }
        }
      }
    }

    Item {
      id: searchButton
      anchors.right: parent.right

      Button {
        anchors { right: parent.right; top: parent.top; rightMargin: 4*dp; topMargin: 4*dp }

        iconSource: Style.getThemeIcon( "ic_baseline_search_white" )
        round: true
        bgcolor: "#80CC28"

        onClicked: locatorItem.searching = true
      }

      visible: opacity > 0
      transitions: Transition {
        SequentialAnimation {
          NumberAnimation {
            target: searchButton
            property: "opacity"
            duration: 400
          }
        }
      }
    }
  }

  Repeater {
    id: repeater
    model: locator.proxyModel()
    width: parent.width

    delegate:
      Text {
        text: model.Text + model.ResultFilterGroupSorting
        property bool isGroup: model.ResultFilterGroupSorting === 0
        visible: model.ResultType !== 0 // remove filter name
        height: visible ? (isGroup ? 20 : 25 ) * dp : 0
        color: isGroup ? "red" : "blue"
        font.italic: isGroup ? true : false
        // other drawing code here.
      }
  }
}

