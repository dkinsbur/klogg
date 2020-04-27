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
#include <QTextEdit>
#include <QTreeWidget>

struct LogObject {
    QString name;
    uint32_t line;
};

class LogObjectItem {
  public:
    static LogObjectItem* loadJson(QString path);

    LogObjectItem( LogObject& data, LogObjectItem* parent);
    LogObjectItem( );
    ~LogObjectItem();
    int count() const;
    LogObjectItem* getChild( int i ) const;
    LogObjectItem* getParent() const;
    LogObject& getData() ;
    LogObjectItem* addChild( LogObject& data, int index=-1);

  private:
    QVector<LogObjectItem*> _children;
    LogObject _data;
    LogObjectItem* _parent;
};

//class TreeWidget : public QDockWidget {
//    Q_OBJECT
//
//  public:
//    TreeWidget();
//    ~TreeWidget();
//
////  public slots:
////    void UpdateTreeInfo( LogObjectItem& root );
////
////signals:
////    void JumpTo( int line );
//
//  private:
//      //QTreeWidget tree_;
//    //void AddLogObject( LogObjectItem* object, QTreeWidget* parent );
//    //  void AddLogObject( LogObjectItem* object, QTreeWidgetItem* parent );
//};
//
class DecodeDockWidget : public QDockWidget {
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

  public slots:
    void UpdateTreeInfo( LogObjectItem& root );

  signals:
    void JumpTo( int line );

  private:
    QTextEdit decodedTextBox_;
    QComboBox comboBox_;
    QString currStr_;
    QProcess process_;
    QTreeWidget tree_;

    void AddLogObject( LogObjectItem* object, QTreeWidget* parent );
    void AddLogObject( LogObjectItem* object, QTreeWidgetItem* parent );

    void parseLine();
};

#endif // DECODEDOCKWIDGET_H
