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
#include <QProgressBar>




#include <QMutex>
#include <QThread>

struct DecodedWidgetState
{
    int             progress;
    bool            decoded;
    LineNumber      line;
    QString         itemInfo;
    QString         lineInfo;
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

    void decodeLog();
    void decodeLogAbort();

    void decodeLine(LineNumber lineNumber, QString lineText);
    void decodeMapItem(uint64_t itemId);
    void generateLogMap(const QString& mapType);
    void openLink(const QUrl&);
    
    void updateDecodeLogProgress(int progress);
    void updateDecodeComplete(bool success);

  signals:
    void logDecoderOpened(void* handle);
    void logDecoderClosed(void* handle);
    void widgetStateChanged(shared_ptr<DecodedWidgetState> state);
    void decodeComplete(bool success, QString logName);


    void lineDecoded( QString& info );


    void decodeLogProgressChanged( int proress );
    void decodeLogCompleted( bool succeeded );
    
    // this signal is triggered to notify the log view that a new line in the log should be selected 
    void foundMatchingLine(LineNumber lineNumber);

    void logMapGenerated();

  private:
      bool getCurrents(shared_ptr<DecodedWidgetState>* state, shared_ptr<DecodedLog>* dl);
    QMap<void*, shared_ptr<DecodedWidgetState>> states_;
    QMap<void*, shared_ptr<DecodedLog>> logs_;

    CrawlerWidget* currLog_;
    QMutex lock_;
};


class DecodeDockWidget : public QDockWidget
{
    Q_OBJECT


signals:
    void selectedMapItemChanged(uint64_t id);
    bool decodeRequested();
    
public slots:
    void reloadWidgetState(shared_ptr<DecodedWidgetState>);

    private slots:
        void checkDoDecode();

private:

    void updateCurrLog(CrawlerWidget* log);
    void updateUI(int updateType);
    void updateMap(MapTree root);
    void updateUISelectedItem(uint64_t item);

    void MinimapAddItem(MapItem* object, QTreeWidget* parent);
    void MinimapAddItem(MapItem* object, QTreeWidgetItem* parent);

public:
    DecodeDockWidget();
    ~DecodeDockWidget();
    DecodeManager& getDecodeManager();

private slots:
    void applyOptions();
    void checkDecodeFailure(bool success, QString logName);

private:
    QWidget mainWidget_;
    QVBoxLayout mainLayout_;
    QComboBox minimapTypeCombo_;
    QTreeWidget minimapTree_;
    QTextBrowser decodedTextBox_;
    QWidget historyLayoutWidget_;
    QHBoxLayout historyLayout_;
    QPushButton historyForwardBtn_;
    QPushButton historyBackwardsBtn_;
    QPushButton decodeBtn_;
    QStatusBar statusbar_;
    DecodeManager decMgr_;
    QThread decMgrTh_;
    DecodedWidgetState currState_;
    QProgressBar decodeProgressBar_;
    QLabel decodeStatusLabel_;

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
