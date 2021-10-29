import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import org.qfield 1.0
import Theme 1.0

Dialog {
  parent: mainWindow.contentItem

  property int selectedCount: 0
  property bool isDeleted: false
  property alias packagedLayersListViewModel: packagedLayersListView.model

  visible: false
  modal: true
  width: mainWindow.width - 20

  x: ( mainWindow.width - width ) / 2
  y: ( mainWindow.height - height ) / 2

  title: qsTr("Problematic QFieldCloud layer packaging")

  ColumnLayout {
    id: layout

    anchors.fill: parent

    Label {
      Layout.fillWidth: true

      text: qsTr( "Some layers have not been packaged correctly on QFieldCloud. These layers might be misconfigured or their data source is not accessible from the QFieldCloud server." )
      wrapMode: Text.WordWrap
    }

    ListView {
      id: packagedLayersListView
      model: []

      Layout.fillWidth: true
      Layout.topMargin: 10
      Layout.preferredHeight: Math.min(childrenRect.height, 300)

      delegate: Text {
        width: parent.width
        text: modelData
        font: Theme.resultFont
        wrapMode: Text.WordWrap
      }
    }
  }

  standardButtons: Dialog.Ok
}

