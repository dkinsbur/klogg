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

#include "crawlerwidget.h"
#include "minimap.h"
#include <QComboBox>
#include <QDockWidget>
#include <QFontInfo>
#include <QMap>
#include <QProcess.h>
#include <QStatusBar>
#include <QTextBrowser>
#include <QTreeWidget>




#include <QMutex>
#include <QThread>

struct DecodedWidgetState
{
    QString         info;
    QStringList     mapTypes;
    QString         mapType;
    uint64_t        mapItem;
    MapTree         map;
};

class DecodeManager : public QObject {

    Q_OBJECT

  public:
      DecodeManager();
      ~DecodeManager();
  public slots:
    void openLogDecoder( CrawlerWidget* logView );
    void closeLogDecoder( CrawlerWidget* logView );
    void switchCurrentLogDecoder( CrawlerWidget* logView );

    void parseLog(){}
    void decodeLine( LineNumber lineNumber, QString& lineText ){}
    void decodeMapItem(uint64_t itemId);
    void generateLogMap(const QString& mapType);

  signals:
    void logDecoderOpened(void* handle);
    void logDecoderClosed(void* handle);
    void widgetStateChanged(shared_ptr<DecodedWidgetState> state);


    void lineDecoded( QString& info );


    void parseProgressChanged( int proress );
    void parseLogCompleted( bool succeeded );
    
    // this signal is triggered to notify the decode widget that a new map item should be selected
    void foundMatchingMapItem(uint64_t itemId);

    // this signal is triggered to notify the log view that a new line in the log should be selected 
    void foundMatchingLine(LineNumber lineNumber);

    void logMapGenerated();

  private:
    QMap<void*, shared_ptr<DecodedWidgetState>> states_;
    QMap<void*, shared_ptr<DecodedLog>> logs_;

    CrawlerWidget* currLog_;
    QMutex lock_;
};


class DecodeDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    void handleLineSelectionChanged(LineNumber line);
    void handleLinkClick(QString& link);
    void handleForwardBackwardsClick(bool forward);
    void handleDecodeClick();

    DecodeManager& getDecodeManager();

signals:
    void selectedMapItemChanged(uint64_t id);
public slots:
    void reloadWidgetState(shared_ptr<DecodedWidgetState>);

    void handleMinimapTypeChange(int index);
    void handleMinimapSelectionChange(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:

    void updateCurrLog(CrawlerWidget* log);
    void updateMapType(QString& mapType);

    void updateUI(int updateType);
    void updateMap(MapTree root);
    void updateUISelectedItem(uint64_t item);

    void MinimapAddItem(MapItem* object, QTreeWidget* parent);
    void MinimapAddItem(MapItem* object, QTreeWidgetItem* parent);

public:
    DecodeDockWidget();
    ~DecodeDockWidget();

private slots:
    void applyOptions();

private:
    QWidget mainWidget_;
    QVBoxLayout mainLayout_;
    QComboBox minimapTypeCombo_;
    QTreeWidget minimapTree_;
    QTextBrowser decodedTextBox_;
    QHBoxLayout historyLayout_;
    QPushButton historyForwardBtn_;
    QPushButton historyBackwardsBtn_;
    QPushButton decodeBtn_;
    QStatusBar statusbar_;
    DecodeManager decMgr_;
    QThread decMgrTh_;
    DecodedWidgetState currState_;

    // void AddLogObject( MinimapObject* object, QTreeWidget* parent );
    // void AddLogObject( MinimapObject* object, QTreeWidgetItem* parent );
    //
    // void parseLine();
    // void selectTreeItemById( uint64_t id );
    // void updateMinimapObjectInfo( QString& info );
    // void updatedHtml();
    void* stateHandle;
    QMutex stateLock_;

    void setStatusMessage(const QString& text);
};
#endif // DECODEDOCKWIDGET_H
