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


//struct MapTreeItem
//{
//public:
//    static MapTreeItem* make() { return new MapTreeItem("", 0); }
//    MapTreeItem* add(const string& name, uint64_t id)
//    {
//        auto item = new MapTreeItem(name, id);
//
//        children_.push_back(item);
//        return item;
//    }
//    uint64_t id() { return id_; } const
//        const string& name() { return name_; }
//    const list< MapTreeItem*> children()
//    {
//        return children_;
//    }
//    MapTreeItem(const string& name, uint64_t id) :name_(name), id_(id) {}
//    ~MapTreeItem()
//    {
//        for (auto x : children_)
//        {
//            delete x;
//        }
//        children_.clear();
//    }
//private:
//
//    uint64_t id_;
//    string name_;
//    list<MapTreeItem*> children_;
//
//};

//struct ItemInfo {
//    uint64_t id;
//    QString name;
//    int type;
//    QString info;
//};

typedef shared_ptr<MapItem> MapTree;

class DecodedLog : public QObject {

    Q_OBJECT

  public:
    static void init();
    DecodedLog( const QString& logFileName );
    ~DecodedLog();

    // minimap
    void InitializeMap();
    void InitializeMapFromCache();
    bool IsMapInitialized();

    QString DecodeLine( const QString& logLine );
    QString DecodeItem( uint64_t id );
    QStringList GetMapTypes();
    MapTree GetMap(const QString& mapType );

  signals:
    void InitMapComplete( bool success );
    void DecodeLineComplete( bool success, QString& info );
    void DecodeItemComplete( bool success, QString& info );

  private:
    void loadCache();
    void generateCache();
    void loadCacheDone( bool success );
    void generateCacheDone( bool success );

    QString fileName_;
    QString cahceFileName_;
    bool isDecoded_;
    QMutex initMapLock_;
    QProcess process_;
    QMap<uint64_t, void*> itemInfo_;

    void* ctx_;

};

#endif