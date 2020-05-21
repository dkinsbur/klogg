/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2014 Nicolas Bonnefon and other contributors
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

#ifndef MINIMAP_H
#define MINIMAP_H

#include <QMutex>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QThread>
#include <memory>
using namespace std;


class MapItem {
  public:
    static MapItem* makeRoot();
    ~MapItem();

    int childCount();

    MapItem* add(const string& name, uint64_t id );

    const string& name() const;
    uint64_t id() const;
    const list<MapItem*> children() const;

  private:
      MapItem(const string& name, uint64_t id);
      list<MapItem*> children_;

    uint64_t id_;
    string name_;

};

class DecodeWorker;

typedef shared_ptr<MapItem> MapTree;

class MutexTryLocker
{
public:
    MutexTryLocker(QMutex* mutex, int timeout = 0);
    ~MutexTryLocker();
    bool locked();

private:
    QMutex* _mutex;
    bool _locked;
};

class DecodedLog : public QObject {

    Q_OBJECT

        friend DecodeWorker;
private:
    static QMutex pyLock_;
  public:
    static void init();
    DecodedLog( const QString& logFileName );
    ~DecodedLog();

    QString logFileName();

    // minimap
    void Decode();
    void DecodeAbort();
    bool IsDecoded();

    QString DecodeLine( const QString& logLine );
    QString DecodeItem( uint64_t id );
    QList<uint32_t> ItemLines( uint64_t id );
    QStringList GetMapTypes();
    MapTree GetMap(const QString& mapType );

  signals:
    void DecodeComplete( bool success );
    void DecodeProgressUpdated(int progress);
    void requestedPythonInUse();

private slots:
    void CleanupWorker();
private:
    void checkProgress();
    QString fileName_;
    QString cahceFileName_;
    bool isDecoded_;
    QProcess process_;
    QMap<uint64_t, void*> itemInfo_;
    void* ctx_;
private:
    DecodeWorker* worker_;
    bool decodeSuccess;


};

class DecodeWorker : public QThread
{
    Q_OBJECT
public:
    DecodeWorker(DecodedLog* dl) : dl_(dl) {}
private:
    DecodedLog* dl_;
protected:
    void run();
};


#endif