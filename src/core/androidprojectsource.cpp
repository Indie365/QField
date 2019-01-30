/***************************************************************************
  androidprojectsource.cpp - AndroidProjectSource

 ---------------------
 begin                : 19.3.2018
 copyright            : (C) 2018 by Matthias Kuhn
 email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmessagelog.h"
#include "androidprojectsource.h"
#include <QFile>
#include <QtAndroid>

AndroidProjectSource::AndroidProjectSource()
  : ProjectSource( nullptr )
  , QAndroidActivityResultReceiver()
{
}

void AndroidProjectSource::handleActivityResult( int receiverRequestCode, int resultCode, const QAndroidJniObject &data )
{
  jint RESULT_OK = QAndroidJniObject::getStaticField<jint>( "android/app/Activity", "RESULT_OK" );
  if ( receiverRequestCode == 103 && resultCode == RESULT_OK )
  {
    QAndroidJniObject uri = data.callObjectMethod( "getData", "()Landroid/net/Uri;" );

    QAndroidJniObject contentResolver = QtAndroid::androidActivity().callObjectMethod( "getContentResolver",
                                        "()Landroid/content/ContentResolver;" );

    QString path = QAndroidJniObject::callStaticObjectMethod( "ch/opengis/qfield/FileUtils", "getPathFromUri",
                   "(Landroid/net/Uri;Landroid/content/ContentResolver;)Ljava/lang/String;",
                   uri.object(), contentResolver.object() ).toString();

    if ( !QFile( path ).exists() )
    {
      QgsMessageLog::logMessage( tr( "File %1 does not exist" ).arg( path ), QStringLiteral( "QField" ), Qgis::Warning );
    }

    emit projectOpened( path );
  }
}
