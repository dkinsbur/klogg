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


#include <QDockWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QFontInfo>
#include <QProcess.h>

class DecodeDockWidget : public QDockWidget
{
  Q_OBJECT

  public:
    DecodeDockWidget();
    ~DecodeDockWidget();


  public slots:
    void updateTextHandler( int index, QString text );

  private slots:
    void updateProjectHandler( const QString& proj );
    void applyOptions();
    void onFinish( int exitCode, QProcess::ExitStatus exitStatus );

  signals:

  private:
    QTextEdit decodedTextBox_;
    QComboBox comboBox_;
    QString   currStr_;
    QProcess process_;

    void parseLine();

};

#endif // DECODEDOCKWIDGET_H
