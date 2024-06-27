/***************************************************************************
                        test_qml_editorwidgets.cpp
                        --------------------------
  begin                : Jul 2021
  copyright            : (C) 2021 by Mathieu Pellerin
  email                : mathieu@opengis.ch
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "attributeformmodel.h"
#include "barcodedecoder.h"
#include "coordinatereferencesystemutils.h"
#include "digitizinglogger.h"
#include "nearfieldreader.h"
#include "platformutilities.h"
#include "positioning.h"
#include "qfield_qml_init.h"
#include "qgsquickcoordinatetransformer.h"
#include "rubberbandmodel.h"
#include "settings.h"
#include "stringutils.h"
#include "submodel.h"
#include "valuemapmodel.h"

#if QT_VERSION >= QT_VERSION_CHECK( 6, 5, 0 )
#include "permissions.h"
#endif

#include <QProcess>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlFileSelector>
#include <QtQuickTest>
#include <qgis.h>
#include <qgsapplication.h>
#include <qgscoordinatereferencesystem.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>
#include <qgslocatormodel.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerproxymodel.h>
#include <qgsmapthemecollection.h>
#include <qgspoint.h>
#include <qgsproject.h>
#include <qgsprojectdisplaysettings.h>
#include <qgsquickcoordinatetransformer.h>
#include <qgsquickelevationprofilecanvas.h>
#include <qgsquickmapcanvasmap.h>
#include <qgsquickmapsettings.h>
#include <qgsquickmaptransform.h>
#include <qgsrelationmanager.h>
#include <qgssnappingutils.h>
#include <qgsvectorlayer.h>
#include <qgsvectorlayereditbuffer.h>

#define REGISTER_SINGLETON( uri, _class, name ) qmlRegisterSingletonType<_class>( uri, 1, 0, name, []( QQmlEngine *engine, QJSEngine *scriptEngine ) -> QObject * { Q_UNUSED(engine); Q_UNUSED(scriptEngine); return new _class(); } )

class NmeaServer : public QObject
{
    Q_OBJECT

  public:
    enum Protocol
    {
      Udp,
      Tcp,
    };

    NmeaServer( const QString &filename,
                Protocol protocol,
                int port )
      : QObject()
      , mFilename( filename )
      , mProtocol( protocol )
      , mPort( port )
    {
    }

    void start( const QString &nmeaServerLocation )
    {
      // start a UDP server streaming NMEA strings (used in tst_positioning.qml)
      if ( nmeaServerLocation.isEmpty() )
        return;

      QString type;
      switch ( mProtocol )
      {
        case Protocol::Tcp:
          type = "tcp";
          break;
        case Protocol::Udp:
          type = "udp";
          break;
      }

      mServerProcess.setProgram( QStringLiteral( "python3" ) );
      mServerProcess.setArguments( QStringList() << QStringLiteral( "%1/nmeaserver.py" ).arg( nmeaServerLocation )
                                                 << QStringLiteral( "--type" )
                                                 << type
                                                 << QStringLiteral( "--port" )
                                                 << QString::number( mPort )
                                                 << QStringLiteral( "%1/%2" ).arg( nmeaServerLocation, mFilename ) );
      mServerProcess.start();
    }

    void kill()
    {
      // kill the server process
      mServerProcess.kill();

      if ( !mServerProcess.waitForFinished() )
        qDebug() << "Waiting for processes to terminate timed out";
    }

    QString dataDir() const { return mDataDir; }

  private:
    QString mDataDir;

    QProcess mServerProcess;

    QString mFilename;
    Protocol mProtocol;
    int mPort;
};


class Setup : public QObject
{
    Q_OBJECT

  private:
    NmeaServer mNmeaServerTrimbleR1 = NmeaServer( "TrimbleR1.txt", NmeaServer::Udp, 1958 );
    NmeaServer mNmeaServerHappy = NmeaServer( "happy.txt", NmeaServer::Tcp, 11111 );
    NmeaServer mNmeaServerHappyWithIMU = NmeaServer( "happyWithIMU.txt", NmeaServer::Udp, 1959 );
    NmeaServer mNmeaServerHappyMonch2WithIMU = NmeaServer( "happyMonch2WithIMU.txt", NmeaServer::Udp, 1960 );

    QString mDataDir;

  public:
    Setup()
    {
      Q_INIT_RESOURCE( qml );
    }

  public slots:
    void applicationAvailable()
    {
      const QStringList arguments = QCoreApplication::arguments();
      QString nmeaServerLocation;
      for ( int i = 0; i < arguments.size(); i++ )
      {
        if ( arguments[i] == QStringLiteral( "-input" ) )
        {
          if ( i + 1 < arguments.size() )
          {
            // the nmea server python script, relative to the absolute input path
            nmeaServerLocation = QString( "%1/../nmea_server" ).arg( arguments[i + 1] );
            mDataDir = QString( "%1/../testdata" ).arg( arguments[i + 1] );
          }
        }
      }

      // Start tcp/udp test servers
      mNmeaServerTrimbleR1.start( nmeaServerLocation );
      mNmeaServerHappy.start( nmeaServerLocation );
      mNmeaServerHappyWithIMU.start( nmeaServerLocation );
      mNmeaServerHappyMonch2WithIMU.start( nmeaServerLocation );
    }

    void cleanupTestCase()
    {
      mNmeaServerTrimbleR1.kill();
      mNmeaServerHappy.kill();
      mNmeaServerHappyWithIMU.kill();
      mNmeaServerHappyMonch2WithIMU.kill();
    }

    void qmlEngineAvailable( QQmlEngine *engine )
    {
      qmlInit( engine );
      engine->rootContext()->setContextProperty( QStringLiteral( "dataDir" ), mDataDir );


      qmlRegisterType<QgsSnappingUtils>( "org.qgis", 1, 0, "SnappingUtils" );
      qmlRegisterType<QgsMapLayerProxyModel>( "org.qgis", 1, 0, "MapLayerModel" );
      qmlRegisterType<QgsVectorLayer>( "org.qgis", 1, 0, "VectorLayer" );
      qmlRegisterType<QgsMapThemeCollection>( "org.qgis", 1, 0, "MapThemeCollection" );
      qmlRegisterType<QgsLocatorProxyModel>( "org.qgis", 1, 0, "QgsLocatorProxyModel" );
      qmlRegisterType<QgsVectorLayerEditBuffer>( "org.qgis", 1, 0, "QgsVectorLayerEditBuffer" );
      qmlRegisterUncreatableType<QgsProject>( "org.qgis", 1, 0, "Project", "" );
      qmlRegisterUncreatableType<QgsProjectDisplaySettings>( "org.qgis", 1, 0, "ProjectDisplaySettings", "" );
      qmlRegisterUncreatableType<QgsRelationManager>( "org.qgis", 1, 0, "RelationManager", "The relation manager is available from the QgsProject. Try `qgisProject.relationManager`" );
      qmlRegisterUncreatableType<QgsMapLayer>( "org.qgis", 1, 0, "MapLayer", "" );
      qmlRegisterUncreatableType<QgsVectorLayer>( "org.qgis", 1, 0, "VectorLayerStatic", "" );
      qmlRegisterType<QgsQuickMapCanvasMap>( "org.qgis", 1, 0, "MapCanvasMap" );
      qmlRegisterType<QgsQuickMapSettings>( "org.qgis", 1, 0, "MapSettings" );
      qmlRegisterType<QgsQuickCoordinateTransformer>( "org.qfield", 1, 0, "CoordinateTransformer" );
      qmlRegisterType<QgsQuickElevationProfileCanvas>( "org.qgis", 1, 0, "ElevationProfileCanvas" );
      qmlRegisterType<QgsQuickMapTransform>( "org.qgis", 1, 0, "MapTransform" );


      qRegisterMetaType<QgsGeometry>( "QgsGeometry" );
      qRegisterMetaType<QgsFeature>( "QgsFeature" );
      qRegisterMetaType<QgsPoint>( "QgsPoint" );
      qRegisterMetaType<QgsPointXY>( "QgsPointXY" );

      qRegisterMetaType<PlatformUtilities::Capabilities>( "PlatformUtilities::Capabilities" );
      qmlRegisterUncreatableType<PlatformUtilities>( "org.qfield", 1, 0, "PlatformUtilities", "" );
      qmlRegisterType<ValueMapModel>( "org.qfield", 1, 0, "ValueMapModel" );
      qmlRegisterType<QgsQuickCoordinateTransformer>( "org.qfield", 1, 0, "CoordinateTransformer" );
      qmlRegisterUncreatableType<QAbstractSocket>( "org.qfield", 1, 0, "QAbstractSocket", "" );
      qmlRegisterUncreatableType<AbstractGnssReceiver>( "org.qfield", 1, 0, "AbstractGnssReceiver", "" );
      qRegisterMetaType<GnssPositionInformation>( "GnssPositionInformation" );
      qmlRegisterType<Positioning>( "org.qfield", 1, 0, "Positioning" );
      qmlRegisterType<AttributeFormModel>( "org.qfield", 1, 0, "AttributeFormModel" );
      qmlRegisterType<SubModel>( "org.qfield", 1, 0, "SubModel" );
      qmlRegisterType<DigitizingLogger>( "org.qfield", 1, 0, "DigitizingLogger" );
      qmlRegisterType<RubberbandModel>( "org.qfield", 1, 0, "RubberbandModel" );
      qmlRegisterType<NearFieldReader>( "org.qfield", 1, 0, "NearFieldReader" );
      qmlRegisterType<BarcodeDecoder>( "org.qfield", 1, 0, "BarcodeDecoder" );
#if QT_VERSION >= QT_VERSION_CHECK( 6, 5, 0 )
      qmlRegisterType<CameraPermission>( "org.qfield", 1, 0, "QfCameraPermission" );
      qmlRegisterType<MicrophonePermission>( "org.qfield", 1, 0, "QfMicrophonePermission" );
#endif

      QgsApplication *mApp;
      qreal dpi = mApp->primaryScreen()->logicalDotsPerInch();

      Settings mSettings;
      mSettings.setValue( "/Map/searchRadiusMM", 5 );

      // Register some globally available variables
      engine->rootContext()->setContextProperty( "ppi", dpi );
      engine->rootContext()->setContextProperty( "qVersion", qVersion() );
      engine->rootContext()->setContextProperty( "settings", &mSettings );
      engine->rootContext()->setContextProperty( "systemFontPointSize", PlatformUtilities::instance()->systemFontPointSize() );
      engine->rootContext()->setContextProperty( "platformUtilities", PlatformUtilities::instance() );

      REGISTER_SINGLETON( "org.qfield", CoordinateReferenceSystemUtils, "CoordinateReferenceSystemUtils" );
      REGISTER_SINGLETON( "org.qfield", StringUtils, "StringUtils" );
    }
};

QUICK_TEST_MAIN_WITH_SETUP( test_qml, Setup )

#include "test_qml.moc"
