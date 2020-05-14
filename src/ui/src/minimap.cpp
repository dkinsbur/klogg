/*
 * Copyright (C) 2011, 2012 Nicolas Bonnefon and other contributors
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

//#include "log.h"
#include <pybind11/stl.h>
#include <pybind11/embed.h>
#include <QFile.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <qdebug.h>
#include "minimap.h"
#include "log.h"

namespace py = pybind11;

MapItem* MapItem::makeRoot()
{
    return new MapItem("", 0);
}

MapItem ::~MapItem()
{
    for (auto x : children_)
    {
        delete x;
    }
    children_.clear();
}
int MapItem::childCount()
{
    return children_.size();
}

MapItem* MapItem::add(const string& name, uint64_t id )
{
    auto item = new MapItem(name, id);

    children_.push_back(item);
    return item;

   
}

const string& MapItem::name() const
{
    return name_;
}

uint64_t MapItem::id() const
{
    return id_;
}

const list<MapItem*> MapItem::children() const
{
    return children_;
}

MapItem::MapItem( const string& name, uint64_t id )
    : children_(), name_(name), id_(id) {}


PYBIND11_EMBEDDED_MODULE(logmap, m)
{

    py::class_<MapItem>(m, "MapItem")
        .def_static("make", &MapItem::makeRoot, py::return_value_policy::reference)
        .def("add", &MapItem::add, py::return_value_policy::reference)
        .def("__repr__", [](MapItem& a) { return "MapItem(named='" + a.name() + "',id=" + to_string(a.id()) + ")"; });
}

////////////////////////////////////////////////////
////// DecodedLog ////////////////////////////////
////////////////////////////////////////////////////
struct GlobalPyCtx
{
    GlobalPyCtx()
    {
        py::initialize_interpreter();
        pyKlog = py::module::import("klogg");
    }
    ~GlobalPyCtx()
    {
        pyKlog = py::none();
        py::finalize_interpreter();
    }

    py::module pyKlog;

};

struct LogPyCtx
{
    LogPyCtx(shared_ptr<GlobalPyCtx> gpc) : globalPyCtx(gpc){}
    shared_ptr<GlobalPyCtx> globalPyCtx;
    py::object pyLog;
};

shared_ptr<GlobalPyCtx> gPyCtx;

#define PY_LOG_CTX  ((LogPyCtx*)ctx_)
#define PY_GLB_CTX  (PY_LOG_CTX->globalPyCtx)
#define PY_KLOGG    (PY_GLB_CTX->pyKlog)
#define PY_TRY      try
#define PY_CATCH    catch (exception& e){ qCritical() << "exception " << e.what(); LOG(logERROR) << "exception " << e.what();}catch(...) {LOG(logERROR) << "DecodedLog Failed";}
#define PY_CHECK_LOG_CTX()  if(ctx_== nullptr) throw exception("Log context is empty")

void DecodedLog::init()
{
    if (gPyCtx == nullptr)
    {
        gPyCtx = make_shared<GlobalPyCtx>();
    }
}

DecodedLog::DecodedLog( const QString& logFileName )
    : fileName_( logFileName )
    , isDecoded_( false )
    , initMapLock_()
    , cahceFileName_(logFileName + ".info")
    , ctx_(nullptr)
{
    PY_TRY{
        init();
        ctx_ = new LogPyCtx(gPyCtx);

        PY_LOG_CTX->pyLog = PY_KLOGG.attr("DecodedLog")(logFileName.toStdString());
    } PY_CATCH;
}


DecodedLog::~DecodedLog() 
{
    if (ctx_)
    {
        delete ctx_;
    }
}

bool DecodedLog::IsMapInitialized()
{
    PY_TRY{
        PY_CHECK_LOG_CTX();
        return PY_LOG_CTX->pyLog.attr("is_map_initialized")().cast<bool>();
    }PY_CATCH;
}

void DecodedLog::generateCache() {
    // call python
}

void DecodedLog::loadCache()
{
    PY_TRY{
        PY_CHECK_LOG_CTX();
    }PY_CATCH;
}

void DecodedLog::loadCacheDone( bool success )
{
    initMapLock_.unlock();
    emit InitMapComplete( success );
}

void DecodedLog::generateCacheDone( bool success )
{
    loadCache();
}

void DecodedLog::InitializeMap()
{
    if ( !initMapLock_.try_lock() ) {
        LOG( logERROR ) << "already initializing map";
        return;
    }
    generateCache();
}



QString DecodedLog::DecodeLine( const QString& logLine )
{
    PY_TRY{
        PY_CHECK_LOG_CTX();
    }PY_CATCH;

}

QString DecodedLog::DecodeItem( uint64_t id )
{
    return QString::fromStdString(PY_LOG_CTX->pyLog.attr("decode_map_item")(id).cast<string>());
}

QStringList DecodedLog::GetMapTypes()
{
    QStringList types;
    PY_TRY{
        PY_CHECK_LOG_CTX();
        auto stdTypes = PY_LOG_CTX->pyLog.attr("get_map_types")().cast<list<string>>();
        for (auto t : stdTypes)
        {
            types << QString::fromStdString(t);
        }
        return types;
    }PY_CATCH;

    return types;
}

MapTree DecodedLog::GetMap(const QString& type)
{
    PY_TRY{
        PY_CHECK_LOG_CTX();
        auto root = PY_LOG_CTX->pyLog.attr("get_map")(type.toStdString()).cast<MapItem*>();
        LOG(logINFO) << "Created new map:" << root << " Log:" << this;
        return MapTree(root, [this](MapItem* ptr) { LOG(logINFO) << "Deleted map:" << ptr << " Log:" << this; });
    }PY_CATCH;

    return MapTree(nullptr);
}
