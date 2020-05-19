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
#include <pybind11/functional.h>
#include <QFile.h>
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

#define PY_LOG_CTX_(ctx)        ((LogPyCtx*)ctx)
#define PY_LOG_CTX              PY_LOG_CTX_(ctx_)
#define PY_GLB_CTX              (PY_LOG_CTX->globalPyCtx)
#define PY_KLOGG                (PY_GLB_CTX->pyKlog)
#define PY_TRY                  try
#define PY_CATCH                catch (exception& e){ qCritical() << "exception " << e.what(); LOG(logERROR) << "exception " << e.what();}catch(...) {LOG(logERROR) << "DecodedLog Failed";}
#define PY_CHECK_LOG_CTX_(ctx)  if((ctx)== nullptr || PY_LOG_CTX_(ctx)->pyLog.ptr() == NULL) throw exception("Log context is empty")
#define PY_CHECK_LOG_CTX()      PY_CHECK_LOG_CTX_(ctx_)

#define PY_DL_INIT                      "DecodedLog"
#define PY_DL_IS_DECODED                "is_decoded"
#define PY_DL_DECODE                    "decode"
#define PY_DL_DECODE_ABORT              "decode_abort"
#define PY_DL_GET_DECODE_PROGRESS       "get_decode_progress"
#define PY_DL_GET_MAP_TYPES             "get_map_types"
#define PY_DL_GET_MAP                   "get_map"
#define PY_DL_GET_MAP_ITEM_LOG_LINES    "get_map_item_log_lines"
#define PY_DL_DECODE_MAP_ITEM           "decode_map_item"
#define PY_DL_DECODE_LINE               "decode_line"

void DecodeWorker::run()
{
    
    bool success = false;
    PY_TRY{
        PY_CHECK_LOG_CTX_(dl_->ctx_);
        success = PY_LOG_CTX_(dl_->ctx_)->pyLog.attr(PY_DL_DECODE)().cast<bool>();
    }PY_CATCH;

    emit dl_->DecodeComplete(success);
}

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
    , worker_(nullptr)
{
    PY_TRY{
        init();
        ctx_ = new LogPyCtx(gPyCtx);
        function<void(int)> progressCallback = [this](int progress) { emit DecodeProgressUpdated(progress); };
        PY_LOG_CTX->pyLog = PY_KLOGG.attr(PY_DL_INIT)(logFileName.toStdString(), progressCallback);
    } PY_CATCH;
}

DecodedLog::~DecodedLog() 
{
    if (ctx_)
    {
        delete ctx_;
    }
}

QString DecodedLog::logFileName()
{
    return fileName_;
}

bool DecodedLog::IsDecoded()
{
    PY_TRY{
        PY_CHECK_LOG_CTX();
        return PY_LOG_CTX->pyLog.attr(PY_DL_IS_DECODED)().cast<bool>();
    }PY_CATCH;

    return false;
}

void DecodedLog::Decode()
{
    if ( !initMapLock_.try_lock() ) {
        LOG( logERROR ) << "already initializing map";
        return;
    }

    //timer_.setInterval(500);
    //connect(&timer_, &QTimer::timeout, [this]()
    //{
    //    LOG(logERROR) << "timer" << this << " " << QThread::currentThread() << " " << this->thread();
    //});// &DecodedLog::checkProgress);
    //timer_.start(500);

    worker_ = new DecodeWorker(this);
    connect(worker_, &QThread::finished, this, &DecodedLog::CleanupWorker);
    worker_->start();

}

void DecodedLog::CleanupWorker()
{
    // cleanup timer
    //timer_.stop();

    disconnect(worker_, &QThread::finished, this, &DecodedLog::CleanupWorker);
    delete worker_;
    worker_ = nullptr;
    initMapLock_.unlock();
}
void DecodedLog::checkProgress()
{
    LOG(logINFO) << "checknig progress";
    int progress = -1;

    PY_TRY{
        PY_CHECK_LOG_CTX();
        progress = PY_LOG_CTX->pyLog.attr(PY_DL_GET_DECODE_PROGRESS)().cast<int>();
    }PY_CATCH;

    LOG(logINFO) << "progress: " << progress;
    emit DecodeProgressUpdated(progress);
}

void DecodedLog::DecodeAbort()
{
    LOG(logINFO) << "Aborting Decode";

    if (initMapLock_.tryLock())
    {
        LOG(logINFO) << "Currently not decoding";
        initMapLock_.unlock();
        return;
    }
    
    PY_TRY{
        PY_CHECK_LOG_CTX();
        PY_LOG_CTX->pyLog.attr(PY_DL_DECODE_ABORT)().cast<bool>();
    }PY_CATCH;

}


QString DecodedLog::DecodeLine( const QString& logLine )
{
    PY_TRY{
        PY_CHECK_LOG_CTX();
        return QString::fromStdString(PY_LOG_CTX->pyLog.attr(PY_DL_DECODE_LINE)(logLine.toStdString()).cast<string>());
    }PY_CATCH;
    return "";
}

QString DecodedLog::DecodeItem( uint64_t id )
{
    PY_TRY{
        PY_CHECK_LOG_CTX();
       return QString::fromStdString(PY_LOG_CTX->pyLog.attr(PY_DL_DECODE_MAP_ITEM)(id).cast<string>());
    }PY_CATCH;

    return "";
}

QList<uint32_t> DecodedLog::ItemLines(uint64_t id)
{
    PY_TRY{
        PY_CHECK_LOG_CTX();
       return QList<uint32_t>::fromStdList(PY_LOG_CTX->pyLog.attr(PY_DL_GET_MAP_ITEM_LOG_LINES)(id).cast<list<uint32_t>>());
    }PY_CATCH;

    return QList<uint32_t>();
    
}

QStringList DecodedLog::GetMapTypes()
{
    QStringList types;
    PY_TRY{
        PY_CHECK_LOG_CTX();
        auto stdTypes = PY_LOG_CTX->pyLog.attr(PY_DL_GET_MAP_TYPES)().cast<list<string>>();
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
        auto root = PY_LOG_CTX->pyLog.attr(PY_DL_GET_MAP)(type.toStdString()).cast<MapItem*>();
        LOG(logINFO) << "Created new map:" << root << " Log:" << this;
        return MapTree(root, [this](MapItem* ptr) { LOG(logINFO) << "Deleted map:" << ptr << " Log:" << this; });
    }PY_CATCH;

    return MapTree(nullptr);
}
