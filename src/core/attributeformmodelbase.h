/***************************************************************************
  attributeformmodelbase.h - AttributeFormModelBase

 ---------------------
 begin                : 16.8.2016
 copyright            : (C) 2016 by Matthias Kuhn
 email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef ATTRIBUTEFORMMODELBASE_H
#define ATTRIBUTEFORMMODELBASE_H

#include "featuremodel.h"

#include <QStack>
#include <QStandardItemModel>
#include <qgsattributeeditorcontainer.h>
#include <qgseditformconfig.h>
#include <qgsexpressioncontext.h>

class AttributeFormModelBase : public QStandardItemModel
{
    Q_OBJECT

    Q_PROPERTY( FeatureModel *featureModel READ featureModel WRITE setFeatureModel NOTIFY featureModelChanged )
    Q_PROPERTY( bool hasTabs READ hasTabs WRITE setHasTabs NOTIFY hasTabsChanged )
    Q_PROPERTY( bool constraintsHardValid READ constraintsHardValid NOTIFY constraintsHardValidChanged )
    Q_PROPERTY( bool constraintsSoftValid READ constraintsSoftValid NOTIFY constraintsSoftValidChanged )

  public:
    explicit AttributeFormModelBase( QObject *parent = nullptr );

    QHash<int, QByteArray> roleNames() const override;

    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) override;

    FeatureModel *featureModel() const;
    void setFeatureModel( FeatureModel *featureModel );

    bool hasTabs() const;
    void setHasTabs( bool hasTabs );

    bool save();

    bool create();

    bool deleteFeature();

    bool constraintsHardValid() const;

    bool constraintsSoftValid() const;

    QVariant attribute( const QString &name );

    //! Applies feature model data such as attribute values, constraints, visibility to the attribute form model
    void applyFeatureModel();

  signals:
    void featureModelChanged();
    void hasTabsChanged();
    void featureChanged();
    void constraintsHardValidChanged();
    void constraintsSoftValidChanged();

  private:
    /**
     * Generates a root container for autogenerated layouts, so we can just use the same
     * form logic to deal with them.
     */
    QgsAttributeEditorContainer *generateRootContainer() const;

    QgsAttributeEditorContainer *invisibleRootContainer() const;

    void updateAttributeValue( QStandardItem *item );

    void flatten( QgsAttributeEditorContainer *container, QStandardItem *parent, const QString &parentVisibilityExpressions, QVector<QStandardItem *> &items, int currentTabIndex = 0, const QColor &color = QColor() );

    void updateDefaultValues( int fieldIndex = -1, QVector<int> updatedFields = QVector<int>() );

    void updateVisibilityAndConstraints( int fieldIndex = -1 );

    void setConstraintsHardValid( bool constraintsHardValid );

    void setConstraintsSoftValid( bool constraintsSoftValid );

    /**
     * finds the best widget type regarding to the field type or the configured widget setup
     * \param fieldIndex to get the field
     * \returns widget setup containing the best widget type
     */
    QgsEditorWidgetSetup findBest( int fieldIndex );

    //! Resets the attribute form model
    void resetModel();

    //! Sets up a connection to listen to project map theme change
    void onMapThemeCollectionChanged();

    FeatureModel *mFeatureModel = nullptr;
    QgsVectorLayer *mLayer = nullptr;
    std::unique_ptr<QgsAttributeEditorContainer> mTemporaryContainer;
    bool mHasTabs = false;

    typedef QPair<QgsExpression, QVector<QStandardItem *>> VisibilityExpression;
    QList<VisibilityExpression> mVisibilityExpressions;
    QMap<QStandardItem *, QgsFieldConstraints> mConstraints;
    QMap<QStandardItem *, QString> mEditorWidgetCodes;

    QgsExpressionContext mExpressionContext;
    bool mConstraintsHardValid = true;
    bool mConstraintsSoftValid = true;
};

#endif // ATTRIBUTEFORMMODELBASE_H
