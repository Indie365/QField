/***************************************************************************
    qfieldcloudprojectsmodel.cpp
    ---------------------
    begin                : January 2020
    copyright            : (C) 2020 by Matthias Kuhn
    email                : matthias at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qfieldcloudprojectsmodel.h"
#include "qfieldcloudconnection.h"
#include "qfieldcloudutils.h"
#include "layerobserver.h"
#include "deltafilewrapper.h"
#include "fileutils.h"
#include "qfield.h"

#include <qgis.h>
#include <qgsnetworkaccessmanager.h>
#include <qgsapplication.h>
#include <qgsproject.h>
#include <qgsproviderregistry.h>
#include <qgsmessagelog.h>

#include <QNetworkReply>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QDirIterator>
#include <QSettings>
#include <QDebug>

QFieldCloudProjectsModel::QFieldCloudProjectsModel() :
  mProject( QgsProject::instance() )
{
  QJsonArray projects;
  reload( projects );

  // TODO all of these connects are a bit too much, and I guess not very precise, should be refactored!
  connect( this, &QFieldCloudProjectsModel::currentProjectIdChanged, this, [ = ]()
  {
    const int index = findProject( mCurrentProjectId );

    if ( index == -1 || index >= mCloudProjects.size() )
      return;

    refreshProjectModification( mCurrentProjectId );
  } );

  connect( this, &QFieldCloudProjectsModel::modelReset, this, [ = ]()
  {
    const int index = findProject( mCurrentProjectId );

    if ( index == -1 || index >= mCloudProjects.size() )
      return;

    emit currentProjectDataChanged();

    refreshProjectModification( mCurrentProjectId );
  } );

  connect( this, &QFieldCloudProjectsModel::dataChanged, this, [ = ]( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> &roles )
  {
    Q_UNUSED( bottomRight );
    Q_UNUSED( roles );

    const int index = findProject( mCurrentProjectId );

    if ( index == -1 || index >= mCloudProjects.size() )
      return;

    // current project
    if ( topLeft.row() == index )
    {
      emit currentProjectDataChanged();
    }
  } );

  connect( this, &QFieldCloudProjectsModel::cloudConnectionChanged, this, [ = ]()
  {
    if ( !mCloudConnection )
      return;

    mUsername = mCloudConnection->username();
    connect( mCloudConnection, &QFieldCloudConnection::usernameChanged, this, [ = ]()
    {
      mUsername = mCloudConnection->username();
    } );
  } );
}

QFieldCloudConnection *QFieldCloudProjectsModel::cloudConnection() const
{
  return mCloudConnection;
}

void QFieldCloudProjectsModel::setCloudConnection( QFieldCloudConnection *cloudConnection )
{
  if ( mCloudConnection == cloudConnection )
    return;

  if ( cloudConnection )
    connect( cloudConnection, &QFieldCloudConnection::statusChanged, this, &QFieldCloudProjectsModel::connectionStatusChanged );

  mCloudConnection = cloudConnection;
  emit cloudConnectionChanged();
}

LayerObserver *QFieldCloudProjectsModel::layerObserver() const
{
  return mLayerObserver;
}

void QFieldCloudProjectsModel::setLayerObserver( LayerObserver *layerObserver )
{
  if ( mLayerObserver == layerObserver )
    return;

  mLayerObserver = layerObserver;

  if ( ! layerObserver )
    return;

  connect( layerObserver, &LayerObserver::layerEdited, this, &QFieldCloudProjectsModel::layerObserverLayerEdited );
  connect( layerObserver->deltaFileWrapper(), &DeltaFileWrapper::countChanged, this, [ = ]()
  {
    refreshProjectModification( mCurrentProjectId );
  } );

  emit layerObserverChanged();
}

QString QFieldCloudProjectsModel::currentProjectId() const
{
  return mCurrentProjectId;
}

void QFieldCloudProjectsModel::setCurrentProjectId( const QString &currentProjectId )
{
  if ( mCurrentProjectId == currentProjectId )
    return;

  mCurrentProjectId = currentProjectId;
  emit currentProjectIdChanged();
  emit currentProjectDataChanged();
}

QVariantMap QFieldCloudProjectsModel::currentProjectData() const
{
  return getProjectData( mCurrentProjectId );
}

QVariantMap QFieldCloudProjectsModel::getProjectData( const QString &projectId ) const
{
  QVariantMap data;

  const int index = findProject( projectId );
  const QModelIndex idx = this->index( index, 0 );

  if ( !idx.isValid() )
    return data;

  const QHash<int, QByteArray> rn = this->roleNames();

  for ( auto [ key, value ] : qfield::asKeyValueRange( rn ) )
  {
    data[value] = idx.data( key );
  }

  return data;
}

void QFieldCloudProjectsModel::refreshProjectsList()
{
  switch ( mCloudConnection->status() )
  {
    case QFieldCloudConnection::ConnectionStatus::LoggedIn:
    {
      NetworkReply *reply = mCloudConnection->get( QStringLiteral( "/api/v1/projects/" ) );
      connect( reply, &NetworkReply::finished, this, &QFieldCloudProjectsModel::projectListReceived );
      break;
    }
    case QFieldCloudConnection::ConnectionStatus::Disconnected:
    {
      QJsonArray projects;
      reload( projects );
      break;
    }
    case QFieldCloudConnection::ConnectionStatus::Connecting:
      // Nothing done for this intermediary status.
      break;
  }
}

int QFieldCloudProjectsModel::findProject( const QString &projectId ) const
{
  const QList<CloudProject> cloudProjects = mCloudProjects;
  int index = -1;
  for ( int i = 0; i < cloudProjects.count(); i++ )
  {
    if ( cloudProjects.at( i ).id == projectId )
    {
      index = i;
      break;
    }
  }
  return index;
}

void QFieldCloudProjectsModel::removeLocalProject( const QString &projectId )
{
  QDir dir( QStringLiteral( "%1/%2/%3" ).arg( QFieldCloudUtils::localCloudDirectory(), mUsername, projectId ) );

  if ( dir.exists() )
  {
    int index = findProject( projectId );
    if ( index > -1 )
    {
      if ( mCloudProjects.at( index ).status == ProjectStatus::Idle && mCloudProjects.at( index ).checkout & RemoteCheckout )
      {
        mCloudProjects[index].localPath = QString();
        mCloudProjects[index].checkout = RemoteCheckout;
        mCloudProjects[index].modification = NoModification;
        QModelIndex idx = createIndex( index, 0 );
        emit dataChanged( idx, idx,  QVector<int>() << StatusRole << LocalPathRole << CheckoutRole );
      }
      else
      {
        beginRemoveRows( QModelIndex(), index, index );
        mCloudProjects.removeAt( index );
        endRemoveRows();
      }
    }

    dir.removeRecursively();
  }

  QSettings().remove( QStringLiteral( "QFieldCloud/projects/%1" ).arg( projectId ) );
}

QFieldCloudProjectsModel::ProjectStatus QFieldCloudProjectsModel::projectStatus( const QString &projectId ) const
{
  const int index = findProject( projectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return QFieldCloudProjectsModel::ProjectStatus::Idle;

  return mCloudProjects[index].status;
}

bool QFieldCloudProjectsModel::canSyncProject( const QString &projectId ) const
{
  const int index = findProject( projectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return false;

  if ( mCurrentProjectId.isEmpty() )
    return false;
  else if ( projectStatus( projectId ) == ProjectStatus::Idle )
    return true;

  return false;
}

QFieldCloudProjectsModel::ProjectModifications QFieldCloudProjectsModel::projectModification( const QString &projectId ) const
{
  const int index = findProject( projectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return NoModification;

  return mCloudProjects[index].modification;
}

void QFieldCloudProjectsModel::refreshProjectModification( const QString &projectId )
{
  const int index = findProject( projectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return;

  ProjectModifications oldModifications = mCloudProjects[index].modification;

  if ( mLayerObserver->deltaFileWrapper()->count() > 0 )
    mCloudProjects[index].modification |= LocalModification;
  else if ( mCloudProjects[index].modification & LocalModification )
    mCloudProjects[index].modification ^= LocalModification;

  if ( oldModifications != mCloudProjects[index].modification )
  {
    QModelIndex idx = createIndex( index, 0 );
    emit dataChanged( idx, idx, QVector<int>() << ModificationRole << CanSyncRole );
  }
}

QString QFieldCloudProjectsModel::layerFileName( const QgsMapLayer *layer ) const
{
  return layer->dataProvider()->dataSourceUri().split( '|' )[0];
}

QFieldCloudProjectsModel::ExportStatus QFieldCloudProjectsModel::exportStatus( const QString &status ) const
{
  const QString statusUpper = status.toUpper();
  if ( statusUpper == QStringLiteral( "STATUS_PENDING" ) )
    return ExportPendingStatus;
  else if ( statusUpper == QStringLiteral( "STATUS_BUSY" ) )
    return ExportBusyStatus;
  else if ( statusUpper == QStringLiteral( "STATUS_EXPORTED" ) )
    return ExportFinishedStatus;
  else
    // "STATUS_ERROR" or any unknown status is considered an error
    return ExportErrorStatus;
}

void QFieldCloudProjectsModel::downloadProject( const QString &projectId )
{
  if ( !mCloudConnection )
    return;

  int index = findProject( projectId );
  if ( index == -1 || index >= mCloudProjects.size() )
    return;

  // before downloading, the project should be idle
  if ( mCloudProjects[index].status != ProjectStatus::Idle )
    return;

  // before downloading, there should be no local modification, otherwise it will be overwritten
  if ( mCloudProjects[index].modification & LocalModification )
    return;

  mCloudProjects[index].exportStatus = ExportUnstartedStatus;
  mCloudProjects[index].exportStatusString = QString();
  mCloudProjects[index].downloadFileTransfers.clear();
  mCloudProjects[index].downloadFilesFinished = 0;
  mCloudProjects[index].downloadFilesFailed = 0;
  mCloudProjects[index].downloadBytesTotal = 0;
  mCloudProjects[index].downloadBytesReceived = 0;
  mCloudProjects[index].downloadProgress = 0;

  mCloudProjects[index].status = ProjectStatus::Downloading;
  mCloudProjects[index].errorStatus = NoErrorStatus;
  mCloudProjects[index].modification = NoModification;
  QModelIndex idx = createIndex( index, 0 );
  emit dataChanged( idx, idx,  QVector<int>() << StatusRole << ErrorStatusRole << DownloadProgressRole );

  // //////////
  // 1) request a new export the project
  // //////////
  NetworkReply *reply = mCloudConnection->post( QStringLiteral( "/api/v1/qfield-files/export/%1/" ).arg( projectId ) );

  connect( reply, &NetworkReply::finished, reply, [ = ]()
  {
    QNetworkReply *rawReply = reply->reply();
    reply->deleteLater();

    if ( rawReply->error() != QNetworkReply::NoError )
    {
      mCloudProjects[index].status = ProjectStatus::Idle;

      QModelIndex idx = createIndex( index, 0 );
      emit dataChanged( idx, idx,  QVector<int>() << StatusRole << DownloadProgressRole );
      // TODO this should not be a warning, but a meaningful message to the user!
      emit warning( QFieldCloudConnection::errorString( rawReply ) );

      return;
    }

    const QJsonObject payload = QJsonDocument::fromJson( rawReply->readAll() ).object();
    Q_ASSERT( ! payload.isEmpty() );
    mCloudProjects[index].exportStatus = exportStatus( payload.value( QStringLiteral( "status" ) ).toString() );

    emit dataChanged( idx, idx, QVector<int>() << ExportStatusRole );

    projectGetDownloadStatus( projectId );
  } );
}


void QFieldCloudProjectsModel::projectGetDownloadStatus( const QString &projectId )
{
  const int index = findProject( projectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return;

  Q_ASSERT( index >= 0 && index < mCloudProjects.size() );
  Q_ASSERT( mCloudProjects[index].exportStatus != ExportUnstartedStatus );

  QModelIndex idx = createIndex( index, 0 );
  NetworkReply *downloadStatusReply = mCloudConnection->get( QStringLiteral( "/api/v1/qfield-files/export/%1/" ).arg( projectId ) );

  mCloudProjects[index].exportStatusString = QString();

  connect( downloadStatusReply, &NetworkReply::finished, this, [ = ]()
  {
    QNetworkReply *rawReply = downloadStatusReply->reply();
    const QJsonObject payload = QJsonDocument::fromJson( rawReply->readAll() ).object();
    downloadStatusReply->deleteLater();

    Q_ASSERT( downloadStatusReply->isFinished() );
    Q_ASSERT( rawReply );
    Q_ASSERT( ! payload.isEmpty() );

    mCloudProjects[index].exportStatus = ( rawReply->error() == QNetworkReply::NoError )
                                         ? exportStatus( payload.value( QStringLiteral( "status" ) ).toString() )
                                         : ExportErrorStatus;

    switch ( mCloudProjects[index].exportStatus )
    {
      case ExportUnstartedStatus:
        // download export job should be already started!!!
        Q_ASSERT( 0 );
        FALLTHROUGH
      case ExportPendingStatus:
      case ExportBusyStatus:
        // infinite retry, there should be one day, when we can get the status!
        QTimer::singleShot( sDelayBeforeStatusRetry, [ = ]()
        {
          projectGetDownloadStatus( projectId );
        } );
        break;
      case ExportErrorStatus:
        mCloudProjects[index].status = ProjectStatus::Idle;
        mCloudProjects[index].errorStatus = DownloadErrorStatus;

        if ( rawReply->error() == QNetworkReply::NoError )
        {
          QString output = payload.value( QStringLiteral( "output" ) ).toString().split( '\n' ).last();

          mCloudProjects[index].exportStatusString = output.size() > 0
              ? output
              : tr( "Export failed" );
        }
        else
        {
          mCloudProjects[index].exportStatusString = QFieldCloudConnection::errorString( rawReply );
        }

        emit dataChanged( idx, idx, QVector<int>() << StatusRole << ExportStatusRole );
        break;
      case ExportFinishedStatus:
        NetworkReply *exportedFilesReply = mCloudConnection->get( QStringLiteral( "/api/v1/qfield-files/%1/" ).arg( projectId ) );

        mCloudProjects[index].exportStatusString = QString();
        emit dataChanged( idx, idx, QVector<int>() << ExportStatusRole );

        connect( exportedFilesReply, &NetworkReply::finished, exportedFilesReply, [ = ]()
        {
          QNetworkReply *exportedFilesRawReply = exportedFilesReply->reply();
          if ( exportedFilesRawReply->error() != QNetworkReply::NoError )
          {
            mCloudProjects[index].status = ProjectStatus::Idle;
            mCloudProjects[index].errorStatus = DownloadErrorStatus;

            if ( exportedFilesRawReply->error() == QNetworkReply::NoError )
              mCloudProjects[index].exportStatusString = payload.value( QStringLiteral( "output" ) ).toString().split( '\n' ).last();
            else
              mCloudProjects[index].exportStatusString = QFieldCloudConnection::errorString( exportedFilesRawReply );

            emit dataChanged( idx, idx, QVector<int>() << StatusRole << ExportStatusRole << DownloadErrorStatus );
            return;
          }

          const QJsonObject exportedFilesPayload = QJsonDocument::fromJson( exportedFilesRawReply->readAll() ).object();

          const QJsonArray files = exportedFilesPayload.value( QStringLiteral( "files" ) ).toArray();
          for ( const QJsonValue file : files )
          {
            QJsonObject fileObject = file.toObject();
            QString fileName = fileObject.value( QStringLiteral( "name" ) ).toString();
            QString projectFileName = QStringLiteral( "%1/%2/%3/%4" ).arg( QFieldCloudUtils::localCloudDirectory(), mUsername, projectId, fileName );
            QString cloudChecksum = fileObject.value( QStringLiteral( "sha256" ) ).toString();
            QString localChecksum = FileUtils::fileChecksum( projectFileName, QCryptographicHash::Sha256 ).toHex();

            if ( cloudChecksum == localChecksum )
              continue;

            int fileSize = fileObject.value( QStringLiteral( "size" ) ).toInt();

            mCloudProjects[index].downloadFileTransfers.insert( fileName, FileTransfer( fileName, fileSize ) );
            mCloudProjects[index].downloadBytesTotal += std::max( fileSize, 0 );
          }
          projectDownloadFiles( projectId );
        } );
        break;
    }
  } );
}

void QFieldCloudProjectsModel::projectDownloadFiles( const QString &projectId )
{
  if ( !mCloudConnection )
    return;

  const int index = findProject( projectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return;

  const QStringList fileNames = mCloudProjects[index].downloadFileTransfers.keys();

  // why call download project files, if there are no project files?
  if ( fileNames.isEmpty() )
  {
    QModelIndex idx = createIndex( index, 0 );

    mCloudProjects[index].status = ProjectStatus::Idle;
    mCloudProjects[index].downloadProgress = 1;

    emit dataChanged( idx, idx,  QVector<int>() << StatusRole << DownloadProgressRole );
    emit warning( QStringLiteral( "Project empty" ) );

    return;
  }

  for ( const QString &fileName : fileNames )
  {
    NetworkReply *reply = downloadFile( projectId, fileName );
    QTemporaryFile *file = new QTemporaryFile( reply );

    file->setAutoRemove( false );

    if ( ! file->open() )
    {
      QgsMessageLog::logMessage( QStringLiteral( "Failed to open temporary file for \"%1\", reason:\n%2" )
                                 .arg( fileName )
                                 .arg( file->errorString() ) );

      mCloudProjects[index].downloadFilesFailed++;

      emit projectDownloaded( projectId, true, mCloudProjects[index].name );

      return;
    }

    mCloudProjects[index].downloadFileTransfers[fileName].tmpFile = file->fileName();
    mCloudProjects[index].downloadFileTransfers[fileName].networkReply = reply;

    connect( reply, &NetworkReply::downloadProgress, this, [ = ]( int bytesReceived, int bytesTotal )
    {
      Q_UNUSED( bytesTotal );

      // it means the NetworkReply has failed and retried
      mCloudProjects[index].downloadBytesReceived -= mCloudProjects[index].downloadFileTransfers[fileName].bytesTransferred;
      mCloudProjects[index].downloadBytesReceived += bytesReceived;
      mCloudProjects[index].downloadFileTransfers[fileName].bytesTransferred = bytesReceived;
      mCloudProjects[index].downloadProgress = std::clamp( ( static_cast<double>( mCloudProjects[index].downloadBytesReceived ) / std::max( mCloudProjects[index].downloadBytesTotal, 1 ) ), 0., 1. );

      QModelIndex idx = createIndex( index, 0 );

      emit dataChanged( idx, idx,  QVector<int>() << DownloadProgressRole );
    } );

    connect( reply, &NetworkReply::finished, this, [ = ]()
    {
      QVector<int> rolesChanged;
      QNetworkReply *rawReply = reply->reply();

      Q_ASSERT( reply->isFinished() );
      Q_ASSERT( reply );

      mCloudProjects[index].downloadFilesFinished++;

      bool hasError = false;
      QString errorMessage;

      if ( rawReply->error() != QNetworkReply::NoError )
      {
        hasError = true;
        errorMessage = QFieldCloudConnection::errorString( rawReply );
      }

      if ( ! hasError && ! file->write( rawReply->readAll() ) )
      {
        hasError = true;
        errorMessage = file->errorString();
      }

      QgsMessageLog::logMessage( tr( "Failed to write downloaded file stored at \"%1\", reason:\n%2" ).arg( fileName ).arg( errorMessage ) );

      if ( hasError )
      {
        QString trimmedErrorMessage = errorMessage.size() > 100
                                      ? ( errorMessage.left( 100 ) + tr( " (see more in the QField error log)…" ) )
                                      : errorMessage;
        mCloudProjects[index].downloadFilesFailed++;
        mCloudProjects[index].errorStatus = DownloadErrorStatus;
        mCloudProjects[index].exportStatusString = tr( "Failed to write downloaded file stored at \"%1\", reason:\n%2" ).arg( fileName ).arg( trimmedErrorMessage );

        rolesChanged << StatusRole << LocalPathRole << CheckoutRole << ErrorStringRole;

        emit projectDownloaded( projectId, true, mCloudProjects[index].name );
      }

      if ( mCloudProjects[index].downloadFilesFinished == fileNames.count() )
      {
        if ( ! hasError )
        {
          const QStringList unprefixedGpkgFileNames = filterGpkgFileNames( mCloudProjects[index].downloadFileTransfers.keys() );
          const QStringList gpkgFileNames = projectFileNames( mProject->homePath(), unprefixedGpkgFileNames );
          QString projectFileName = mProject->fileName();
          mProject->setFileName( QString() );

          for ( const QString &fileName : gpkgFileNames )
            mGpkgFlusher->stop( fileName );

          // move the files from their temporary location to their permanent one
          if ( ! projectMoveDownloadedFilesToPermanentStorage( projectId ) )
            mCloudProjects[index].errorStatus = DownloadErrorStatus;

          deleteGpkgShmAndWal( gpkgFileNames );

          for ( const QString &fileName : gpkgFileNames )
            mGpkgFlusher->start( fileName );

          mProject->setFileName( projectFileName );

          mCloudProjects[index].checkout = ProjectCheckout::LocalAndRemoteCheckout;
          mCloudProjects[index].localPath = QFieldCloudUtils::localProjectFilePath( mUsername, projectId );
          mCloudProjects[index].lastLocalExport = QDateTime::currentDateTimeUtc().toString( Qt::ISODate );

          projectSetSetting( projectId, QStringLiteral( "lastLocalExport" ), mCloudProjects[index].lastLocalExport );

          emit projectDownloaded( projectId, false, mCloudProjects[index].name );
        }

        for ( const QString &fileName : fileNames )
        {
          mCloudProjects[index].downloadFileTransfers[fileName].networkReply->deleteLater();
        }

        mCloudProjects[index].status = ProjectStatus::Idle;
      }


      QModelIndex idx = createIndex( index, 0 );
      rolesChanged << StatusRole << LocalPathRole << CheckoutRole;
      emit dataChanged( idx, idx, rolesChanged );
    } );
  }
}


bool QFieldCloudProjectsModel::projectMoveDownloadedFilesToPermanentStorage( const QString &projectId )
{
  if ( !mCloudConnection )
    return false;

  const int index = findProject( projectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return false;

  bool hasError = false;
  const QStringList fileNames = mCloudProjects[index].downloadFileTransfers.keys();

  for ( const QString &fileName : fileNames )
  {
    QFileInfo fileInfo( fileName );
    QFile file( mCloudProjects[index].downloadFileTransfers[fileName].tmpFile );
    QDir dir( QStringLiteral( "%1/%2/%3/%4" ).arg( QFieldCloudUtils::localCloudDirectory(), mUsername, projectId, fileInfo.path() ) );

    if ( ! hasError && ! dir.exists() && ! dir.mkpath( QStringLiteral( "." ) ) )
    {
      hasError = true;
      QgsMessageLog::logMessage( QStringLiteral( "Failed to create directory at \"%1\"" ).arg( dir.path() ) );
    }

    const QString destinationFileName( dir.filePath( fileInfo.fileName() ) );

    // if the file already exists, we need to delete it first, as QT does not support overwriting
    // NOTE: it is possible that someone creates the file in the meantime between this and the next if statement
    if ( ! hasError && QFile::exists( destinationFileName ) && ! file.remove( destinationFileName ) )
    {
      hasError = true;
      QgsMessageLog::logMessage( QStringLiteral( "Failed to remove file before overwriting stored at \"%1\", reason:\n%2" ).arg( fileName ).arg( file.errorString() ) );
    }

    if ( ! hasError && ! file.copy( destinationFileName ) )
    {
      hasError = true;
      QgsMessageLog::logMessage( QStringLiteral( "Failed to write downloaded file stored at \"%1\", reason:\n%2" ).arg( fileName ).arg( file.errorString() ) );

      if ( ! QFile::remove( dir.filePath( fileName ) ) )
        QgsMessageLog::logMessage( QStringLiteral( "Failed to remove partly overwritten file stored at \"%1\"" ).arg( fileName ) );
    }

    if ( ! file.remove() )
      QgsMessageLog::logMessage( QStringLiteral( "Failed to remove temporary file \"%1\"" ).arg( fileName ) );
  }

  return ! hasError;
}


void QFieldCloudProjectsModel::uploadProject( const QString &projectId, const bool shouldDownloadUpdates )
{
  if ( !mCloudConnection )
    return;

  int index = findProject( projectId );

  if ( index == -1 )
    return;

  if ( !( mCloudProjects[index].status == ProjectStatus::Idle ) )
    return;

  DeltaFileWrapper *deltaFileWrapper = mLayerObserver->deltaFileWrapper();

  if ( shouldDownloadUpdates && deltaFileWrapper->count() == 0 )
  {
    downloadProject( projectId );
    return;
  }

  if ( !( mCloudProjects[index].modification & LocalModification ) )
    return;

  if ( ! mLayerObserver->deltaFileWrapper()->toFile() )
    return;

  QModelIndex idx = createIndex( index, 0 );

  if ( deltaFileWrapper->hasError() )
  {
    QgsMessageLog::logMessage( QStringLiteral( "The delta file has an error: %1" ).arg( deltaFileWrapper->errorString() ) );
    return;
  }

  mCloudProjects[index].status = ProjectStatus::Uploading;
  mCloudProjects[index].deltaFileId = deltaFileWrapper->id();
  mCloudProjects[index].deltaFileUploadStatus = DeltaLocalStatus;
  mCloudProjects[index].deltaFileUploadStatusString = QString();

  mCloudProjects[index].uploadAttachments.empty();
  mCloudProjects[index].uploadAttachmentsFinished = 0;
  mCloudProjects[index].uploadAttachmentsFailed = 0;

  refreshProjectModification( projectId );

  emit dataChanged( idx, idx,  QVector<int>() << StatusRole << UploadAttachmentsProgressRole << UploadDeltaProgressRole << UploadDeltaStatusRole << UploadDeltaStatusStringRole );

  // //////////
  // prepare attachment files to be uploaded
  // //////////
  const QStringList attachmentFileNames = deltaFileWrapper->attachmentFileNames().keys();
  for ( const QString &fileName : attachmentFileNames )
  {
    QFileInfo fileInfo( fileName );

    if ( !fileInfo.exists() )
    {
      QgsMessageLog::logMessage( QStringLiteral( "Attachment file '%1' does not exist" ).arg( fileName ) );
      continue;
    }

    const int fileSize = fileInfo.size();

    // ? should we also check the checksums of the files being uploaded? they are available at deltaFile->attachmentFileNames()->values()

    mCloudProjects[index].uploadAttachments.insert( fileName, FileTransfer( fileName, fileSize ) );
    mCloudProjects[index].uploadAttachmentsBytesTotal += fileSize;
  }

  QString deltaFileToUpload = deltaFileWrapper->toFileForUpload();

  if ( deltaFileToUpload.isEmpty() )
  {
    mCloudProjects[index].status = ProjectStatus::Idle;
    emit dataChanged( idx, idx,  QVector<int>() << StatusRole );
    return;
  }

  // //////////
  // 1) upload the deltas
  // //////////
  NetworkReply *deltasCloudReply = mCloudConnection->post(
                                     QStringLiteral( "/api/v1/deltas/%1/" ).arg( projectId ),
                                     QVariantMap(),
                                     QStringList( {deltaFileToUpload} ) );

  Q_ASSERT( deltasCloudReply );

  connect( deltasCloudReply, &NetworkReply::uploadProgress, this, [ = ]( int bytesSent, int bytesTotal )
  {
    mCloudProjects[index].uploadDeltaProgress = std::clamp( ( static_cast<double>( bytesSent ) / bytesTotal ), 0., 1. );

    emit dataChanged( idx, idx,  QVector<int>() << UploadDeltaProgressRole );
  } );
  connect( deltasCloudReply, &NetworkReply::finished, this, [ = ]()
  {
    QNetworkReply *deltasReply = deltasCloudReply->reply();
    deltasCloudReply->deleteLater();

    Q_ASSERT( deltasCloudReply->isFinished() );
    Q_ASSERT( deltasReply );

    // if there is an error, cannot continue sync
    if ( deltasReply->error() != QNetworkReply::NoError )
    {
      // TODO check why exactly we failed
      // maybe the project does not exist, then create it?
      QgsMessageLog::logMessage( QStringLiteral( "Failed to upload delta file, reason:\n%1\n%2" ).arg( deltasReply->errorString(), QFieldCloudConnection::errorString( deltasReply ) ) );

      mCloudProjects[index].deltaFileUploadStatusString = QFieldCloudConnection::errorString( deltasReply );
      projectCancelUpload( projectId );
      return;
    }

    mCloudProjects[index].uploadDeltaProgress = 1;
    mCloudProjects[index].deltaFileUploadStatus = DeltaPendingStatus;
    mCloudProjects[index].deltaLayersToDownload = deltaFileWrapper->deltaLayerIds();

    emit dataChanged( idx, idx,  QVector<int>() << UploadDeltaProgressRole << UploadDeltaStatusRole );
    emit networkDeltaUploaded( projectId );
  } );


  // //////////
  // 2) delta successfully uploaded, then send attachments
  // //////////
  QObject *networkDeltaUploadedParent = new QObject( this ); // we need this to unsubscribe
  connect( this, &QFieldCloudProjectsModel::networkDeltaUploaded, networkDeltaUploadedParent, [ = ]( const QString & uploadedProjectId )
  {
    if ( projectId != uploadedProjectId )
      return;

    delete networkDeltaUploadedParent;

    // attachments can be uploaded in the background.
    // ? what if an attachment fail to be uploaded?
    projectUploadAttachments( projectId );

    if ( shouldDownloadUpdates )
    {
      projectApplyDeltas( projectId );
    }
    else
    {
      mCloudProjects[index].status = ProjectStatus::Idle;
      mCloudProjects[index].modification ^= LocalModification;

      deltaFileWrapper->reset();
      deltaFileWrapper->resetId();

      if ( ! deltaFileWrapper->toFile() )
        QgsMessageLog::logMessage( QStringLiteral( "Failed to reset delta file after delta push. %1" ).arg( deltaFileWrapper->errorString() ) );

      QModelIndex idx = createIndex( index, 0 );

      emit dataChanged( idx, idx, QVector<int>() << StatusRole );
      emit pushFinished( projectId, false );
    }
  } );


  // //////////
  // 3) new delta status received. Never give up to get a successful status.
  // //////////
  QObject *networkDeltaStatusCheckedParent = new QObject( this ); // we need this to unsubscribe
  connect( this, &QFieldCloudProjectsModel::networkDeltaStatusChecked, networkDeltaStatusCheckedParent, [ = ]( const QString & uploadedProjectId )
  {
    if ( projectId != uploadedProjectId )
      return;

    switch ( mCloudProjects[index].deltaFileUploadStatus )
    {
      case DeltaLocalStatus:
        // delta file should be already sent!!!
        Q_ASSERT( 0 );
        FALLTHROUGH
      case DeltaPendingStatus:
      case DeltaBusyStatus:
        // infinite retry, there should be one day, when we can get the status!
        QTimer::singleShot( sDelayBeforeStatusRetry, [ = ]()
        {
          projectGetDeltaStatus( projectId );
        } );
        break;
      case DeltaErrorStatus:
        delete networkDeltaStatusCheckedParent;
        deltaFileWrapper->resetId();

        if ( ! deltaFileWrapper->toFile() )
          QgsMessageLog::logMessage( QStringLiteral( "Failed update committed delta file." ) );

        projectCancelUpload( projectId );
        return;
      case DeltaConflictStatus:
      case DeltaNotAppliedStatus:
      case DeltaAppliedStatus:
        delete networkDeltaStatusCheckedParent;

        deltaFileWrapper->reset();
        deltaFileWrapper->resetId();

        if ( ! deltaFileWrapper->toFile() )
          QgsMessageLog::logMessage( QStringLiteral( "Failed to reset delta file. %1" ).arg( deltaFileWrapper->errorString() ) );

        mCloudProjects[index].status = ProjectStatus::Idle;
        mCloudProjects[index].modification ^= LocalModification;
        mCloudProjects[index].modification |= RemoteModification;
        mCloudProjects[index].lastLocalPushDeltas = QDateTime::currentDateTimeUtc().toString( Qt::ISODate );

        projectSetSetting( projectId, QStringLiteral( "lastLocalPushDeltas" ), mCloudProjects[index].lastLocalPushDeltas );

        emit dataChanged( idx, idx, QVector<int>() << ModificationRole );

        // download the updated files, so the files are for sure the same on the client and on the server
        if ( shouldDownloadUpdates )
        {
          downloadProject( projectId );
        }
        else
        {
          QModelIndex idx = createIndex( index, 0 );
          emit dataChanged( idx, idx, QVector<int>() << StatusRole );

          emit pushFinished( projectId, false );
        }
    }
  } );


// this code is no longer needed, as we do not upload or download files selectively
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  // //////////
//  // 4) project downloaded, if all done, then reload the project and sync done!
//  // //////////
//  connect( this, &QFieldCloudProjectsModel::networkAllLayersDownloaded, this, [ = ]( const QString & callerProjectId )
//  {
//    if ( projectId != callerProjectId )
//      return;

//    // wait until all layers are downloaded
//    if ( mCloudProjects[index].downloadLayersFinished < mCloudProjects[index].deltaLayersToDownload.size() )
//      return;

//    // there are some files that failed to download
//    if ( mCloudProjects[index].downloadLayersFailed > 0 )
//    {
//      // TODO translate this message
//      mCloudProjects[index].deltaFileUploadStatusString = QStringLiteral( "Failed to retrieve some of the updated layers, but changes are committed on the server. "
//                            "Try to reload the project from the cloud." );
//      projectCancelUpload( projectId );

//      return;
//    }

//    QgsProject::instance()->reloadAllLayers();

//    emit dataChanged( idx, idx, QVector<int>() << StatusRole );
//    emit syncFinished( projectId, false );
//  } );
}


void QFieldCloudProjectsModel::projectApplyDeltas( const QString &projectId )
{
  const int index = findProject( projectId );

  Q_ASSERT( index >= 0 && index < mCloudProjects.size() );

  QModelIndex idx = createIndex( index, 0 );
  NetworkReply *reply = mCloudConnection->post( QStringLiteral( "/api/v1/deltas/apply/%1/" ).arg( mCloudProjects[index].id ) );

  connect( reply, &NetworkReply::finished, this, [ = ]()
  {
    QNetworkReply *rawReply = reply->reply();
    reply->deleteLater();

    Q_ASSERT( reply->isFinished() );
    Q_ASSERT( rawReply );

    if ( rawReply->error() != QNetworkReply::NoError )
    {
      mCloudProjects[index].status = ProjectStatus::Idle;
      mCloudProjects[index].deltaFileUploadStatus = DeltaErrorStatus;
      mCloudProjects[index].deltaFileUploadStatusString = QFieldCloudConnection::errorString( rawReply );

      emit dataChanged( idx, idx,  QVector<int>() << UploadDeltaStatusRole << UploadDeltaStatusStringRole );
      emit networkDeltaStatusChecked( projectId );
      return;
    }

    projectGetDeltaStatus( projectId );
  } );
}


void QFieldCloudProjectsModel::projectGetDeltaStatus( const QString &projectId )
{
  const int index = findProject( projectId );

  Q_ASSERT( index >= 0 && index < mCloudProjects.size() );

  QModelIndex idx = createIndex( index, 0 );
  NetworkReply *deltaStatusReply = mCloudConnection->get( QStringLiteral( "/api/v1/deltas/%1/%2/" ).arg( mCloudProjects[index].id, mCloudProjects[index].deltaFileId ) );

  mCloudProjects[index].deltaFileUploadStatusString = QString();

  connect( deltaStatusReply, &NetworkReply::finished, this, [ = ]()
  {
    QNetworkReply *rawReply = deltaStatusReply->reply();
    deltaStatusReply->deleteLater();

    Q_ASSERT( deltaStatusReply->isFinished() );
    Q_ASSERT( rawReply );

    if ( rawReply->error() != QNetworkReply::NoError )
    {
      mCloudProjects[index].deltaFileUploadStatus = DeltaErrorStatus;
      // TODO this is oversimplification. e.g. 404 error is when the requested delta file id is not existant
      mCloudProjects[index].deltaFileUploadStatusString = QFieldCloudConnection::errorString( rawReply );

      emit dataChanged( idx, idx,  QVector<int>() << UploadDeltaStatusRole << UploadDeltaStatusStringRole );
      emit networkDeltaStatusChecked( projectId );

      return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson( rawReply->readAll() );

    mDeltaStatusListModel = std::make_unique<DeltaStatusListModel>( doc );

    if ( ! mDeltaStatusListModel->isValid() )
    {
      mCloudProjects[index].deltaFileUploadStatus = DeltaErrorStatus;
      mCloudProjects[index].deltaFileUploadStatusString = mDeltaStatusListModel->errorString();
      emit dataChanged( idx, idx,  QVector<int>() << UploadDeltaStatusRole << UploadDeltaStatusStringRole );
      emit networkDeltaStatusChecked( projectId );
      return;
    }

    mCloudProjects[index].deltaFileUploadStatusString = QString();

    if ( ! mDeltaStatusListModel->allHaveFinalStatus() )
    {
      mCloudProjects[index].deltaFileUploadStatus = DeltaPendingStatus;
      emit dataChanged( idx, idx,  QVector<int>() << UploadDeltaStatusRole << UploadDeltaStatusStringRole );
      emit networkDeltaStatusChecked( projectId );
      return;
    }

    // probably reseting the uniq ptr is not needed here, but it will be clear when the Delta Status UI is created
    mDeltaStatusListModel.reset();

    mCloudProjects[index].deltaFileUploadStatus = DeltaAppliedStatus;

    emit dataChanged( idx, idx,  QVector<int>() << UploadDeltaStatusRole << UploadDeltaStatusStringRole );
    emit networkDeltaStatusChecked( projectId );
  } );
}

void QFieldCloudProjectsModel::projectUploadAttachments( const QString &projectId )
{
  const int index = findProject( projectId );
  QModelIndex idx = createIndex( index, 0 );

  Q_ASSERT( index >= 0 && index < mCloudProjects.size() );

  // start uploading the attachments
  const QStringList attachmentFileNames;
  for ( const QString &fileName : attachmentFileNames )
  {
    NetworkReply *attachmentCloudReply = uploadFile( mCloudProjects[index].id, fileName );

    mCloudProjects[index].uploadAttachments[fileName].networkReply = attachmentCloudReply;

    connect( attachmentCloudReply, &NetworkReply::uploadProgress, this, [ = ]( int bytesSent, int bytesTotal )
    {
      Q_UNUSED( bytesTotal );
      mCloudProjects[index].uploadAttachments[fileName].bytesTransferred = bytesSent;
      emit dataChanged( idx, idx, QVector<int>() << UploadAttachmentsProgressRole );
    } );

    connect( attachmentCloudReply, &NetworkReply::finished, this, [ = ]()
    {
      QNetworkReply *attachmentReply = attachmentCloudReply->reply();

      Q_ASSERT( attachmentCloudReply->isFinished() );
      Q_ASSERT( attachmentReply );

      // if there is an error, don't panic, we continue uploading. The files may be later manually synced.
      if ( attachmentReply->error() != QNetworkReply::NoError )
      {
        mCloudProjects[index].uploadAttachmentsFailed++;
        QgsMessageLog::logMessage( tr( "Failed to upload attachment stored at \"%1\", reason:\n%2" )
                                   .arg( fileName )
                                   .arg( QFieldCloudConnection::errorString( attachmentReply ) ) );
      }

      mCloudProjects[index].uploadAttachmentsFinished++;
      mCloudProjects[index].uploadAttachmentsProgress = mCloudProjects[index].uploadAttachmentsFinished / attachmentFileNames.size();
      emit dataChanged( idx, idx, QVector<int>() << UploadAttachmentsProgressRole );
    } );
  }
}

void QFieldCloudProjectsModel::projectCancelUpload( const QString &projectId )
{
  if ( ! mCloudConnection )
    return;

  int index = findProject( projectId );

  if ( index == -1 )
    return;

  const QStringList attachmentFileNames = mCloudProjects[index].uploadAttachments.keys();
  for ( const QString &attachmentFileName : attachmentFileNames )
  {
    NetworkReply *attachmentReply = mCloudProjects[index].uploadAttachments[attachmentFileName].networkReply;

    // the replies might be already disposed
    if ( ! attachmentReply )
      continue;

    // the replies might be already finished
    if ( attachmentReply->isFinished() )
      continue;

    attachmentReply->abort();
  }

  mCloudProjects[index].status = ProjectStatus::Idle;
  mCloudProjects[index].errorStatus = UploadErrorStatus;

  QModelIndex idx = createIndex( index, 0 );

  emit dataChanged( idx, idx, QVector<int>() << StatusRole << ErrorStatusRole );
  emit pushFinished( projectId, true, mCloudProjects[index].deltaFileUploadStatusString );

  return;
}

void QFieldCloudProjectsModel::connectionStatusChanged()
{
  refreshProjectsList();
}

void QFieldCloudProjectsModel::layerObserverLayerEdited( const QString &layerId )
{
  Q_UNUSED( layerId );

  const int index = findProject( mCurrentProjectId );

  if ( index == -1 || index >= mCloudProjects.size() )
  {
    QgsMessageLog::logMessage( QStringLiteral( "Layer observer triggered `isDirtyChanged` signal incorrectly" ) );
    return;
  }

  beginResetModel();

  const DeltaFileWrapper *deltaFileWrapper = mLayerObserver->deltaFileWrapper();

  Q_ASSERT( deltaFileWrapper );

  if ( deltaFileWrapper->count() > 0 )
    mCloudProjects[index].modification |= LocalModification;
  else if ( mCloudProjects[index].modification & LocalModification )
    mCloudProjects[index].modification ^= LocalModification;

  endResetModel();
}

void QFieldCloudProjectsModel::projectListReceived()
{
  NetworkReply *reply = qobject_cast<NetworkReply *>( sender() );
  QNetworkReply *rawReply = reply->reply();

  Q_ASSERT( rawReply );

  if ( rawReply->error() != QNetworkReply::NoError )
  {
    emit warning( QFieldCloudConnection::errorString( rawReply ) );
    return;
  }

  QByteArray response = rawReply->readAll();

  QJsonDocument doc = QJsonDocument::fromJson( response );
  QJsonArray projects = doc.array();
  reload( projects );
}

NetworkReply *QFieldCloudProjectsModel::downloadFile( const QString &projectId, const QString &fileName )
{
  return mCloudConnection->get( QStringLiteral( "/api/v1/qfield-files/%1/%2/" ).arg( projectId, fileName ) );
}

NetworkReply *QFieldCloudProjectsModel::uploadFile( const QString &projectId, const QString &fileName )
{
  return mCloudConnection->post( QStringLiteral( "/api/v1/files/%1/%2/" ).arg( projectId, fileName ), QVariantMap(), QStringList( {fileName} ) );
}

QHash<int, QByteArray> QFieldCloudProjectsModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[IdRole] = "Id";
  roles[OwnerRole] = "Owner";
  roles[NameRole] = "Name";
  roles[DescriptionRole] = "Description";
  roles[ModificationRole] = "Modification";
  roles[CheckoutRole] = "Checkout";
  roles[StatusRole] = "Status";
  roles[ErrorStatusRole] = "ErrorStatus";
  roles[ErrorStringRole] = "ErrorString";
  roles[DownloadProgressRole] = "DownloadProgress";
  roles[ExportStatusRole] = "ExportStatus";
  roles[UploadAttachmentsProgressRole] = "UploadAttachmentsProgress";
  roles[UploadDeltaProgressRole] = "UploadDeltaProgress";
  roles[UploadDeltaStatusRole] = "UploadDeltaStatus";
  roles[UploadDeltaStatusStringRole] = "UploadDeltaStatusString";
  roles[LocalDeltasCountRole] = "LocalDeltasCount";
  roles[LocalPathRole] = "LocalPath";
  roles[CanSyncRole] = "CanSync";
  roles[LastLocalExport] = "LastLocalExport";
  roles[LastLocalPushDeltas] = "LastLocalPushDeltas";
  roles[CollaboratorRole] = "CollaboratorRole";

  return roles;
}

void QFieldCloudProjectsModel::reload( const QJsonArray &remoteProjects )
{
  beginResetModel();
  mCloudProjects.clear();

  QgsProject *qgisProject = QgsProject::instance();

  for ( const auto project : remoteProjects )
  {
    QVariantHash projectDetails = project.toObject().toVariantHash();
    CloudProject cloudProject( projectDetails.value( "id" ).toString(),
                               projectDetails.value( "owner" ).toString(),
                               projectDetails.value( "name" ).toString(),
                               projectDetails.value( "description" ).toString(),
                               projectDetails.value( "collaborators__role" ).toString(),
                               QString(),
                               RemoteCheckout,
                               ProjectStatus::Idle );

    const QString projectPrefix = QStringLiteral( "QFieldCloud/projects/%1" ).arg( cloudProject.id );
    projectSetSetting( cloudProject.id, QStringLiteral( "owner" ), cloudProject.owner );
    projectSetSetting( cloudProject.id, QStringLiteral( "name" ), cloudProject.name );
    projectSetSetting( cloudProject.id, QStringLiteral( "description" ), cloudProject.description );
    projectSetSetting( cloudProject.id, QStringLiteral( "updatedAt" ), cloudProject.updatedAt );
    projectSetSetting( cloudProject.id, QStringLiteral( "collaboratorRole" ), cloudProject.collaboratorRole );

    if ( !mUsername.isEmpty() )
    {
      QDir localPath( QStringLiteral( "%1/%2/%3" ).arg( QFieldCloudUtils::localCloudDirectory(), mUsername, cloudProject.id ) );
      if ( localPath.exists() )
      {
        cloudProject.checkout = LocalAndRemoteCheckout;
        cloudProject.localPath = QFieldCloudUtils::localProjectFilePath( mUsername, cloudProject.id );
        cloudProject.deltasCount = DeltaFileWrapper( qgisProject, QStringLiteral( "%1/deltafile.json" ).arg( localPath.absolutePath() ) ).count();
        cloudProject.lastLocalExport = projectSetting( cloudProject.id, QStringLiteral( "lastLocalExport" ) ).toString();
        cloudProject.lastLocalPushDeltas = projectSetting( cloudProject.id, QStringLiteral( "lastLocalPushDeltas" ) ).toString();
      }
    }

    mCloudProjects << cloudProject;
  }

  if ( !mUsername.isEmpty() )
  {
    QDirIterator projectDirs( QStringLiteral( "%1/%2" ).arg( QFieldCloudUtils::localCloudDirectory(), mUsername ), QDir::Dirs | QDir::NoDotAndDotDot );
    while ( projectDirs.hasNext() )
    {
      projectDirs.next();

      const QString projectId = projectDirs.fileName();
      int index = findProject( projectId );
      if ( index != -1 )
        continue;

      const QString projectPrefix = QStringLiteral( "QFieldCloud/projects/%1" ).arg( projectId );
      if ( !QSettings().contains( QStringLiteral( "%1/name" ).arg( projectPrefix ) ) )
        continue;

      const QString owner = projectSetting( projectId, QStringLiteral( "owner" ) ).toString();
      const QString name = projectSetting( projectId, QStringLiteral( "name" ) ).toString();
      const QString description = projectSetting( projectId, QStringLiteral( "description" ) ).toString();
      const QString updatedAt = projectSetting( projectId, QStringLiteral( "updatedAt" ) ).toString();
      const QString collaboratorRole = projectSetting( projectId, QStringLiteral( "collaboratorRole" ) ).toString();

      CloudProject cloudProject( projectId, owner, name, description, collaboratorRole, QString(), LocalCheckout, ProjectStatus::Idle );
      QDir localPath( QStringLiteral( "%1/%2/%3" ).arg( QFieldCloudUtils::localCloudDirectory(), mUsername, cloudProject.id ) );
      cloudProject.localPath = QFieldCloudUtils::localProjectFilePath( mUsername, cloudProject.id );
      cloudProject.deltasCount = DeltaFileWrapper( qgisProject, QStringLiteral( "%1/deltafile.json" ).arg( localPath.absolutePath() ) ).count();
      cloudProject.lastLocalExport = projectSetting( projectId, QStringLiteral( "lastLocalExport" ) ).toString();
      cloudProject.lastLocalPushDeltas = projectSetting( projectId, QStringLiteral( "lastLocalPushDeltas" ) ).toString();

      mCloudProjects << cloudProject;

      Q_ASSERT( projectId == cloudProject.id );
    }
  }

  endResetModel();
}

QVariantMap QFieldCloudProjectsModel::getEntry( int row )
{
  QHash<int, QByteArray> names = roleNames();
  QHashIterator<int, QByteArray> i( names );
  QVariantMap res;
  while ( i.hasNext() )
  {
    i.next();
    QModelIndex idx = index( row, 0 );
    QVariant data = idx.data( i.key() );
    res[i.value()] = data;
  }
  return res;
}

int QFieldCloudProjectsModel::rowCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() )
    return mCloudProjects.size();
  else
    return 0;
}

QVariant QFieldCloudProjectsModel::data( const QModelIndex &index, int role ) const
{
  if ( index.row() >= mCloudProjects.size() || index.row() < 0 )
    return QVariant();

  switch ( static_cast<ColumnRole>( role ) )
  {
    case IdRole:
      return mCloudProjects.at( index.row() ).id;
    case OwnerRole:
      return mCloudProjects.at( index.row() ).owner;
    case NameRole:
      return mCloudProjects.at( index.row() ).name;
    case DescriptionRole:
      return mCloudProjects.at( index.row() ).description;
    case ModificationRole:
      return static_cast<int>( mCloudProjects.at( index.row() ).modification );
    case CheckoutRole:
      return static_cast<int>( mCloudProjects.at( index.row() ).checkout );
    case StatusRole:
      return static_cast<int>( mCloudProjects.at( index.row() ).status );
    case ErrorStatusRole:
      return static_cast<int>( mCloudProjects.at( index.row() ).errorStatus );
    case ErrorStringRole:
      return mCloudProjects.at( index.row() ).errorStatus == DownloadErrorStatus
             ? mCloudProjects.at( index.row() ).exportStatusString
             : mCloudProjects.at( index.row() ).errorStatus == UploadErrorStatus
             ? mCloudProjects.at( index.row() ).deltaFileUploadStatusString
             : QString();
    case ExportStatusRole:
      return mCloudProjects.at( index.row() ).exportStatus;
    case DownloadProgressRole:
      return mCloudProjects.at( index.row() ).downloadProgress;
    case UploadAttachmentsProgressRole:
      // when we start syncing also the photos, it would make sense to go there
      return mCloudProjects.at( index.row() ).uploadAttachmentsProgress;
    case UploadDeltaProgressRole:
      return mCloudProjects.at( index.row() ).uploadDeltaProgress;
    case UploadDeltaStatusRole:
      return mCloudProjects.at( index.row() ).deltaFileUploadStatus;
    case UploadDeltaStatusStringRole:
      return mCloudProjects.at( index.row() ).deltaFileUploadStatusString;
    case LocalDeltasCountRole:
      return mCloudProjects.at( index.row() ).deltasCount;
    case LocalPathRole:
      return mCloudProjects.at( index.row() ).localPath;
    case CanSyncRole:
      return canSyncProject( mCloudProjects.at( index.row() ).id );
    case LastLocalExport:
      return mCloudProjects.at( index.row() ).lastLocalExport;
    case LastLocalPushDeltas:
      return mCloudProjects.at( index.row() ).lastLocalPushDeltas;
    case CollaboratorRole:
      return mCloudProjects.at( index.row() ).collaboratorRole;
  }

  return QVariant();
}

bool QFieldCloudProjectsModel::revertLocalChangesFromCurrentProject()
{
  const int index = findProject( mCurrentProjectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return false;

  DeltaFileWrapper *deltaFileWrapper = mLayerObserver->deltaFileWrapper();

  if ( ! deltaFileWrapper->toFile() )
    return false;

  if ( ! deltaFileWrapper->applyReversed() )
  {
    QgsMessageLog::logMessage( QStringLiteral( "Failed to apply reversed" ) );
    return false;
  }

  mCloudProjects[index].modification ^= LocalModification;

  deltaFileWrapper->reset();
  deltaFileWrapper->resetId();

  if ( ! deltaFileWrapper->toFile() )
    return false;

  return true;
}

bool QFieldCloudProjectsModel::discardLocalChangesFromCurrentProject()
{
  const int index = findProject( mCurrentProjectId );

  if ( index == -1 || index >= mCloudProjects.size() )
    return false;

  DeltaFileWrapper *deltaFileWrapper = mLayerObserver->deltaFileWrapper();

  if ( ! deltaFileWrapper->toFile() )
    QgsMessageLog::logMessage( QStringLiteral( "Failed to write deltas." ) );

  mCloudProjects[index].modification ^= LocalModification;

  deltaFileWrapper->reset();
  deltaFileWrapper->resetId();

  if ( ! deltaFileWrapper->toFile() )
    return false;

  return true;
}

void QFieldCloudProjectsModel::setGpkgFlusher( QgsGpkgFlusher *flusher )
{
  if ( mGpkgFlusher == flusher )
    return;

  mGpkgFlusher = flusher;

  emit gpkgFlusherChanged();
}

bool QFieldCloudProjectsModel::deleteGpkgShmAndWal( const QStringList &gpkgFileNames )
{
  bool isSuccess = true;

  for ( const QString &fileName : gpkgFileNames )
  {
    QFile shmFile( QStringLiteral( "%1-shm" ).arg( fileName ) );
    if ( shmFile.exists() )
    {
      if ( ! shmFile.remove() )
      {
        QgsMessageLog::logMessage( QStringLiteral( "Failed to remove -shm file '%1' " ).arg( shmFile.fileName() ) );
        isSuccess = false;
      }
    }

    QFile walFile( QStringLiteral( "%1-wal" ).arg( fileName ) );

    if ( walFile.exists() )
    {
      if ( ! walFile.remove() )
      {
        QgsMessageLog::logMessage( QStringLiteral( "Failed to remove -wal file '%1' " ).arg( walFile.fileName() ) );
        isSuccess = false;
      }
    }
  }

  return isSuccess;
}

QStringList QFieldCloudProjectsModel::filterGpkgFileNames( const QStringList &fileNames ) const
{
  QStringList gpkgFileNames;

  for ( const QString &fileName : fileNames )
  {
    if ( fileName.endsWith( QStringLiteral( ".gpkg" ) ) )
    {
      gpkgFileNames.append( fileName );
    }
  }

  return gpkgFileNames;
}

QStringList QFieldCloudProjectsModel::projectFileNames( const QString &projectPath, const QStringList &fileNames ) const
{
  QStringList prefixedFileNames;

  for ( const QString &fileName : fileNames )
  {
    prefixedFileNames.append( QStringLiteral( "%1/%2" ).arg( projectPath, fileName ) );
  }

  return prefixedFileNames;
}

void QFieldCloudProjectsModel::projectSetSetting( const QString &projectId, const QString &setting, const QVariant &value )
{
  const QString projectPrefix = QStringLiteral( "QFieldCloud/projects/%1" ).arg( projectId );
  return QSettings().setValue( QStringLiteral( "%1/%2" ).arg( projectPrefix, setting ), value );
}

QVariant QFieldCloudProjectsModel::projectSetting( const QString &projectId, const QString &setting, const QVariant &defaultValue )
{
  const QString projectPrefix = QStringLiteral( "QFieldCloud/projects/%1" ).arg( projectId );
  return QSettings().value( QStringLiteral( "%1/%2" ).arg( projectPrefix, setting ), defaultValue );
}
