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

class AndroidPlatformUtilities : public PlatformUtilities
{
  public:
    AndroidPlatformUtilities();

    PlatformUtilities::Capabilities capabilities() const override { return Capabilities() | NativeCamera | AdjustBrightness; }

    void initSystem() override;
    QString systemGenericDataLocation() const override;
    QString qgsProject() const override;
    QString qfieldDataDir() const override;
    PictureSource *getCameraPicture( QQuickItem *parent, const QString &prefix, const QString &pictureFilePath, const QString &suffix ) override;
    PictureSource *getGalleryPicture( QQuickItem *parent, const QString &prefix, const QString &pictureFilePath ) override;
    ViewStatus *open( const QString &uri ) override;
    ProjectSource *openProject() override;

    bool checkPositioningPermissions() const override;

    bool checkCameraPermissions() const override;

    bool checkWriteExternalStoragePermissions() const override;

    void setScreenLockPermission( const bool allowLock ) override;

    void dimBrightness() override;
    void restoreBrightness() override;

    void showRateThisApp() const override;

  private:
    bool checkAndAcquirePermissions( const QString &permissionString ) const;
    QString getIntentExtra( const QString &, QAndroidJniObject = nullptr ) const;
    QAndroidJniObject getNativeIntent() const;
    QAndroidJniObject getNativeExtras() const;
    QAndroidJniObject mActivity;
    QString mSystemGenericDataLocation;
};

#endif // ANDROIDPLATFORMUTILITIES_H
