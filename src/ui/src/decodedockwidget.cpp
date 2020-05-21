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
#include <QMessageBox>

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
    connect(dl.get(), &DecodedLog::requestedPythonInUse, this, &DecodeManager::updatePythonBusy);
    connect(dl.get(), &DecodedLog::DecodeComplete, this, &DecodeManager::updateDecodeComplete);
    connect(dl.get(), &DecodedLog::DecodeProgressUpdated, this, &DecodeManager::updateDecodeLogProgress);


    state->decoded = dl->IsDecoded();
    state->mapTypes = dl->GetMapTypes();
    state->mapType = state->mapTypes.isEmpty() ? "" : state->mapTypes.first();
    state->map = state->mapTypes.isEmpty() ? MapTree() : dl->GetMap(state->mapType);


    {
        QMutexLocker locker(&lock_);
        logs_[logView] = dl;
        states_[logView] = state;
    }

}

void DecodeManager::updateDecodeLogProgress(int progress)
{
    progress = (progress < 0) ? 0 : progress;
    progress = (progress > 100) ? 100 : progress;
    auto _sender = sender();

    LOG(logINFO) << "update progress of DL: " << _sender << " progress: " << progress;

    bool stateChanged = false;
    shared_ptr<DecodedWidgetState> state;

    {
        QMutexLocker locker(&lock_);
        for (auto k : logs_.keys())
        {
            if ((void*)logs_[k].get() == _sender)
            {
                state = states_[k];
                state->progress = progress;
                if (k == currLog_)
                {
                    stateChanged = true;
                }
                break;
            }
        }
    }
    
    if (stateChanged)
    {
        LOG(logINFO) << "sender matches current log - updating UI progress. log:"<<currLog_;

        emit widgetStateChanged(state);
    }

}

void DecodeManager::updateDecodeComplete(bool success)
{

    bool stateChanged = false;
    shared_ptr<DecodedWidgetState> state;
    shared_ptr<DecodedLog> dl;
    QString logName;

    auto _sender = sender();
    
    LOG(logINFO) << "update completion of DL: " << _sender << " success: " << success;

    {
        QMutexLocker locker(&lock_);
        for (auto k : logs_.keys())
        {
            if ((void*)logs_[k].get() == _sender)
            {
                logName = static_cast<CrawlerWidget*>(k)->logFileName();
                state = states_[k];
                dl = logs_[k];
                state->progress = -1;
                state->decoded = dl->IsDecoded();
                state->mapTypes = dl->GetMapTypes();
                state->mapType = state->mapTypes.isEmpty() ? "" : state->mapTypes.first();
                state->map = state->mapTypes.isEmpty() ? MapTree() : dl->GetMap(state->mapType);
                
                if (k == currLog_)
                {
                    stateChanged = true;
                }
                break;
            }
        }
        state->pythonBusy = false;

    }

    if (!logName.isEmpty())
    {
        emit decodeComplete(success, logName);
    }

    if (stateChanged)
    {
        LOG(logINFO) << "sender matches current log - updating UI completion. log:" << currLog_;

        emit widgetStateChanged(state);
    }
}

void DecodeManager::updatePythonBusy()
{
    bool stateChanged = false;
    shared_ptr<DecodedWidgetState> state;
    shared_ptr<DecodedLog> dl;

    {
        QMutexLocker locker(&lock_);

        if (!getCurrents(&state, &dl))
        {
            return;
        }

        if (!state->pythonBusy)
        {
            state->pythonBusy = true;
            stateChanged = true;
        }
    }

    if (stateChanged)
    {
        emit widgetStateChanged(state);
    }

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

bool DecodeManager::getCurrents(shared_ptr<DecodedWidgetState>* state, shared_ptr<DecodedLog>* dl)
{
    if (!logs_.contains(currLog_))
    {
        return false;
    }

    *state = states_[currLog_];
    *dl = logs_[currLog_];

    return true;
}
void DecodeManager::generateLogMap(const QString& mapType)
{
    bool stateChanged = false;
    shared_ptr<DecodedWidgetState> state;
    shared_ptr<DecodedLog> dl;

    {
        QMutexLocker locker(&lock_);

        if (!getCurrents(&state, &dl))
        {
            return;
        }

        if (mapType != state->mapType)
        {
            stateChanged = true;
            state->mapType = mapType;
            state->map = dl->GetMap(mapType);
        }

        state->pythonBusy = false;
    }

    if (stateChanged)
    {
        emit widgetStateChanged(state);
    }
}
void DecodeManager::decodeLine(LineNumber line, QString lineText)
{
    bool stateChanged = false;
    shared_ptr<DecodedWidgetState> state;
    shared_ptr<DecodedLog> dl;
    {
        QMutexLocker locker(&lock_);
        
        if (!getCurrents(&state, &dl))
        {
            return;
        }

        //if (line != state->line)
        {
            state->line = line;
            state->lineInfo = dl->DecodeLine(lineText);

            stateChanged = true;
        }

        state->pythonBusy = false;

    }

    if (stateChanged)
    {
        emit widgetStateChanged(state);
    }
}

void DecodeManager::decodeMapItem(uint64_t itemId)
{
    bool stateChanged = false;
    shared_ptr<DecodedWidgetState> state;
    shared_ptr<DecodedLog> dl;
    QList<uint32_t> lines;

    {
        QMutexLocker locker(&lock_);
        if (!getCurrents(&state, &dl))
        {
            return;
        }

        //if (itemId != state->mapItem)
        {
            state->mapItem = itemId;
            state->itemInfo = dl->DecodeItem(itemId);
            lines = dl->ItemLines(itemId);

            stateChanged = true;
        }
        state->pythonBusy = false;
    }

    if (stateChanged)
    {
        emit widgetStateChanged(state);
        if (!lines.isEmpty())
        {
            emit foundMatchingLine(LineNumber(lines[0]));
        }
    }

}
void DecodeManager::decodeLog()
{
    shared_ptr<DecodedWidgetState> state;
    shared_ptr<DecodedLog> dl;

    {
        QMutexLocker locker(&lock_);
        if (!getCurrents(&state, &dl))
        {
            return;
        }

    }
    
    dl->Decode();
}
void DecodeManager::decodeLogAbort()
{
    shared_ptr<DecodedWidgetState> state;
    shared_ptr<DecodedLog> dl;

    {
        QMutexLocker locker(&lock_);
        if (!getCurrents(&state, &dl))
        {
            return;
        }
    }
}
void DecodeManager::openLink(const QUrl& link)
{
    bool stateChanged = false;
    bool lineChanged = false;
    uint32_t line;
    shared_ptr<DecodedWidgetState> state;
    shared_ptr<DecodedLog> dl;
    {
        QMutexLocker locker(&lock_);
     
        if (!getCurrents(&state, &dl))
        {
            return;
        }
        
        QStringList args = link.toString().split(':');
        if (args.length() != 2)
        {
            return;
        }

        if (args[0] == "id")
        {
            bool ok = false;
            uint64_t itemId = args[1].toULongLong(&ok);
            if (!ok)
            {
                return;
            }
            state->mapItem = itemId;
            stateChanged = true;
        }
        else if (args[0] == "line")
        {
            bool ok = false;
            line = args[1].toULong(&ok);
            if (!ok)
            {
                return;
            }
            lineChanged = true;
        }
        else
        {
            return;
        }
    }

    if (lineChanged)
    {
        emit foundMatchingLine(LineNumber(line));
    }

    if (stateChanged)
    {
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
            auto dl = logs_[logView];
            disconnect(dl.get(), &DecodedLog::DecodeProgressUpdated, this, &DecodeManager::updateDecodeLogProgress);
            states_.remove(logView);
            logs_.remove(logView);

        }
    }
}

Q_DECLARE_METATYPE(shared_ptr<DecodedWidgetState>)

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void DecodeDockWidget::checkDoDecode()
{
    bool doDecode = false;

    if (currState_.decoded)
    {
        QMessageBox m;
        m.setText("The log is already decoded.");
        m.setInformativeText("Do you want to re-parse the log?");
        m.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        m.setDefaultButton(QMessageBox::No);

       
        int ret = m.exec();
        if (ret == QMessageBox::Yes)
        {
            doDecode = true;
        }
    }
    else
    {
        doDecode = true;
    }

    if (doDecode)
    {
        emit decodeRequested();
    }
}

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

    decodeProgressBar_.setMinimum(0);
    decodeProgressBar_.setMaximum(100);

    decodeBtn_.setText("Decode");
    historyBackwardsBtn_.setText("<--");
    historyForwardBtn_.setText("-->");
    historyLayout_.addWidget(&decodeBtn_);
    historyLayout_.addWidget(&historyBackwardsBtn_);
    historyLayout_.addWidget(&historyForwardBtn_);
    historyLayoutWidget_.setLayout(&historyLayout_);
    mainLayout_.addWidget(&historyLayoutWidget_);
    mainLayout_.addWidget(&statusbar_);
    statusbar_.addPermanentWidget(&decodeStatusLabel_);
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
    connect(
        &decodedTextBox_, &QTextBrowser::anchorClicked, 
        &decMgr_, &DecodeManager::openLink,
        Qt::QueuedConnection);

    connect(
        &historyBackwardsBtn_, &QPushButton::clicked,
        &decodedTextBox_, &QTextBrowser::backward);

    connect(
        &historyForwardBtn_, &QPushButton::clicked,
        &decodedTextBox_, &QTextBrowser::forward);

    connect(
        &decodeBtn_, &QPushButton::clicked, 
        this, &DecodeDockWidget::checkDoDecode
    );

    connect(
        this, &DecodeDockWidget::decodeRequested,
        &decMgr_, &DecodeManager::decodeLog,
        Qt::QueuedConnection
    );

    connect(
        this, &DecodeDockWidget::decodeRequested,
        &decMgr_, &DecodeManager::decodeLog,
        Qt::QueuedConnection
    );

    connect(
        &decMgr_, &DecodeManager::decodeComplete,
        this, &DecodeDockWidget::checkDecodeFailure,
        Qt::QueuedConnection
    );

}

void DecodeDockWidget::checkDecodeFailure(bool success, QString logName)
{
    if (!success)
    {
        QMessageBox m;
        m.setIcon(QMessageBox::Critical);
        m.setText(QString("Log decoding failed for:\n") + logName);
        m.exec();
    }
}



DecodeManager& DecodeDockWidget::getDecodeManager()
{
    return decMgr_;
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

    auto index = currState_.mapTypes.indexOf(currState_.mapType);

    decodedTextBox_.setHtml(currState_.itemInfo + "<br><br>" + currState_.lineInfo);

    minimapTypeCombo_.blockSignals(true);
    minimapTypeCombo_.clear();
    minimapTypeCombo_.addItems(currState_.mapTypes);
    if (index >= 0)
    {
        minimapTypeCombo_.setCurrentIndex(index);
    }
    minimapTypeCombo_.blockSignals(false);
    
    
    if (prev.map != currState_.map)
    {
        updateMap(currState_.map);
    }

    if (prev.mapItem != currState_.mapItem)
    {
        auto iter = QTreeWidgetItemIterator(&minimapTree_);
        while (*iter)
        {
            if ((*iter)->data(0, Qt::UserRole).toULongLong() == currState_.mapItem)
            {
                minimapTree_.setCurrentItem(*iter);
                break;
            }
            ++iter;
        }
    }

    if (currState_.progress <= 0)
    {
        statusbar_.removeWidget(&decodeProgressBar_);
        //if (currState_.pythonBusy)
        //{
        //    statusbar_.showMessage("Python is Busy - cannot query info");
        //}
        {
            decodeStatusLabel_.setText(currState_.decoded ? "Log is Decoded" : "Log is NOT decoded");
        }

    }
    else
    {
        decodeStatusLabel_.setText("Log is being decoded");
        statusbar_.addWidget(&decodeProgressBar_);
        decodeProgressBar_.show();
        decodeProgressBar_.setValue(currState_.progress);
    }
}

void DecodeDockWidget::updateCurrLog(CrawlerWidget* logView) {}
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
