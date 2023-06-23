/***************************************************************************
  featureutils.cpp - FeatureUtils

 ---------------------
 begin                : 05.03.2020
 copyright            : (C) 2020 by Denis Rouzaud
 email                : denis@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "featureutils.h"
#include "qgsquickmapsettings.h"

#include <qgsexpressioncontextutils.h>
#include <qgsproject.h>
#include <qgsvectorlayer.h>


FeatureUtils::FeatureUtils( QObject *parent )
  : QObject( parent )
{
}

QgsFeature FeatureUtils::initFeature( QgsVectorLayer *layer, QgsGeometry geometry )
{
  QgsFeature f( layer->fields() );
  f.setGeometry( geometry );
  return f;
}

QString FeatureUtils::displayName( QgsVectorLayer *layer, const QgsFeature &feature )
{
  if ( !layer )
    return QString();

  QgsExpressionContext context = QgsExpressionContext()
                                 << QgsExpressionContextUtils::globalScope()
                                 << QgsExpressionContextUtils::projectScope( QgsProject::instance() )
                                 << QgsExpressionContextUtils::layerScope( layer );
  context.setFeature( feature );

  QString name = QgsExpression( layer->displayExpression() ).evaluate( &context ).toString();
  if ( name.isEmpty() )
    name = QString::number( feature.id() );

  return name;
}

QgsRectangle FeatureUtils::extent( QgsQuickMapSettings *mapSettings, QgsVectorLayer *layer, const QgsFeature &feature )
{
  if ( mapSettings && layer && layer->geometryType() != Qgis::GeometryType::Unknown && layer->geometryType() != Qgis::GeometryType::Null )
  {
    QgsCoordinateTransform transf( layer->crs(), mapSettings->destinationCrs(), mapSettings->mapSettings().transformContext() );
    QgsGeometry geom( feature.geometry() );
    if ( !geom.isNull() )
    {
      geom.transform( transf );

      QgsRectangle extent;
      if ( geom.type() == Qgis::GeometryType::Point )
      {
        QgsVector delta = QgsPointXY( geom.asPoint() ) - mapSettings->extent().center();
        extent = mapSettings->extent();
        extent.setXMinimum( extent.xMinimum() + delta.x() );
        extent.setXMaximum( extent.xMaximum() + delta.x() );
        extent.setYMinimum( extent.yMinimum() + delta.y() );
        extent.setYMaximum( extent.yMaximum() + delta.y() );
      }
      else
      {
        QgsRectangle featureExtent = geom.boundingBox();
        extent = featureExtent.buffered( std::max( featureExtent.width(), featureExtent.height() ) / 4.0 );
      }
      return extent;
    }
  }

  return QgsRectangle();
}
