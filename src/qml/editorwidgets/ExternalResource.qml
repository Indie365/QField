import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Window 2.14
import QtMultimedia 5.14

import org.qgis 1.0
import org.qfield 1.0
import Theme 1.0

import "."
import ".." as QField

EditorWidgetBase {
  anchors.left: parent.left
  anchors.right: parent.right

  height: childrenRect.height

  ExpressionEvaluator {
    id: rootPathEvaluator
  }
  property string prefixToRelativePath: {
    if (qgisProject == undefined)
      return "";

    var path = ""
    if (config["RelativeStorage"] === 1 ) {
      path = qgisProject.homePath
      if (!path.endsWith("/")) path = path +  "/"
    } else if (config["RelativeStorage"] === 2 ) {
      var collection = config["PropertyCollection"]
      var props = collection["properties"]
      if (props) {
        if(props["propertyRootPath"]) {
          var rootPathProps = props["propertyRootPath"]
          rootPathEvaluator.expressionText = rootPathProps["expression"]
        }
      }
      rootPathEvaluator.feature = currentFeature
      rootPathEvaluator.layer = currentLayer
      var evaluatedFilepath = rootPathEvaluator.evaluate().replace("\\", "/")
      if (evaluatedFilepath) {
        path = evaluatedFilepath
      } else {
        path = config["DefaultRoot"] ? config["DefaultRoot"] : qgisProject.homePath
        if (!path.endsWith("/")) path = path +  "/"
      }
    }

    // since we've hardcoded the project path by default so far, let's maintain that until we improve things in qfieldsync
    if (path == "") {
      path = qgisProject.homePath
      if (!path.endsWith("/")) path = path +  "/"
    }

    return path
  }

  property ResourceSource __resourceSource
  property ViewStatus __viewStatus

  // config.DocumentViewer values:
  // - 0 = no type handled as file
  // - 1 = image
  // - 2 = web content (TODO: implement)
  // - 3 = audio
  // - 4 = video
  property int documentViewer: config.DocumentViewer

  property bool isImage: !config.UseLink && FileUtils.mimeTypeName(prefixToRelativePath + value ).startsWith("image/")
  property bool isAudio: !config.UseLink && FileUtils.mimeTypeName(prefixToRelativePath + value).startsWith("audio/")
  property bool isVideo: !config.UserLinke && FileUtils.mimeTypeName(prefixToRelativePath + value).startsWith("video/")

  //to not break any binding of image.source
  property var currentValue: value
  onCurrentValueChanged: {
    if (currentValue != undefined && currentValue !== '')
    {
      image.visible = isImage
      geoTagBadge.visible = isImage
      if (isImage) {
        mediaFrame.height = 200

        image.visible = true
        image.hasImage = true
        image.opacity = 1
        image.anchors.topMargin = 0
        image.source= 'file://' + prefixToRelativePath + value
        geoTagBadge.hasGeoTag = ExifTools.hasGeoTag(prefixToRelativePath + value)
        geoTagBadge.visible = true
      }
    }
  }

  ExpressionEvaluator {
    id: expressionEvaluator
    feature: currentFeature
    layer: currentLayer
    expressionText: {
      if ( currentLayer && currentLayer.customProperty('QFieldSync/attachment_naming') !== undefined ) {
        var value =JSON.parse(currentLayer.customProperty('QFieldSync/attachment_naming'))[field.name];
        return value !== undefined ? value : ''
      } else {
        return ''
      }
    }
  }

  function getResourceFilePath() {
    var evaluatedFilepath = expressionEvaluator.evaluate()
    var filepath = evaluatedFilepath;
    if (FileUtils.fileSuffix(evaluatedFilepath) === '') {
      // we need an extension for media types (image, audio, video), fallback to hardcoded values
      if (documentViewer == 1) {
        filepath = 'DCIM/JPEG_' + (new Date()).toISOString().replace(/[^0-9]/g, '') + '.{extension}';
      } else if (documentViewer == 3) {
        filepath = 'audio/AUDIO_' + (new Date()).toISOString().replace(/[^0-9]/g, '') + '.{extension}';
      } else if (documentViewer == 4) {
        filepath = 'video/VIDEO_' + (new Date()).toISOString().replace(/[^0-9]/g, '') + '.{extension}';
      } else {
        filepath = 'files/' + (new Date()).toISOString().replace(/[^0-9]/g, '') + '_{filename}';
      }
    }
    filepath = filepath.replace('\\', '/')
    return filepath;
  }

  Label {
    id: linkField

    topPadding: 10
    bottomPadding: 10
    height: fontMetrics.height + 30

    property bool hasValue: false
    visible: hasValue && !isImage && !isAudio && !isVideo

    anchors.left: parent.left
    anchors.right: fileButton.left
    color: FileUtils.fileExists(prefixToRelativePath + value) ? Theme.mainColor : 'gray'

    text: {
      var fieldValue = prefixToRelativePath + currentValue
      if (UrlUtils.isRelativeOrFileUrl(fieldValue)) {
        fieldValue = config.FullUrl ? fieldValue : FileUtils.fileName(fieldValue)
      }
      fieldValue = StringUtils.insertLinks(fieldValue)

      hasValue = currentValue !== undefined && !!fieldValue
      return hasValue ? fieldValue : qsTr('No Value')
    }

    font.pointSize: Theme.defaultFont.pointSize
    font.italic: !hasValue
    font.underline: FileUtils.fileExists(prefixToRelativePath + value) || FileUtils.fileExists(value)
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideMiddle

    background: Rectangle {
      y: linkField.height - height - linkField.bottomPadding / 2
      implicitWidth: 120
      height: 1
      color: Theme.accentLightColor
    }

    MouseArea {
      anchors.fill: parent

      onClicked: {
        if ( !value )
          return

        if (!UrlUtils.isRelativeOrFileUrl(value)) { // matches `http://...` but not `file://...` paths
          Qt.openUrlExternally(value)
        } else if (FileUtils.fileExists(prefixToRelativePath + value)) {
          __viewStatus = platformUtilities.open(prefixToRelativePath + value, isEnabled)
        }
      }
    }
  }

  FontMetrics {
    id: fontMetrics
    font: linkField.font
  }

  Rectangle {
    id: mediaFrame
    width: parent.width - fileButton.width - galleryButton.width - cameraButton.width
    height: 48
    visible: !linkField.visible
    color: isEnabled ? Theme.lightGray : "transparent"
    radius: 2
    clip: true

    Image {
      id: image

      property bool hasImage: false

      visible: isImage
      enabled: isImage
      anchors.centerIn: parent
      width: hasImage ? parent.width : 24
      height: hasImage ? parent.height : 24
      opacity: 0.25
      autoTransform: true
      fillMode: Image.PreserveAspectFit
      horizontalAlignment: Image.AlignHCenter
      verticalAlignment: Image.AlignVCenter

      source: Theme.getThemeIcon("ic_photo_notavailable_black_24dp")
      cache: false

      MouseArea {
        anchors.fill: parent

        onClicked: {
          if ( FileUtils.fileExists( prefixToRelativePath + value ) ) {
            __viewStatus = platformUtilities.open( prefixToRelativePath + value, isEnabled );
          }
        }
      }

      Image {
        property bool hasGeoTag: false
        id: geoTagBadge
        visible: false
        anchors.top: image.top
        anchors.right: image.right
        anchors.rightMargin: 10
        anchors.topMargin: 12
        fillMode: Image.PreserveAspectFit
        width: 24
        height: 24
        source: hasGeoTag ? Theme.getThemeIcon("ic_geotag_24dp") : Theme.getThemeIcon("ic_geotag_missing_24dp")
        sourceSize.width: 24 * Screen.devicePixelRatio
        sourceSize.height: 24 * Screen.devicePixelRatio
      }

      QfDropShadow {
        anchors.fill: geoTagBadge
        visible: geoTagBadge.visible
        horizontalOffset: 0
        verticalOffset: 0
        radius: 6.0
        color: "#DD000000"
        source: geoTagBadge
      }
    }
    Rectangle {
      color: "transparent"
      anchors.left: parent.left
      anchors.right: parent.right
      height: isEnabled ? parent.height : 1
      y: isEnabled ? 0 : parent.height - 1
      border.width: 1
      border.color: Theme.accentLightColor
      radius: 2
    }
  }

  QfToolButton {
    id: fileButton
    width: visible ? 48 : 0
    height: 48

    visible: documentViewer == 0 && isEnabled

    anchors.right: cameraButton.left
    anchors.top: parent.top

    bgcolor: "transparent"

    onClicked: {
      Qt.inputMethod.hide()
      var filepath = getResourceFilePath()
      __resourceSource = platformUtilities.getFile(this, qgisProject.homePath+'/', filepath)
    }

    iconSource: Theme.getThemeIcon("ic_file_black_24dp")
  }

  QfToolButton {
    id: cameraButton
    width: visible ? 48 : 0
    height: 48

    property bool isCameraAvailable: platformUtilities.capabilities & PlatformUtilities.NativeCamera || QtMultimedia.availableCameras.length > 0

    // QField has historically handled no viewer type as image, let's carry that on
    visible: (documentViewer == 0 || documentViewer == 1) && isEnabled

    anchors.right: galleryButton.left
    anchors.top: parent.top

    bgcolor: "transparent"

    onClicked: {
      Qt.inputMethod.hide()
      if ( platformUtilities.capabilities & PlatformUtilities.NativeCamera && settings.valueBool("nativeCamera", true) ) {
        var filepath = getResourceFilePath()
        __resourceSource = platformUtilities.getCameraPicture(this, qgisProject.homePath+'/', filepath, FileUtils.fileSuffix(filepath) )
      } else {
        platformUtilities.createDir( qgisProject.homePath, 'DCIM' )
        camloader.active = true
      }
    }

    iconSource: Theme.getThemeIcon("ic_camera_alt_border_24dp")
  }

  QfToolButton {
    id: galleryButton
    width: visible ? 48 : 0
    height: 48

    // QField has historically handled no viewer type as image, let's carry that on
    visible: (documentViewer == 0 || documentViewer == 1) && isEnabled

    anchors.right: parent.right
    anchors.top: parent.top

    bgcolor: "transparent"

    onClicked: {
      Qt.inputMethod.hide()
      var filepath = getResourceFilePath()
      __resourceSource = platformUtilities.getGalleryPicture(this, qgisProject.homePath+'/', filepath)
    }

    iconSource: Theme.getThemeIcon("baseline_photo_library_black_24")
  }

  Loader {
    id: camloader
    sourceComponent: camcomponent
    active: false
  }

  Component {
    id: camcomponent

    Popup {
      id: campopup
      z: 10000 // 1000s are embedded feature forms, use a higher value to insure feature form popups always show above embedded feature forms

      Component.onCompleted: {
        if ( platformUtilities.checkCameraPermissions() )
          open()
      }

      parent: ApplicationWindow.overlay

      x: 0
      y: 0
      height: parent.height
      width: parent.width

      modal: true
      focus: true

      QField.QFieldCamera {
        id: qfieldCamera

        visible: true

        onFinished: {
          var filepath = getResourceFilePath()
          platformUtilities.renameFile(path, prefixToRelativePath + filepath)

          var maximumWidhtHeight = iface.readProjectNumEntry("qfieldsync", "maximumImageWidthHeight", 0)
          if(maximumWidhtHeight > 0) {
            iface.restrictImageSize(prefixToRelativePath + filepath, maximumWidhtHeight)
          }

          valueChangeRequested(filepath, false)
          campopup.close()
        }
        onCanceled: {
          campopup.close()
        }
      }
      onClosed: camloader.active = false
    }
  }

  Connections {
    target: __resourceSource
    function onResourceReceived(path) {
      if( path )
      {
        var maximumWidhtHeight = iface.readProjectNumEntry("qfieldsync", "maximumImageWidthHeight", 0)
        if(maximumWidhtHeight > 0) {
          iface.restrictImageSize(prefixToRelativePath + path, maximumWidhtHeight)
        }

        valueChangeRequested(path, false)
      }
    }
  }

  Connections {
    target: __viewStatus

    onFinished: {
      if (isImage) {
        // In order to make sure the image shown reflects edits, reset the source
        var imageSource = image.source;
        image.source = '';
        image.source = imageSource;
      }
    }

    onStatusReceived: {
      if( status )
      {
        //default message (we would have the passed error message still)
        displayToast( qsTr("Cannot handle this file type"), 'error')
      }
    }
  }
}
