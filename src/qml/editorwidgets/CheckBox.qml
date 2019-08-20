import QtQuick 2.0
import QtQuick.Controls 2.4

Item {
  signal valueChanged( var value, bool isNull )

  height: childrenRect.height

  anchors {
    right: parent.right
    left: parent.left
  }

  CheckBox {

    property var currentValue: value
    //if the field type is boolean, ignore the configured 'CheckedState' and 'UncheckedState' values and work with true/false always
    readonly property bool isBool: field.type == 1 //needs type coercion

    checked: isBool ? currentValue !== undefined ? currentValue : false : String(currentValue) === config['CheckedState']

    onCheckedChanged: {
      console.log("isbool "+isBool+" and checked "+checked+" and "+currentValue+" and value "+value);
      valueChanged( isBool ? checked : checked ? config['CheckedState'] : config['UncheckedState'], false )
      forceActiveFocus()
    }

    indicator.height: 16 * dp
    indicator.width: 16 * dp
    indicator.implicitHeight: 24 * dp
    indicator.implicitWidth: 24 * dp
  }
}
