import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.12

import org.qfield 1.0
import org.qgis 1.0
import Theme 1.0

import ".."
import "."

EditorWidgetBase {
  height: childrenRect.height
  anchors { left: parent.left; right: parent.right; }

  property bool showOpenFormButton: config['ShowOpenFormButton'] === undefined || config['ShowOpenFormButton'] === true

  FeatureCheckListModel {
    id: listModel

    currentLayer: qgisProject.relationManager.relation(config['Relation']).referencedLayer
    keyField: qgisProject.relationManager.relation(config['Relation']).resolveReferencedField(field.name)
    // no, it is not a misspelled version of config['AllowNull']
    addNull: config['AllowNULL']
    orderByValue: config['OrderByValue']

    attributeField: field
    //passing "" instead of undefined, so the model is cleared on adding new features
    attributeValue: value !== undefined ? value : ''
    currentFormFeature: currentFeature
    filterExpression: ""
    allowMulti: false
    onListUpdated: {
      valueChangeRequested( attributeValue, false )
    }
  }

  RelationCombobox {
    id: relationReference
    featureListModel: listModel
    anchors { left: parent.left; right: parent.right; rightMargin: showOpenFormButton ? viewButton.width : 0 }
    enabled: isEnabled
    useSearch: true
    allowAddFeature: config['AllowAddFeatures'] !== undefined && config['AllowAddFeatures'] === true

    property var _relation: qgisProject.relationManager.relation(config['Relation'])
  }

  QfToolButton  {
    id: viewButton

    enabled: showOpenFormButton && relationReference.currentKeyValue !== undefined && relationReference.currentKeyValue !== ''
    anchors { right: parent.right; top: parent.top; }

    width: enabled ? 48 : 0
    height: 48

    bgcolor: "white"
    iconSource: Theme.getThemeIcon("ic_view_green_48dp")

    onClicked: {
        if ( relationReference.currentKeyValue !== undefined && relationReference.currentKeyValue !== '' ) {
          relationReference.embeddedFeatureForm.state = isEnabled ? 'Edit' : 'ReadOnly'
          relationReference.embeddedFeatureForm.currentLayer = listModel.currentLayer
          relationReference.embeddedFeatureForm.feature = listModel.getFeatureFromKeyValue( relationReference.currentKeyValue )
          relationReference.embeddedFeatureForm.open()
        }
    }
  }
}
