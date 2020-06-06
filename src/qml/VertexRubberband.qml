import QtQuick 2.12
import QtQml 2.3

import org.qgis 1.0
import org.qfield 1.0

Repeater {
    id: vertexRubberband
    property MapSettings mapSettings

    delegate: Rectangle {
      MapToScreen {
          id: mapToScreen
          mapSettings: vertexRubberband.mapSettings
          mapPoint: Point
      }

      x: mapToScreen.screenPoint.x - width/2
      y: mapToScreen.screenPoint.y - width/2

      width: 20
      height: 20
      radius: PointType === VertexModel.ExistingVertex ? width / 2 : 0

      border.color: if (CurrentVertex) {'red'} else {'blue'}
      border.width: 2
    }
}

