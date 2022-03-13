import QtQuick 2.12
import QtQuick.Shapes 1.12
import QtGraphicalEffects 1.0

import org.qgis 1.0
import org.qfield 1.0

import Theme 1.0

Item {
    id: bookmarkRenderer

    property var bookmarkIndex: undefined
    property string bookmarkName: ''
    property string bookmarkId: ''

    property MapSettings mapSettings
    property alias geometryWrapper: geometryWrapper

    QgsGeometryWrapper {
        id: geometryWrapper
    }

    Connections {
        target: geometryWrapper

        function onQgsGeometryChanged() {
            geometryComponent.sourceComponent = undefined
            if (geometryWrapper && geometryWrapper.qgsGeometry.type === QgsWkbTypes.PointGeometry) {
                geometryComponent.sourceComponent = pointHighlight
            }
        }
    }

    Component {
        id: pointHighlight

        Repeater {
            model: geometryWrapper.pointList()

            Item {
                property CoordinateTransformer ct: CoordinateTransformer {
                    id: _ct
                    sourceCrs: geometryWrapper.crs
                    sourcePosition: modelData
                    destinationCrs: mapCanvas.mapSettings.destinationCrs
                    transformContext: qgisProject.transformContext
                }

                MapToScreen {
                    id: mapToScreenPosition
                    mapSettings: mapCanvas.mapSettings
                    mapPoint: _ct.projectedPosition
                }

                Shape {
                    id: bookmark

                    x: mapToScreenPosition.screenPoint.x - width/2
                    y: mapToScreenPosition.screenPoint.y - height

                    width: 36
                    height: 36

                    ShapePath {
                        strokeWidth: 3
                        strokeColor: "white"
                        strokeStyle: ShapePath.SolidLine
                        fillColor: Theme.mainColor
                        startX: 6
                        startY: 16
                        PathArc {
                            x: 30
                            y: 16
                            radiusX: 12
                            radiusY: 14
                        }
                        PathArc {
                            x: 18
                            y: 36
                            radiusX: 36
                            radiusY: 36
                        }
                        PathArc{
                            x: 6
                            y: 16
                            radiusX: 36
                            radiusY: 36
                        }
                    }

                    Rectangle {
                        x: 13
                        y: 9
                        width: 10
                        height: 10
                        color: "white"
                        radius: 4
                    }

                    layer.enabled: true
                    layer.effect: DropShadow {
                        transparentBorder: true
                        radius: 8
                        samples: 25
                        color: "#99000000"
                        horizontalOffset: 0
                        verticalOffset: 0
                    }
                }

                MouseArea {
                    anchors.fill: bookmark
                    onClicked: {
                        displayToast(qsTr('Bookmark: %1').arg(bookmarkRenderer.bookmarkName));
                    }
                    onDoubleClicked: {
                        bookmarkModel.setExtentFromBookmark(bookmarkModel.index(bookmarkRenderer.bookmarkIndex, 0));
                    }
                }
            }
        }
    }

    Loader {
        id: geometryComponent
        // the sourceComponent is updated with the connection on wrapper qgsGeometryChanged signal
        // but it needs to be ready on first used
        sourceComponent: geometryWrapper && geometryWrapper.qgsGeometry.type === QgsWkbTypes.PointGeometry ? pointHighlight : undefined
    }
}

