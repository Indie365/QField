import QtQuick 2.0

import QtQuick.Controls 1.4 as Controls
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import org.qgis 1.0
import org.qfield 1.0
import QtQml.Models 2.2
import QtQuick.Controls.Styles 1.4

import "js/style.js" as Style

TreeView {
  id: listView
  model: layerTree

  style: TreeViewStyle{
    indentation: 24 * dp
    branchDelegate: Item {
        width: 24 * dp
        height: 24 * dp
        Rectangle{
            anchors.fill: parent
            color: styleData.row !== undefined && layerTree.data(listView.__model.mapRowToModelIndex(styleData.row), LayerTreeModel.VectorLayer) === currentLayer && currentLayer != null ? "#999" : "#fff"
            Image {
              anchors.fill: parent
              source:  styleData.isExpanded ? Style.getThemeIcon("ic_arrow_drop_down_black_24dp") : Style.getThemeIcon("ic_arrow_right_black_24dp")
            }
        }
    }
  }

  headerVisible: false

  QtObject {
    id: properties

    property var previousIndex
  }

  property VectorLayer currentLayer

  Controls.TableViewColumn {
    role: "display"
  }

  rowDelegate: Rectangle {
    height: layerTree.data(listView.__model.mapRowToModelIndex(styleData.row), LayerTreeModel.Type) === 'legend' ? 36 * dp : 48 * dp
    color: styleData.row !== undefined && layerTree.data(listView.__model.mapRowToModelIndex(styleData.row), LayerTreeModel.VectorLayer) === currentLayer && currentLayer != null ? "#999" : "#fff"
    //small hack: since the image of a root item should be aligned to the expand triangles of branches, it needs to be printed here
    Image {
      visible: styleData.row !== undefined && layerTree.data(listView.__model.mapRowToModelIndex(styleData.row), LayerTreeModel.Type) === 'layer'
      source: "image://legend/" + layerTree.data(listView.__model.mapRowToModelIndex(styleData.row), LayerTreeModel.LegendImage)
      width: delegatedItem.height
      height: delegatedItem.height
      x: ( 24 * dp - width )/2
      anchors.verticalCenter: parent.verticalCenter
    }
  }

  itemDelegate: Item {
    id: delegatedItem
    height: Math.max(16, label.implicitHeight)
    property int implicitWidth: label.implicitWidth + 20

    RowLayout {
      height: layerTree.data(listView.__model.mapRowToModelIndex(styleData.row), LayerTreeModel.Type) === 'legend' ? 36 * dp : 48 * dp
      Image {
        visible: layerTree.data(listView.__model.mapRowToModelIndex(styleData.row), LayerTreeModel.Type) === 'legend'
        source: "image://legend/" + layerTree.data(styleData.index, LayerTreeModel.LegendImage)
        width: 24 * dp
        height: 24 * dp
        Layout.alignment: Qt.AlignVCenter
      }
      Text {
        id: label
        horizontalAlignment: styleData.textAlignment
        Layout.alignment: Qt.AlignVCenter
        elide: styleData.elideMode
        text: styleData.value !== undefined ? styleData.value : ""
        renderType: Settings.isMobile ? Text.QtRendering : Text.NativeRendering
        color: layerTree.data(styleData.index, LayerTreeModel.Visible) ? "black" : "gray"
      }
    }
  }

  /**
   * User clicked an item
   *
   * @param index : QModelIndex
   */
  onClicked: {
    var nodeType = layerTree.data(index, LayerTreeModel.Type)
    if (nodeType !== 'layer') {
      if (listView.isExpanded(index))
          listView.collapse(index)
      else
          listView.expand(index)
    }
    else
      currentLayer = layerTree.data(index, LayerTreeModel.VectorLayer)
  }

  onPressAndHold: {
    itemProperties.index = index
    itemProperties.open()
  }

  onDoubleClicked: {
    if (listView.isExpanded(index))
        listView.collapse(index)
    else
        listView.expand(index)
  }

  LayerTreeItemProperties {
      id: itemProperties
      layerTree: listView.model

      modal: true
      closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
      parent: ApplicationWindow.overlay
  }
}
