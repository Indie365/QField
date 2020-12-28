/***************************************************************************
                            MapCanvas.qml
                              -------------------
              begin                : 10.12.2014
              copyright            : (C) 2014 by Matthias Kuhn
              email                : matthias (at) opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQml 2.2

import org.qgis 1.0

Item {
  id: mapArea
  property alias mapSettings: mapCanvasWrapper.mapSettings
  property alias isRendering: mapCanvasWrapper.isRendering
  property alias incrementalRendering: mapCanvasWrapper.incrementalRendering

  property bool mouseAsTouchScreen: qfieldSettings.mouseAsTouchScreen
  property bool freehandDigitizing: false

  // for signals, type can be "stylus" for any device click or "touch"

  //! This signal is emitted independently of double tap / click
  signal clicked(var point, var type)

  //! This signal is only emitted if there is no double tap/click coming. It is emitted with a delay of 250ms
  signal confirmedClicked(var point)

  signal longPressed(var point, var type)

  //! Emitted when a release happens after a long press
  signal longPressReleased(var type)

  signal panned

  /**
   * Freezes the map canvas refreshes.
   *
   * In case of repeated geometry changes (animated resizes, pinch, pan...)
   * triggering refreshes all the time can cause severe performance impacts.
   *
   * If freeze is called, an internal counter is incremented and only when the
   * counter is 0, refreshes will happen.
   * It is therefore important to call freeze() and unfreeze() exactly the same
   * number of times.
   */
  function freeze(id) {
    mapCanvasWrapper.__freezecount[id] = true
    mapCanvasWrapper.freeze = true
  }

  function unfreeze(id) {
    delete mapCanvasWrapper.__freezecount[id]
    mapCanvasWrapper.freeze = Object.keys(mapCanvasWrapper.__freezecount).length !== 0
  }

  function zoomIn(point) {
    mapCanvasWrapper.zoom(point, 0.67)
  }

  function zoomOut(point) {
    mapCanvasWrapper.zoom(point, 1.5)
  }

  MapCanvasMap {
    id: mapCanvasWrapper

    width: mapArea.width
    height: mapArea.height

    property var __freezecount: ({})

    freeze: false
  }

    // stylus clicks
    TapHandler {
      enabled: !mouseAsTouchScreen
      acceptedDevices: PointerDevice.AllDevices & ~PointerDevice.TouchScreen
      property bool longPressActive: false

      onSingleTapped: {
        mapArea.clicked(point.position, "stylus")
      }

      onLongPressed: {
          mapArea.longPressed(point.position, "stylus")
          longPressActive = true
      }

      onPressedChanged: {
          if (longPressActive)
              mapArea.longPressReleased("stylus")
          longPressActive = false
      }
    }

    // touch clicked & zoom in and out
    TapHandler {
        acceptedDevices: mouseAsTouchScreen ? PointerDevice.AllDevices : PointerDevice.TouchScreen
        property bool longPressActive: false

        onSingleTapped: {
            if( point.modifiers === Qt.RightButton)
            {
              mapCanvasWrapper.zoom(point.position, 1.25)
            }
            else
            {
              timer.tapPoint = point.position
              timer.restart()
            }
        }

        onDoubleTapped: {
            timer.stop();
            mapCanvasWrapper.zoom(point.position, 0.8)
        }

        onLongPressed: {
            mapArea.longPressed(point.position, "touch")
            longPressActive = true
        }

        onPressedChanged: {
            if (longPressActive)
                mapArea.longPressReleased("touch")
            longPressActive = false
        }

        property var timer: Timer {
            property var tapPoint
            interval: 250
            repeat: false

            onTriggered: {
                confirmedClicked(tapPoint)
            }
        }
    }

    DragHandler {
        enabled: !freehandDigitizing
        target: null
        grabPermissions: PointerHandler.ApprovesTakeOverByHandlersOfSameType | PointerHandler.ApprovesTakeOverByHandlersOfDifferentType

        property var oldPos

        onActiveChanged: {
            if ( active )
                freeze('pan')
            else
                unfreeze('pan')
        }

        onCentroidChanged: {
            var oldPos1 = oldPos
            oldPos = centroid.position
            if ( active )
            {
                mapCanvasWrapper.pan(centroid.position, oldPos1)
                panned()
            }
        }
    }

    DragHandler {
        target: null
        acceptedDevices: PointerDevice.Stylus | PointerDevice.Mouse
        grabPermissions: PointerHandler.TakeOverForbidden
        acceptedButtons: Qt.MiddleButton | Qt.RightButton

        property real oldTranslationY
        property point zoomCenter

        onActiveChanged: {
            if (active)
            {
                oldTranslationY = 0
                zoomCenter = centroid.position
            }

            if ( active )
                freeze('zoom')
            else
                unfreeze('zoom')
        }

        onTranslationChanged: {
            if (active)
            {
              mapCanvasWrapper.zoom(zoomCenter, Math.pow(0.8, (oldTranslationY - translation.y)/60))
            }

            oldTranslationY = translation.y
        }
    }

    PinchHandler {
        id: pinch
        target: null
        acceptedDevices: PointerDevice.TouchScreen | PointerDevice.TouchPad
        grabPermissions: PointerHandler.TakeOverForbidden

        property var oldPos
        property real oldScale: 1.0

        onActiveChanged: {
            if ( active ) {
                freeze('pinch')
                oldScale = 1.0
                oldPos = centroid.position
            } else {
                unfreeze('pinch')
            }
        }

        onCentroidChanged: {
            var oldPos1 = oldPos
            oldPos = centroid.position
            if ( active )
            {
                mapCanvasWrapper.pan(centroid.position, oldPos1)
                panned()
            }
        }

        onActiveScaleChanged: {
            mapCanvasWrapper.zoom( pinch.centroid.position, oldScale / pinch.activeScale )
            mapCanvasWrapper.pan( pinch.centroid.position, oldPos )
            panned()
            oldScale = pinch.activeScale
        }
    }


    WheelHandler {
        id: wheel

        onWheel: {
            if ( event.angleDelta.y > 0 )
            {
                zoomIn(point.position)
            }
            else
            {
                zoomOut(point.position)
            }
        }
    }
    // TODO add WheelHandler once we can expect Qt 5.14 on all platforms
}
