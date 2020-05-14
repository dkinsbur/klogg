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

#include "log.h"

#include <cassert>

#include "configuration.h"
#include "decodedockwidget.h"
#include "persistentinfo.h"
#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>

DecodeManager::DecodeManager() : lock_()
{
    LOG(logINFO) << "DecodeManager ";
}
DecodeManager::~DecodeManager()
{
    {
        QMutexLocker locker(&lock_);
        states_.clear();
        logs_.clear();
    }
}
void DecodeManager::openLogDecoder(CrawlerWidget* logView)
{
    LOG(logINFO) << "openLogDecoder " << logView;
    
    shared_ptr<DecodedLog> dl = make_shared<DecodedLog>(logView->logFileName());
    shared_ptr<DecodedWidgetState> state = make_shared<DecodedWidgetState>();

    {
        QMutexLocker locker(&lock_);
        logs_[logView] = dl;
        states_[logView] = state;
    }

    state->mapTypes = dl->GetMapTypes();
    state->mapType = state->mapTypes.isEmpty() ? "" : state->mapTypes.first();
    state->map = state->mapTypes.isEmpty() ? MapTree() : dl->GetMap(state->mapType);
    
}

void DecodeManager::switchCurrentLogDecoder(CrawlerWidget* logView)
{
    LOG(logINFO) << "switchCurrentLogDecoder:" << logView;

    shared_ptr<DecodedWidgetState> state(nullptr);

    {
        QMutexLocker locker(&lock_);
        currLog_ = logView;
        if (states_.contains(logView))
        {
            state = states_[currLog_];
        }
    }
    
    emit widgetStateChanged(state);
}

void DecodeManager::generateLogMap(const QString& mapType)
{
    if (!logs_.contains(currLog_))
    {
        return;
    }

    lock_.lock();
    auto state = states_[currLog_];
    auto dl = logs_[currLog_];
    lock_.unlock();

    if (mapType != state->mapType)
    {
        state->mapType = mapType;
        state->map = dl->GetMap(mapType);
        emit widgetStateChanged(state);
    }
}
void DecodeManager::decodeMapItem(uint64_t itemId)
{
    if (!logs_.contains(currLog_))
    {
        return;
    }

    lock_.lock();
    auto state = states_[currLog_];
    auto dl = logs_[currLog_];
    lock_.unlock();

    if (itemId != state->mapItem)
    {
        state->mapItem = itemId;
        state->info = dl->DecodeItem(itemId);
        emit widgetStateChanged(state);
    }
}

void DecodeManager::closeLogDecoder(CrawlerWidget* logView)
{
    LOG(logINFO) << "onLogClose " << logView;

    if (currLog_ == logView)
    {
        switchCurrentLogDecoder(nullptr);
    }

    {
        QMutexLocker locker(&lock_);
        if (logs_.contains(logView))
        {
            states_.remove(logView);
            logs_.remove(logView);
        }
    }
}

DecodeManager& DecodeDockWidget::getDecodeManager()
{
    return decMgr_;
}

Q_DECLARE_METATYPE(shared_ptr<DecodedWidgetState>)

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

DecodeDockWidget::DecodeDockWidget()
    : QDockWidget()
    , decodedTextBox_()
    , minimapTypeCombo_()
    , statusbar_()
    , decMgr_()
    , decMgrTh_()
{
    decodedTextBox_.setReadOnly(true);
    decodedTextBox_.setLineWrapMode(QTextEdit::NoWrap);
    decodedTextBox_.setAutoFillBackground(true);
    decodedTextBox_.setOpenExternalLinks(false);
    decodedTextBox_.setOpenLinks(false);

    minimapTree_.setColumnCount(1);
    minimapTree_.setHeaderHidden(true);
    minimapTree_.setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout_.addWidget(&minimapTypeCombo_);
    mainLayout_.addWidget(&minimapTree_);

    mainLayout_.addWidget(&decodedTextBox_);
    mainLayout_.addWidget(&statusbar_);
    mainLayout_.setContentsMargins(1, 1, 1, 1);
    mainWidget_.setLayout(&mainLayout_);
    setWidget(&mainWidget_);
    setFeatures(DockWidgetMovable | DockWidgetFloatable | DockWidgetClosable);
    setWindowTitle(tr("Log Info"));

    decMgr_.moveToThread(&decMgrTh_);
    decMgrTh_.start();

    connect(
        &decMgr_, &DecodeManager::widgetStateChanged, 
        this, &DecodeDockWidget::reloadWidgetState, 
        Qt::QueuedConnection
    );
    connect(
        &minimapTypeCombo_, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), 
        &decMgr_, &DecodeManager::generateLogMap, 
        Qt::QueuedConnection
    );
    connect(
        &minimapTree_, &QTreeWidget::currentItemChanged,
        [this](QTreeWidgetItem* current, QTreeWidgetItem* previous){emit selectedMapItemChanged(current->data(0, Qt::UserRole).toULongLong());}
    );
    connect(
        this, &DecodeDockWidget::selectedMapItemChanged,
        &decMgr_, &DecodeManager::decodeMapItem,
        Qt::QueuedConnection
    );

}

DecodeDockWidget::~DecodeDockWidget() 
{
}

void DecodeDockWidget::reloadWidgetState(shared_ptr<DecodedWidgetState> state)
{

    if (state == nullptr)
    {
        currState_ = DecodedWidgetState();
        minimapTree_.blockSignals(true);
        minimapTypeCombo_.blockSignals(true);
        minimapTypeCombo_.clear();
        minimapTree_.clear();
        decodedTextBox_.clear();
        minimapTree_.blockSignals(false);
        minimapTypeCombo_.blockSignals(false);
        return;
    }

    auto prev = currState_;
    currState_ = *state;

    auto index = state->mapTypes.indexOf(state->mapType);

    decodedTextBox_.setHtml(state->info);

    minimapTypeCombo_.blockSignals(true);
    minimapTypeCombo_.clear();
    minimapTypeCombo_.addItems(state->mapTypes);
    if (index >= 0)
    {
        minimapTypeCombo_.setCurrentIndex(index);
    }
    minimapTypeCombo_.blockSignals(false);
    
    
    if (prev.map != state->map)
    {
        updateMap(state->map);
        auto iter = QTreeWidgetItemIterator(&minimapTree_);
        while (*iter)
        {
            if ((*iter)->data(0, Qt::UserRole).toULongLong() == state->mapItem)
            {
                minimapTree_.setCurrentItem(*iter);
                break;
            }
            ++iter;
        }
    }
}

void DecodeDockWidget::updateMapType(QString& mapType)
{
    //if (currCtx_)
    //{
    //    currCtx_->setMinimapType(mapType);
    //
    //    if (currCtx_->decodedLog()->IsMapInitialized())
    //    {
    //        updateUI(UPDATE_MAP);
    //    }
    //}
}

void DecodeDockWidget::updateCurrLog(CrawlerWidget* logView) {}
void DecodeDockWidget::updateUI(int updateType)
{
#if 0
    LOG(logINFO) << "type: " << updateType;

    switch (updateType)
    {

    case UPDATE_LOG:
        minimapTypeCombo_.clear();
        minimapTree_.clear();
        decodedTextBox_.clear();
        statusbar_.clearMessage();
        if (currCtx_)
        {
            if (currCtx_->decodedLog()->IsMapInitialized())
            {
                auto mapType = currCtx_->minimapType();
                minimapTypeCombo_.addItems(currCtx_->decodedLog()->GetMapTypes());
                for (int i = 0; i < minimapTypeCombo_.count(); i++)
                {
                    qInfo() << minimapTypeCombo_.itemText(i) << mapType;
                    if (minimapTypeCombo_.itemText(i) == mapType)
                    {
                        minimapTypeCombo_.setCurrentIndex(i);
                        break;
                    }
                }
            }
            setStatusMessage((currCtx_->decodedLog()->IsMapInitialized() ? "Lod Decoded"
                : "Log NOT decoded"));
        }
        break;
    case UPDATE_MAP:
        decodedTextBox_.clear();
        if (currCtx_ && currCtx_->decodedLog()->IsMapInitialized())
        {
            updatedUIMinimap(currCtx_->decodedLog()->GetMap(currCtx_->minimapType()));
            auto iter = QTreeWidgetItemIterator(&minimapTree_);
            while (*iter)
            {
                if ((*iter)->data(0, Qt::UserRole).toULongLong())
                {
                    minimapTree_.setCurrentItem((*iter));
                    break;
                }
                ++iter;
            }
        }
        break;
    case UPDATE_ITEM:
        if (currCtx_ && currCtx_->decodedLog()->IsMapInitialized())
        {
            decodedTextBox_.setHtml(
                currCtx_->decodedLog()->DecodeItem(currCtx_->selectedItem()));
        }
        break;
    }
#endif
}

void DecodeDockWidget::updateUISelectedItem(uint64_t item) {}

void DecodeDockWidget::handleLineSelectionChanged(LineNumber line) {}
void DecodeDockWidget::handleMinimapTypeChange(int index)
{
    updateMapType(minimapTypeCombo_.itemText(index));
}
void DecodeDockWidget::handleMinimapSelectionChange(QTreeWidgetItem* current,
    QTreeWidgetItem* previous)
{
    //if (currCtx_)
    //{
    //    if (current == nullptr)
    //    {
    //        currCtx_->setSelectedItem(-1);
    //    }
    //    else
    //    {
    //        currCtx_->setSelectedItem(current->data(0, Qt::UserRole).toULongLong());
    //    }
    //    updateUI(UPDATE_ITEM);
    //}
}
void DecodeDockWidget::handleLinkClick(QString& link) {}
void DecodeDockWidget::handleForwardBackwardsClick(bool forward) {}
void DecodeDockWidget::handleDecodeClick() {}

void DecodeDockWidget::MinimapAddItem(MapItem* object, QTreeWidget* parent)
{
    auto item = new QTreeWidgetItem(parent, QStringList() << QString::fromStdString(object->name()));
    item->setData(0, Qt::UserRole, qVariantFromValue(object->id()));
    for (auto ch : object->children())
    {
        MinimapAddItem(ch, item);
    }
}

void DecodeDockWidget::MinimapAddItem(MapItem* object, QTreeWidgetItem* parent)
{
    auto item = new QTreeWidgetItem(parent, QStringList() << QString::fromStdString(object->name()));
    item->setData(0, Qt::UserRole, qVariantFromValue(object->id()));
    for (auto ch : object->children())
    {
        MinimapAddItem(ch, item);
    }
}

void DecodeDockWidget::updateMap(MapTree root)
{
    minimapTree_.blockSignals(true);

    minimapTree_.clear();

    if (root != nullptr)
    {
        for (auto ch : root->children())
        {
            MinimapAddItem(ch, &minimapTree_);
        }
    }
    minimapTree_.blockSignals(false);
}

void DecodeDockWidget::applyOptions()
{
    const auto& config = Configuration::get();
    QFont font = config.mainFont();

    LOG(logDEBUG) << "DecodeDockWidget::applyOptions";

    // Whatever font we use, we should NOT use kerning
    font.setKerning(false);
    font.setFixedPitch(true);
#if QT_VERSION > 0x040700
    // Necessary on systems doing subpixel positionning (e.g. Ubuntu 12.04)
    font.setStyleStrategy(QFont::ForceIntegerMetrics);
#endif

    decodedTextBox_.setFont(font);
}

void DecodeDockWidget::setStatusMessage(const QString& text)
{
    LOG(logINFO) << "update status: " << text;
    statusbar_.showMessage(text);
}
