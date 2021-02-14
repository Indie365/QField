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
#include "qgismobileapp.h"

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

void AppInterface::print( int layoutIndex )
{
  return mApp->print( layoutIndex );
}

void AppInterface::openFeatureForm()
{
  emit openFeatureFormRequested();
}
