import QtQuick 2.12
import QtQuick.Shapes 1.12
import QtQuick.Window 2.12
import QtGraphicalEffects 1.12
import QtSensors 5.12

import org.qgis 1.0
import Theme 1.0

Item {
  id: item

  property variant location // QgsPoint
  property string device // Empty string means internal device is used
  property real accuracy
  property real direction // A -1 value indicates absence of direction information (note: when an external GNSS device is connected, direction is actually a compass)
  property real speed // A -1 value indicates absence of speed information
  property MapSettings mapSettings

  QtObject {
    id: props

    property point screenLocation
    property real screenAccuracy

    property bool isOnMapCanvas: screenLocation.x > 0
                              && screenLocation.x < mapCanvas.width
                              && screenLocation.y > 0
                              && screenLocation.y < mapCanvas.height
  }
  function updateScreenLocation() {
    props.screenLocation = mapSettings.coordinateToScreen( location )
    props.screenAccuracy = accuracy / mapSettings.mapUnitsPerPoint
  }

  Magnetometer {
    id: magnetometer
    active: false
    returnGeoValues: false

    Screen.orientationUpdateMask: Qt.PortraitOrientation | Qt.InvertedPortraitOrientation | Qt.LandscapeOrientation | Qt.InvertedLandscapeOrientation
    Screen.onOrientationChanged: {
        switch (Screen.orientation) {
          case Qt.LandscapeOrientation:
            magnetometer.userOrientation = 90;
            break;
          case Qt.InvertedLandscapeOrientation:
            magnetometer.userOrientation = 270;
            break;
          case Qt.PortraitOrientation:
          default:
            magnetometer.userOrientation = 0;
            break;
        }
    }

    property bool hasValue: false
    property real x: 0
    property real y: 0

    onReadingChanged: {
      magnetometer.x = reading.x
      magnetometer.y = reading.y
      magnetometer.hasValue = true
    }
  }

  Rectangle {
    id: accuracyMarker
    visible: props.screenAccuracy > 0.0
    width: props.screenAccuracy * 2
    height: props.screenAccuracy * 2

    x: props.screenLocation.x - width / 2
    y: props.screenLocation.y - height / 2

    radius: width/2

    color: Theme.positionColorSemiOpaque
    border.color: Theme.darkPositionColorSemiOpaque
    border.width: 0.7
  }

  Image {
    id: compassDirectionMarker
    visible: device === '' && magnetometer.hasValue
    width: 48
    height: 48
    opacity: 0.6

    x: props.screenLocation.x - width / 2
    y: props.screenLocation.y - height

    source: Theme.getThemeVectorIcon( "ic_compass_direction" )
    fillMode: Image.PreserveAspectFit
    rotation: magnetometer.userOrientation + (-(Math.atan2(magnetometer.x, magnetometer.y) / Math.PI) * 180)
    transformOrigin: Item.Bottom
    smooth: true
  }

  Shape {
    id: movementMarker
    visible: speed > 0 && props.isOnMapCanvas
    width: 26
    height: 26

    x: props.screenLocation.x - width / 2
    y: props.screenLocation.y - height / 2

    rotation: direction
    transformOrigin: Item.Center

    ShapePath {
      strokeWidth: 3
      strokeColor: "white"
      strokeStyle: ShapePath.SolidLine
      fillColor: Theme.positionColor
      joinStyle: ShapePath.MiterJoin
      startX: 10
      startY: 2
      PathLine { x: 18; y: 22 }
      PathLine { x: 10; y: 16 }
      PathLine { x: 2; y: 22 }
      PathLine { x: 10; y: 2 }

      SequentialAnimation on fillColor  {
        loops: Animation.Infinite
        ColorAnimation  { from: Theme.positionColor; to: Theme.darkPositionColor; duration: 2000; easing.type: Easing.InOutQuad }
        ColorAnimation  { from: Theme.darkPositionColor; to: Theme.positionColor; duration: 1000; easing.type: Easing.InOutQuad }
      }
    }

    layer.enabled: true
    layer.samples: 4
    layer.effect: DropShadow {
        transparentBorder: true
        radius: 8
        samples: 25
        color: "#99000000"
        horizontalOffset: 0
        verticalOffset: 0
    }
  }

  Rectangle {
    id: positionMarker
    visible: !movementMarker.visible && props.isOnMapCanvas

    width: 12
    height: 12

    x: props.screenLocation.x - width / 2
    y: props.screenLocation.y - height / 2

    radius: width / 2

    color: Theme.positionColor
    border.color: "white"
    border.width: 3

    SequentialAnimation on color  {
      loops: Animation.Infinite
      ColorAnimation  { from: Theme.positionColor; to: Theme.darkPositionColor; duration: 2000; easing.type: Easing.InOutQuad }
      ColorAnimation  { from: Theme.darkPositionColor; to: Theme.positionColor; duration: 1000; easing.type: Easing.InOutQuad }
    }

    layer.enabled: true
    layer.samples: 4
    layer.effect: DropShadow {
        transparentBorder: true
        radius: 8
        samples: 25
        color: "#99000000"
        horizontalOffset: 0
        verticalOffset: 0
    }
  }

  Shape {
    id: edgeMarker
    visible: !props.isOnMapCanvas
    width: 20
    height: 24

    x: Math.min(mapCanvas.width - width, Math.max(0, props.screenLocation.x - width / 2))
    y: Math.min(mapCanvas.height - width, Math.max(0, props.screenLocation.y - width / 2))

    transform: Rotation {
        origin.x: edgeMarker.width / 2;
        origin.y: edgeMarker.width / 2;
        angle:-(Math.atan2(mapCanvas.width / 2 - props.screenLocation.x, mapCanvas.height / 2 - props.screenLocation.y) / Math.PI) * 180
    }

    ShapePath {
      strokeWidth: 3
      strokeColor: "white"
      strokeStyle: ShapePath.SolidLine
      fillColor: Theme.positionColor
      joinStyle: ShapePath.MiterJoin
      startX: 10
      startY: 0
      PathLine { x: 18; y: 20 }
      PathLine { x: 2; y: 20 }
      PathLine { x: 10; y: 0 }

      SequentialAnimation on fillColor  {
        loops: Animation.Infinite
        ColorAnimation  { from: Theme.positionColor; to: Theme.darkPositionColor; duration: 2000; easing.type: Easing.InOutQuad }
        ColorAnimation  { from: Theme.darkPositionColor; to: Theme.positionColor; duration: 1000; easing.type: Easing.InOutQuad }
      }
    }

    layer.enabled: true
    layer.samples: 4
    layer.effect: DropShadow {
        transparentBorder: true
        radius: 8
        samples: 25
        color: "#99000000"
        horizontalOffset: 0
        verticalOffset: 0
    }
  }

  Connections {
    target: mapSettings


    function onExtentChanged() {
      updateScreenLocation()
    }
    function onOutputSizeChanged() {
      updateScreenLocation()
    }
  }

  onLocationChanged: {
   updateScreenLocation()
  }

  Connections {
    target: positionSource

    function onActiveChanged() {
      if (positionSource.active) {
        magnetometer.active = true
        magnetometer.start()
      } else {
        magnetometer.stop()
        magnetometer.active = false
        magnetometer.hasValue = false
      }
    }
  }

  Component.onCompleted: {
    if (positionSource.active) {
      magnetometer.start()
    }
  }
}
