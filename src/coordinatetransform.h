/***************************************************************************
              coordinatetransform.h
               ----------------------------------------------------
              date                 : 26.2.2015
              copyright            : (C) 2015 by Matthias Kuhn
              email                : matthias (at) opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef COORDINATETRANSFORM_H
#define COORDINATETRANSFORM_H

#include <QObject>
#include <QPointF>
#include <qgspoint.h>

#include <qgscoordinatetransform.h>

class CoordinateTransform : public QObject
{
    Q_OBJECT

    Q_PROPERTY( QgsCoordinateReferenceSystem sourceCRS READ sourceCRS WRITE setSourceCRS NOTIFY sourceCRSChanged )
    Q_PROPERTY( QgsCoordinateReferenceSystem destinationCRS READ destinationCRS WRITE setDestinationCRS NOTIFY destinationCRSChanged )

  public:
    explicit CoordinateTransform( QObject *parent = nullptr );

    Q_INVOKABLE QgsPointXY transform( QgsPointXY pt ) const;

    QgsCoordinateReferenceSystem sourceCRS() const;
    QgsCoordinateReferenceSystem destinationCRS() const;

    void setSourceCRS( QgsCoordinateReferenceSystem sourceCRS );
    void setDestinationCRS( QgsCoordinateReferenceSystem destCRS );

  signals:
    void destinationCRSChanged();
    void sourceCRSChanged();

  private:
    QgsCoordinateTransform mTransform;
};

#endif // COORDINATETRANSFORM_H
