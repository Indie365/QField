/***************************************************************************
                            appinterface.cpp
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

#include "appinterface.h"
#include "qfield.h"
#include "qgismobileapp.h"
#if WITH_SENTRY
#include "sentry_wrapper.h"
#endif
#include "ziputils.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <qgsexiftools.h>
#include <qgsmessagelog.h>
#include <qgsziputils.h>

AppInterface *AppInterface::sAppInterface = nullptr;

AppInterface::AppInterface( QgisMobileapp *app )
  : mApp( app )
{
}

void AppInterface::removeRecentProject( const QString &path )
{
  return mApp->removeRecentProject( path );
}

void AppInterface::loadLastProject()
{
  return mApp->loadLastProject();
}

void AppInterface::loadFile( const QString &path, const QString &name )
{
  const QUrl url( path );
  return mApp->loadProjectFile( url.isLocalFile() ? url.toLocalFile() : url.path(), name );
}

void AppInterface::reloadProject()
{
  return mApp->reloadProjectFile();
}

void AppInterface::readProject()
{
  return mApp->readProjectFile();
}

QString AppInterface::readProjectEntry( const QString &scope, const QString &key, const QString &def ) const
{
  return mApp->readProjectEntry( scope, key, def );
}

int AppInterface::readProjectNumEntry( const QString &scope, const QString &key, int def ) const
{
  return mApp->readProjectNumEntry( scope, key, def );
}

double AppInterface::readProjectDoubleEntry( const QString &scope, const QString &key, double def ) const
{
  return mApp->readProjectDoubleEntry( scope, key, def );
}

bool AppInterface::print( const QString &layoutName )
{
  return mApp->print( layoutName );
}

bool AppInterface::printAtlasFeatures( const QString &layoutName, const QList<long long> &featureIds )
{
  return mApp->printAtlasFeatures( layoutName, featureIds );
}

void AppInterface::openFeatureForm()
{
  emit openFeatureFormRequested();
}

void AppInterface::setScreenDimmerTimeout( int timeoutSeconds )
{
  mApp->setScreenDimmerTimeout( timeoutSeconds );
}

QVariantMap AppInterface::availableLanguages() const
{
  QVariantMap languages;
  QDirIterator it( QStringLiteral( ":/i18n/" ), { QStringLiteral( "*.qm" ) }, QDir::Files );
  while ( it.hasNext() )
  {
    it.next();
    if ( it.fileName().startsWith( "qfield_" ) )
    {
      const int delimiter = it.fileName().indexOf( '.' );
      const QString languageCode = it.fileName().mid( 7, delimiter - 7 );
      const bool hasCoutryCode = languageCode.indexOf( '_' ) > -1;

      const QLocale locale( languageCode );
      QString displayName;
      if ( languageCode == QStringLiteral( "en" ) )
      {
        displayName = QStringLiteral( "english" );
      }
      else if ( locale.nativeLanguageName().isEmpty() )
      {
        displayName = QStringLiteral( "code (%1)" ).arg( languageCode );
      }
      else
      {
        displayName = locale.nativeLanguageName().toLower() + ( hasCoutryCode ? QStringLiteral( " / %1" ).arg( locale.nativeCountryName() ) : QString() );
      }

      languages.insert( languageCode, displayName );
    }
  }
  return languages;
}

bool AppInterface::isFileExtensionSupported( const QString &filename ) const
{
  const QFileInfo fi( filename );
  const QString suffix = fi.suffix().toLower();
  return SUPPORTED_PROJECT_EXTENSIONS.contains( suffix ) || SUPPORTED_VECTOR_EXTENSIONS.contains( suffix ) || SUPPORTED_RASTER_EXTENSIONS.contains( suffix );
}

void AppInterface::logMessage( const QString &message )
{
  QgsMessageLog::logMessage( message, QStringLiteral( "QField" ) );
}

void AppInterface::sendLog( const QString &message )
{
#if WITH_SENTRY
  sentry_wrapper::capture_event( message.toUtf8().constData() );
#endif
}

void AppInterface::initiateSentry() const
{
#if WITH_SENTRY
  sentry_wrapper::init();
#endif
}

void AppInterface::closeSentry() const
{
#if WITH_SENTRY
  sentry_wrapper::close();
#endif
}

void AppInterface::restrictImageSize( const QString &imagePath, int maximumWidthHeight )
{
  QVariantMap metadata = QgsExifTools::readTags( imagePath );
  QImage img( imagePath );
  if ( !img.isNull() && ( img.width() > maximumWidthHeight || img.height() > maximumWidthHeight ) )
  {
    QImage scaledImage = img.width() > img.height()
                           ? img.scaledToWidth( maximumWidthHeight, Qt::SmoothTransformation )
                           : img.scaledToHeight( maximumWidthHeight, Qt::SmoothTransformation );
    scaledImage.save( imagePath );

    for ( const QString key : metadata.keys() )
    {
      QgsExifTools::tagImage( imagePath, key, metadata[key] );
    }
  }
}

void AppInterface::importUrl( const QString &url )
{
  if ( url.isEmpty() )
    return;

  QUrl u = QUrl( url );
  emit importUrlTriggered( url, u.fileName() );

  QgsNetworkAccessManager *manager = QgsNetworkAccessManager::instance();
  QNetworkRequest request( u );
  request.setAttribute( QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy );
  QNetworkReply *reply = manager->get( request );
  connect( reply, &QNetworkReply::finished, this, [=]() {
    const QString applicationDirectory = PlatformUtilities::instance()->applicationDirectory();
    if ( !applicationDirectory.isEmpty() && reply->error() == QNetworkReply::NoError )
    {
      QString fileName = reply->url().fileName();
      QString contentDisposition = reply->header( QNetworkRequest::ContentDispositionHeader ).toString();
      if ( !contentDisposition.isEmpty() )
      {
        QRegularExpression rx( QStringLiteral( "filename=\"?([^\";]*)\"?" ) );
        QRegularExpressionMatch match = rx.match( contentDisposition );
        if ( match.hasMatch() )
        {
          fileName = match.captured( 1 );
        }
      }

      QFileInfo fileInfo = QFileInfo( fileName );
      const QString fileSuffix = fileInfo.completeSuffix().toLower();
      const bool isProjectFile = fileSuffix == QLatin1String( "qgs" ) || fileSuffix == QLatin1String( "qgz" );

      QString filePath = QStringLiteral( "%1/%2/%3" ).arg( applicationDirectory, isProjectFile ? QLatin1String( "Imported Projects" ) : QLatin1String( "Imported Datasets" ), fileName );
      {
        int i = 0;
        while ( QFileInfo::exists( filePath ) )
        {
          filePath = QStringLiteral( "%1/%2/%3_%4.%5" ).arg( applicationDirectory, isProjectFile ? QLatin1String( "Imported Projects" ) : QLatin1String( "Imported Datasets" ), fileInfo.baseName(), QString::number( ++i ), fileSuffix );
        }
      }
      QDir( QFileInfo( filePath ).absolutePath() ).mkpath( "." );

      QFile file( filePath );
      if ( file.open( QIODevice::WriteOnly ) )
      {
        const QByteArray data = reply->readAll();
        file.write( data );
        file.close();

        if ( fileSuffix == QLatin1String( "zip" ) )
        {
          // Check if this is a compressed project and handle accordingly
          QString zipDirectory = QStringLiteral( "%1/Imported Projects/%2" ).arg( applicationDirectory, fileInfo.baseName() );
          {
            int i = 0;
            while ( QFileInfo::exists( zipDirectory ) )
            {
              zipDirectory = QStringLiteral( "%1/Imported Projects/%2_%3" ).arg( applicationDirectory, fileInfo.baseName(), QString::number( ++i ) );
            }
          }
          QDir( zipDirectory ).mkpath( "." );

          QStringList files;
          ZipUtils::unzip( filePath, zipDirectory, files, false );
          QString projectFilePath;
          for ( const QString &file : std::as_const( files ) )
          {
            if ( file.toLower().endsWith( QLatin1String( ".qgs" ) ) || file.toLower().endsWith( QLatin1String( ".qgz" ) ) )
            {
              projectFilePath = file;
              break;
            }
          }

          if ( !projectFilePath.isEmpty() )
          {
            // It's a project, remove the ZIP and use the decompressed folder
            file.remove();
            filePath = projectFilePath;
          }
          else
          {
            // Not a project file, remove
            QDir dir( zipDirectory );
            dir.removeRecursively();
          }
        }

        emit importUrlEnded( QFileInfo( filePath ).absolutePath() );
        return;
      }
    }

    emit importUrlEnded();
  } );
}
