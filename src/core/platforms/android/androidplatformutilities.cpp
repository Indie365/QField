/***************************************************************************
                            androidplatformutilities.cpp  -  utilities for qfield

                              -------------------
              begin                : February 2016
              copyright            : (C) 2016 by Matthias Kuhn
              email                : matthias@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "androidplatformutilities.h"
#include "androidprojectsource.h"
#include "androidresourcesource.h"
#include "androidviewstatus.h"
#include "appinterface.h"
#include "fileutils.h"
#include "qfield.h"
#include "qfield_android.h"
#include "qfieldcloudconnection.h"

#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QMimeDatabase>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTimer>
#include <QtAndroid>
#include <qgsfileutils.h>

#include <android/bitmap.h>
#include <android/log.h>
#include <jni.h>

const char *const applicationName = "QField";

#define GLUE_HELPER( u, v, w, x, y, z ) u##v##w##x##y##z
#define JNI_FUNCTION_NAME( package_name, class_name, function_name ) GLUE_HELPER( Java_ch_opengis_, package_name, _, class_name, _, function_name )

AndroidPlatformUtilities::AndroidPlatformUtilities()
  : mActivity( QtAndroid::androidActivity() )
  , mSystemGenericDataLocation( QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ) + QStringLiteral( "/share" ) )
{
}

PlatformUtilities::Capabilities AndroidPlatformUtilities::capabilities() const
{
  PlatformUtilities::Capabilities capabilities = Capabilities() | NativeCamera | AdjustBrightness | CustomLocalDataPicker | CustomImport | CustomExport | CustomSend | FilePicker;
#ifdef WITH_SENTRY
  capabilities |= SentryFramework;
#endif
  return capabilities;
}

void AndroidPlatformUtilities::afterUpdate()
{
  // Copy data away from the virtual path `assets:/` to a path accessible also for non-qt-based libs

  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        QAndroidJniObject messageJni = QAndroidJniObject::fromString( QObject::tr( "Please wait while QField installation finalizes." ) );
        activity.callMethod<void>( "showBlockingProgressDialog", "(Ljava/lang/String;)V", messageJni.object<jstring>() );
      }
    } );
  }

  FileUtils::copyRecursively( QStringLiteral( "assets:/share" ), mSystemGenericDataLocation );

  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        activity.callMethod<void>( "dismissBlockingProgressDialog" );
      }
    } );
  }
}

QString AndroidPlatformUtilities::systemSharedDataLocation() const
{
  return mSystemGenericDataLocation;
}

QString AndroidPlatformUtilities::qgsProject() const
{
  return getIntentExtra( "QGS_PROJECT" );
}

QStringList AndroidPlatformUtilities::appDataDirs() const
{
  const QString dataDirs = getIntentExtra( "QFIELD_APP_DATA_DIRS" );
  return ( !dataDirs.isEmpty() ? dataDirs.split( "--;--" ) : QStringList() );
}

QString AndroidPlatformUtilities::applicationDirectory() const
{
  if ( mActivity.isValid() )
  {
    QAndroidJniObject rootDirs = mActivity.callObjectMethod<jstring>( "getApplicationDirectory" );
    if ( rootDirs.isValid() )
    {
      return rootDirs.toString();
    }
  }

  return QString();
}

QStringList AndroidPlatformUtilities::additionalApplicationDirectories() const
{
  if ( mActivity.isValid() )
  {
    QAndroidJniObject rootDirs = mActivity.callObjectMethod<jstring>( "getAdditionalApplicationDirectories" );
    if ( rootDirs.isValid() )
    {
      return rootDirs.toString().split( "--;--" );
    }
  }

  return QStringList();
}

QStringList AndroidPlatformUtilities::rootDirectories() const
{
  if ( mActivity.isValid() )
  {
    QAndroidJniObject rootDirs = mActivity.callObjectMethod<jstring>( "getRootDirectories" );
    if ( rootDirs.isValid() )
    {
      return rootDirs.toString().split( "--;--" );
    }
  }

  return QStringList();
}

void AndroidPlatformUtilities::importProjectFolder() const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        activity.callMethod<void>( "triggerImportProjectFolder" );
      }
    } );
  }
}

void AndroidPlatformUtilities::importProjectArchive() const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        activity.callMethod<void>( "triggerImportProjectArchive" );
      }
    } );
  }
}

void AndroidPlatformUtilities::importDatasets() const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        activity.callMethod<void>( "triggerImportDatasets" );
      }
    } );
  }
}

void AndroidPlatformUtilities::sendDatasetTo( const QString &path ) const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [path] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        QStringList paths = QStringList() << path;
        const QSet<QString> files = QgsFileUtils::sidecarFilesForPath( path );
        for ( const QString &file : files )
        {
          paths << file;
        }
        QAndroidJniObject pathsJni = QAndroidJniObject::fromString( paths.join( "--;--" ) );
        activity.callMethod<void>( "sendDatasetTo", "(Ljava/lang/String;)V", pathsJni.object<jstring>() );
      }
    } );
  }
}

void AndroidPlatformUtilities::exportDatasetTo( const QString &path ) const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [path] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        QStringList paths = QStringList() << path;
        const QSet<QString> files = QgsFileUtils::sidecarFilesForPath( path );
        for ( const QString &file : files )
        {
          paths << file;
        }
        QAndroidJniObject pathsJni = QAndroidJniObject::fromString( paths.join( "--;--" ) );
        activity.callMethod<void>( "exportToFolder", "(Ljava/lang/String;)V", pathsJni.object<jstring>() );
      }
    } );
  }
}

void AndroidPlatformUtilities::removeDataset( const QString &path ) const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [path] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        QAndroidJniObject pathJni = QAndroidJniObject::fromString( path );
        activity.callMethod<void>( "removeDataset", "(Ljava/lang/String;)V", pathJni.object<jstring>() );
      }
    } );
  }
}

void AndroidPlatformUtilities::exportFolderTo( const QString &path ) const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [path] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        QAndroidJniObject pathJni = QAndroidJniObject::fromString( path );
        activity.callMethod<void>( "exportToFolder", "(Ljava/lang/String;)V", pathJni.object<jstring>() );
      }
    } );
  }
}

void AndroidPlatformUtilities::sendCompressedFolderTo( const QString &path ) const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [path] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        QAndroidJniObject pathJni = QAndroidJniObject::fromString( path );
        activity.callMethod<void>( "sendCompressedFolderTo", "(Ljava/lang/String;)V", pathJni.object<jstring>() );
      }
    } );
  }
}

void AndroidPlatformUtilities::removeFolder( const QString &path ) const
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [path] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        QAndroidJniObject pathJni = QAndroidJniObject::fromString( path );
        activity.callMethod<void>( "removeProjectFolder", "(Ljava/lang/String;)V", pathJni.object<jstring>() );
      }
    } );
  }
}

QString AndroidPlatformUtilities::getIntentExtra( const QString &extra, QAndroidJniObject extras ) const
{
  if ( extras == nullptr )
  {
    extras = getNativeExtras();
  }
  if ( extras.isValid() )
  {
    QAndroidJniObject extraJni = QAndroidJniObject::fromString( extra );
    extraJni = extras.callObjectMethod( "getString", "(Ljava/lang/String;)Ljava/lang/String;", extraJni.object<jstring>() );
    if ( extraJni.isValid() )
    {
      return extraJni.toString();
    }
  }
  return QString();
}

QAndroidJniObject AndroidPlatformUtilities::getNativeIntent() const
{
  if ( mActivity.isValid() )
  {
    QAndroidJniObject intent = mActivity.callObjectMethod( "getIntent", "()Landroid/content/Intent;" );
    return intent;
  }
  return nullptr;
}

QAndroidJniObject AndroidPlatformUtilities::getNativeExtras() const
{
  QAndroidJniObject intent = getNativeIntent();
  if ( intent.isValid() )
  {
    QAndroidJniObject extras = intent.callObjectMethod( "getExtras", "()Landroid/os/Bundle;" );

    return extras;
  }
  return nullptr;
}

ResourceSource *AndroidPlatformUtilities::processCameraActivity( const QString &prefix, const QString &filePath, const QString &suffix, bool isVideo )
{
  if ( !checkCameraPermissions() )
    return nullptr;

  const QFileInfo destinationInfo( prefix + filePath );
  const QDir prefixDir( prefix );
  prefixDir.mkpath( destinationInfo.absolutePath() );

  QAndroidJniObject activity = QAndroidJniObject::fromString( QStringLiteral( "ch.opengis." APP_PACKAGE_NAME ".QFieldCameraActivity" ) );
  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;)V", activity.object<jstring>() );
  QAndroidJniObject packageName = QAndroidJniObject::fromString( QStringLiteral( "ch.opengis." APP_PACKAGE_NAME ) );

  intent.callObjectMethod( "setClassName", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", packageName.object<jstring>(), activity.object<jstring>() );

  if ( isVideo )
  {
    QAndroidJniObject isVideo_label = QAndroidJniObject::fromString( "isVideo" );
    QAndroidJniObject isVideo_value = QAndroidJniObject::fromString( "yes" );
    intent.callObjectMethod( "putExtra",
                             "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                             isVideo_label.object<jstring>(),
                             isVideo_value.object<jstring>() );
  }

  QAndroidJniObject filePath_label = QAndroidJniObject::fromString( "filePath" );
  QAndroidJniObject filePath_value = QAndroidJniObject::fromString( filePath );
  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           filePath_label.object<jstring>(),
                           filePath_value.object<jstring>() );

  QAndroidJniObject prefix_label = QAndroidJniObject::fromString( "prefix" );
  QAndroidJniObject prefix_value = QAndroidJniObject::fromString( prefix );
  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           prefix_label.object<jstring>(),
                           prefix_value.object<jstring>() );

  QAndroidJniObject suffix_label = QAndroidJniObject::fromString( "suffix" );
  QAndroidJniObject suffix_value = QAndroidJniObject::fromString( suffix );
  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           suffix_label.object<jstring>(),
                           suffix_value.object<jstring>() );

  QSettings().setValue( QStringLiteral( "QField/nativeCameraLaunched" ), true );

  AndroidResourceSource *pictureSource = new AndroidResourceSource( prefix );

  QtAndroid::startActivity( intent.object<jobject>(), 171, pictureSource );
  return pictureSource;
}

ResourceSource *AndroidPlatformUtilities::getCameraPicture( QQuickItem *parent, const QString &prefix, const QString &pictureFilePath, const QString &suffix )
{
  Q_UNUSED( parent )

  return processCameraActivity( prefix, pictureFilePath, suffix, false );
}

ResourceSource *AndroidPlatformUtilities::getCameraVideo( QQuickItem *parent, const QString &prefix, const QString &videoFilePath, const QString &suffix )
{
  Q_UNUSED( parent )

  return processCameraActivity( prefix, videoFilePath, suffix, true );
}

ResourceSource *AndroidPlatformUtilities::processGalleryActivity( const QString &prefix, const QString &filePath, const QString &mimeType )
{
  const QFileInfo destinationInfo( prefix + filePath );
  const QDir prefixDir( prefix );
  prefixDir.mkpath( destinationInfo.absolutePath() );

  QAndroidJniObject activity = QAndroidJniObject::fromString( QStringLiteral( "ch.opengis." APP_PACKAGE_NAME ".QFieldGalleryActivity" ) );
  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;)V", activity.object<jstring>() );
  QAndroidJniObject packageName = QAndroidJniObject::fromString( QStringLiteral( "ch.opengis." APP_PACKAGE_NAME ) );

  intent.callObjectMethod( "setClassName", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", packageName.object<jstring>(), activity.object<jstring>() );

  QAndroidJniObject filePath_label = QAndroidJniObject::fromString( "filePath" );
  QAndroidJniObject filePath_value = QAndroidJniObject::fromString( filePath );

  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           filePath_label.object<jstring>(),
                           filePath_value.object<jstring>() );

  QAndroidJniObject prefix_label = QAndroidJniObject::fromString( "prefix" );
  QAndroidJniObject prefix_value = QAndroidJniObject::fromString( prefix );

  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           prefix_label.object<jstring>(),
                           prefix_value.object<jstring>() );

  QAndroidJniObject mimeType_label = QAndroidJniObject::fromString( "mimeType" );
  QAndroidJniObject mimeType_value = QAndroidJniObject::fromString( mimeType );

  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           mimeType_label.object<jstring>(),
                           mimeType_value.object<jstring>() );

  AndroidResourceSource *pictureSource = new AndroidResourceSource( prefix );

  QtAndroid::startActivity( intent.object<jobject>(), 171, pictureSource );
  return pictureSource;
}

ResourceSource *AndroidPlatformUtilities::getGalleryPicture( QQuickItem *parent, const QString &prefix, const QString &pictureFilePath )
{
  Q_UNUSED( parent )

  return processGalleryActivity( prefix, pictureFilePath, QStringLiteral( "image/*" ) );
}

ResourceSource *AndroidPlatformUtilities::getGalleryVideo( QQuickItem *parent, const QString &prefix, const QString &videoFilePath )
{
  Q_UNUSED( parent )

  return processGalleryActivity( prefix, videoFilePath, QStringLiteral( "video/*" ) );
}

ResourceSource *AndroidPlatformUtilities::getFile( QQuickItem *parent, const QString &prefix, const QString &filePath, FileType fileType )
{
  Q_UNUSED( parent )

  const QFileInfo destinationInfo( prefix + filePath );
  const QDir prefixDir( prefix );
  prefixDir.mkpath( destinationInfo.absolutePath() );

  QAndroidJniObject activity = QAndroidJniObject::fromString( QStringLiteral( "ch.opengis." APP_PACKAGE_NAME ".QFieldFilePickerActivity" ) );
  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;)V", activity.object<jstring>() );
  QAndroidJniObject packageName = QAndroidJniObject::fromString( QStringLiteral( "ch.opengis." APP_PACKAGE_NAME ) );

  intent.callObjectMethod( "setClassName", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", packageName.object<jstring>(), activity.object<jstring>() );

  QAndroidJniObject filePath_label = QAndroidJniObject::fromString( "filePath" );
  QAndroidJniObject filePath_value = QAndroidJniObject::fromString( filePath );

  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           filePath_label.object<jstring>(),
                           filePath_value.object<jstring>() );

  QAndroidJniObject prefix_label = QAndroidJniObject::fromString( "prefix" );
  QAndroidJniObject prefix_value = QAndroidJniObject::fromString( prefix );

  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           prefix_label.object<jstring>(),
                           prefix_value.object<jstring>() );

  QString mimeType;
  switch ( fileType )
  {
    case AudioFiles:
      mimeType = "audio/*";
      break;
    case AllFiles:
    default:
      mimeType = "*/*";
      break;
  }

  QAndroidJniObject mimeType_label = QAndroidJniObject::fromString( "mimeType" );
  QAndroidJniObject mimeType_value = QAndroidJniObject::fromString( mimeType );

  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           mimeType_label.object<jstring>(),
                           mimeType_value.object<jstring>() );

  AndroidResourceSource *fileSource = new AndroidResourceSource( prefix );

  QtAndroid::startActivity( intent.object<jobject>(), 171, fileSource );

  return fileSource;
}

ViewStatus *AndroidPlatformUtilities::open( const QString &uri, bool editing )
{
  if ( QFileInfo( uri ).isDir() )
    return nullptr;

  checkWriteExternalStoragePermissions();

  QAndroidJniObject activity = QAndroidJniObject::fromString( QStringLiteral( "ch.opengis." APP_PACKAGE_NAME ".QFieldOpenExternallyActivity" ) );
  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;)V", activity.object<jstring>() );
  QAndroidJniObject packageName = QAndroidJniObject::fromString( QStringLiteral( "ch.opengis." APP_PACKAGE_NAME ) );

  intent.callObjectMethod( "setClassName", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", packageName.object<jstring>(), activity.object<jstring>() );

  QMimeDatabase db;
  QAndroidJniObject filepath_label = QAndroidJniObject::fromString( "filepath" );
  QAndroidJniObject filepath = QAndroidJniObject::fromString( uri );
  QAndroidJniObject filetype_label = QAndroidJniObject::fromString( "filetype" );
  QAndroidJniObject filetype = QAndroidJniObject::fromString( db.mimeTypeForFile( uri ).name() );
  QAndroidJniObject fileediting_label = QAndroidJniObject::fromString( "fileediting" );
  QAndroidJniObject fileediting = QAndroidJniObject::fromString( editing ? "true" : "false" );

  intent.callObjectMethod( "putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", filepath_label.object<jstring>(), filepath.object<jstring>() );
  intent.callObjectMethod( "putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", filetype_label.object<jstring>(), filetype.object<jstring>() );
  intent.callObjectMethod( "putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", fileediting_label.object<jstring>(), fileediting.object<jstring>() );

  AndroidViewStatus *viewStatus = new AndroidViewStatus();
  QtAndroid::startActivity( intent.object<jobject>(), 102, viewStatus );

  return viewStatus;
}

bool AndroidPlatformUtilities::checkPositioningPermissions() const
{
  // First check for coarse permissions. If the user configured QField to only get coarse permissions
  // it's his wish and we just let it be.
  QtAndroid::PermissionResult r = QtAndroid::checkPermission( "android.permission.ACCESS_COARSE_LOCATION" );
  if ( r == QtAndroid::PermissionResult::Denied )
  {
    return checkAndAcquirePermissions( "android.permission.ACCESS_FINE_LOCATION" );
  }
  return true;
}

bool AndroidPlatformUtilities::checkCameraPermissions() const
{
  return checkAndAcquirePermissions( "android.permission.CAMERA" );
}

bool AndroidPlatformUtilities::checkMicrophonePermissions() const
{
  return checkAndAcquirePermissions( "android.permission.RECORD_AUDIO" );
}

bool AndroidPlatformUtilities::checkWriteExternalStoragePermissions() const
{
  return checkAndAcquirePermissions( "android.permission.WRITE_EXTERNAL_STORAGE" );
}

bool AndroidPlatformUtilities::checkAndAcquirePermissions( const QString &permissions ) const
{
  QStringList requestedPermissions = permissions.split( ';' );
  requestedPermissions.erase( std::remove_if( requestedPermissions.begin(), requestedPermissions.end(),
                                              []( const QString &permission ) {
                                                return QtAndroid::checkPermission( permission ) != QtAndroid::PermissionResult::Denied;
                                              } ),
                              requestedPermissions.end() );

  if ( !requestedPermissions.isEmpty() )
  {
    QtAndroid::requestPermissionsSync( requestedPermissions );
    for ( const QString &permission : requestedPermissions )
    {
      QtAndroid::PermissionResult r = QtAndroid::checkPermission( permission );
      if ( r == QtAndroid::PermissionResult::Denied )
      {
        return false;
      }
    }
  }

  return true;
}

void AndroidPlatformUtilities::setScreenLockPermission( const bool allowLock )
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [allowLock] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        QAndroidJniObject window = activity.callObjectMethod( "getWindow", "()Landroid/view/Window;" );

        if ( window.isValid() )
        {
          const int FLAG_KEEP_SCREEN_ON = 128;
          if ( !allowLock )
          {
            window.callMethod<void>( "addFlags", "(I)V", FLAG_KEEP_SCREEN_ON );
          }
          else
          {
            window.callMethod<void>( "clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON );
          }
        }
      }

      QAndroidJniEnvironment env;
      if ( env->ExceptionCheck() )
      {
        env->ExceptionClear();
      }
    } );
  }
}

void AndroidPlatformUtilities::dimBrightness()
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        activity.callMethod<void>( "dimBrightness" );
      }
    } );
  }
}

void AndroidPlatformUtilities::restoreBrightness()
{
  if ( mActivity.isValid() )
  {
    QtAndroid::runOnAndroidThread( [] {
      QAndroidJniObject activity = QtAndroid::androidActivity();
      if ( activity.isValid() )
      {
        activity.callMethod<void>( "restoreBrightness" );
      }
    } );
  }
}

QVariantMap AndroidPlatformUtilities::sceneMargins( QQuickWindow *window ) const
{
  Q_UNUSED( window )

  const QAndroidJniObject activity = QtAndroid::androidActivity();
  double statusBarMargin = std::abs( static_cast<double>( activity.callMethod<jdouble>( "statusBarMargin" ) ) );
  double navigationBarMargin = std::abs( static_cast<double>( activity.callMethod<jdouble>( "navigationBarMargin" ) ) );

  statusBarMargin /= QGuiApplication::primaryScreen()->devicePixelRatio();
  navigationBarMargin /= QGuiApplication::primaryScreen()->devicePixelRatio();

  QVariantMap margins;
  margins[QLatin1String( "top" )] = statusBarMargin;
  margins[QLatin1String( "right" )] = 0.0;
  margins[QLatin1String( "bottom" )] = navigationBarMargin;
  margins[QLatin1String( "left" )] = 0.0;
  return margins;
}

void AndroidPlatformUtilities::uploadPendingAttachments( QFieldCloudConnection *connection ) const
{
  QTimer::singleShot( 500, [connection]() {
    if ( connection )
    {
      qInfo() << "Launching service from main...";
      QAndroidJniObject::callStaticMethod<void>( "ch/opengis/" APP_PACKAGE_NAME "/QFieldService",
                                                 "startQFieldService",
                                                 "(Landroid/content/Context;)V",
                                                 QtAndroid::androidActivity().object() );
    }
  } );
}

bool AndroidPlatformUtilities::isSystemDarkTheme() const
{
  if ( mActivity.isValid() )
  {
    bool isDarkTheme = mActivity.callMethod<jboolean>( "isDarkTheme" ) == JNI_TRUE;
    return isDarkTheme;
  }
  return false;
}

#ifdef __cplusplus
extern "C" {
#endif

// QFieldActivity class functions
JNIEXPORT void JNICALL JNI_FUNCTION_NAME( APP_PACKAGE_JNI_NAME, QFieldActivity, openProject )( JNIEnv *env, jobject obj, jstring path )
{
  if ( AppInterface::instance() )
  {
    const char *pathStr = env->GetStringUTFChars( path, NULL );
    AppInterface::instance()->loadFile( QString( pathStr ) );
    env->ReleaseStringUTFChars( path, pathStr );
  }
  return;
}

JNIEXPORT void JNICALL JNI_FUNCTION_NAME( APP_PACKAGE_JNI_NAME, QFieldActivity, openPath )( JNIEnv *env, jobject obj, jstring path )
{
  if ( AppInterface::instance() )
  {
    const char *pathStr = env->GetStringUTFChars( path, NULL );
    emit AppInterface::instance()->openPath( QString( pathStr ) );
    env->ReleaseStringUTFChars( path, pathStr );
  }
  return;
}

#ifdef __cplusplus
}
#endif
