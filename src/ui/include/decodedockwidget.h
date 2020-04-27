/*
 * Copyright (C) 2014 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DECODEDOCKWIDGET_H
#define DECODEDOCKWIDGET_H

#include <QComboBox>
#include <QDockWidget>
#include <QFontInfo>
#include <QProcess.h>
#include <QTextBrowser>
#include <QTreeWidget>
#include "minimap.h"

class DecodeDockWidget : public QDockWidget {
    Q_OBJECT

  public:
    DecodeDockWidget();
    ~DecodeDockWidget();

  public slots:
    void updateTextHandler( int index, QString text );
    void updatedMinimap( MinimapObject* root );

  private slots:
    void updateProjectHandler( const QString& proj );
    void applyOptions();
    void onFinish( int exitCode, QProcess::ExitStatus exitStatus );

  signals:
    void MinimapObjectChanged( int line );

  private:
    QTextBrowser decodedTextBox_;
    QComboBox comboBox_;
    QString currStr_;
    QProcess process_;
    QTreeWidget tree_;

    void AddLogObject( MinimapObject* object, QTreeWidget* parent );
    void AddLogObject( MinimapObject* object, QTreeWidgetItem* parent );

    void parseLine();
    void selectTreeItemById( uint64_t id );
};

#endif // DECODEDOCKWIDGET_H
