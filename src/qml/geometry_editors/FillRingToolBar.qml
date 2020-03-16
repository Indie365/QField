import ".."
import QtQuick 2.12
import org.qgis 1.0
import org.qfield 1.0
import Theme 1.0

VisibilityFadingRow {
  id: fillRingToolbar

  signal finished()

  property FeatureModel featureModel
  readonly property bool blocking: drawPolygonToolbar.isDigitizing

  spacing: 4 * dp

  DigitizingToolbar {
    id: drawPolygonToolbar
    showConfirmButton: true

    QuestionDialog{
      id: questionDialog
    }

    Loader {
      id: formPopupLoader
    }

    onConfirm: {
      if (!featureModel.currentLayer.editBuffer())
        featureModel.currentLayer.startEditing()
      var result = GeometryUtils.addRingFromRubberBand(featureModel.currentLayer, featureModel.feature.id, rubberbandModel)
      if ( result !== QgsGeometryStatic.Success )
      {
        // TODO WARN
        /*
      AddRingNotClosed, //!< The input ring is not closed
      AddRingNotValid, //!< The input ring is not valid
      AddRingCrossesExistingRings, //!< The input ring crosses existing rings (it is not disjoint)
      AddRingNotInExistingFeature
      */
        featureModel.currentLayer.rollBack()
        cancel()
        finished()
      }
      else
      {
        questionDialog.questionText = qsTr("Do you want to add a ring of fill a polygon in the ring?")
        questionDialog.nButtons = 3
        questionDialog.button1Text = qsTr("Cancel")
        questionDialog.button2Text = qsTr("Add ring")
        questionDialog.button3Text = qsTr("Fill poylgon")

        questionDialog.button1Clicked.connect(rollbackRingAndCancel)
        questionDialog.button2Clicked.connect(commitAndFinish)
        questionDialog.button3Clicked.connect(fillPolygon)
        questionDialog.open()
      }
    }
  }

  function init(featureModel, mapSettings, editorRubberbandModel)
  {
    fillRingToolbar.featureModel = featureModel
    drawPolygonToolbar.rubberbandModel = editorRubberbandModel
    drawPolygonToolbar.rubberbandModel.geometryType = QgsWkbTypes.PolygonGeometry
    drawPolygonToolbar.mapSettings = mapSettings
    drawPolygonToolbar.stateVisible = true
  }

  function cancel()
  {
    drawPolygonToolbar.cancel()
  }

  function commitAndFinish(){
    featureModel.currentLayer.commitChanges()
    cancel()
    finished()
  }

  function rollbackRingAndCancel()
  {
    featureModel.currentLayer.rollBack()
    cancel()
    finished()
  }

  function fillPolygon()
  {
    var polygonGeometry = GeometryUtils.polygonFromRubberband(drawPolygonToolbar.rubberbandModel, featureModel.currentLayer.crs)
    var feature = FeatureUtils.initFeature(featureModel.currentLayer, polygonGeometry)

    // Show form
    formPopupLoader.source = '../EmbeddedFeatureForm.qml'

    formPopupLoader.item.onFeatureSaved.connect(commitAndFinish)
    formPopupLoader.item.onFeatureCancelled.connect(rollbackRingAndCancel)

    formPopupLoader.item.state = 'Add'
    formPopupLoader.item.currentLayer = featureModel.currentLayer
    formPopupLoader.item.feature = feature
    formPopupLoader.item.open()
  }
}
