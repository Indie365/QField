/***************************************************************************
                            androidplatformutilities.h  -  utilities for qfield

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

#ifndef ANDROIDPLATFORMUTILITIES_H
#define ANDROIDPLATFORMUTILITIES_H

#include "platformutilities.h"
#include <QAndroidJniObject>
#include <QAndroidActivityResultReceiver>

class AndroidPlatformUtilities : public PlatformUtilities, public QAndroidActivityResultReceiver
{
  public:
    AndroidPlatformUtilities();

    virtual QString configDir() const override;
    virtual QString shareDir() const override;
    virtual void getPicture( const QString &prefix ) override;

    //! QAndroidActivityResultReceiver
    void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject& data) override;

  private:
    QMap<QString, QString> getIntentExtras( QStringList ) const;
    QString getIntentExtra( QString, QAndroidJniObject=0 ) const;
    QAndroidJniObject getNativeIntent() const;
    QAndroidJniObject getNativeExtras() const;
};

#endif // ANDROIDPLATFORMUTILITIES_H
