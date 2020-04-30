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
#include <QProcess>

void DecodeDockWidget::AddLogObject( MinimapObject* object, QTreeWidget* parent )
{
    auto data = object->getData();
    auto item = new QTreeWidgetItem( parent, QStringList() << data->name );
    item->setData( 0, Qt::UserRole, qVariantFromValue( data ) );
    for ( int i = 0; i < object->count(); i++ ) {
        AddLogObject( object->getChild( i ), item );
    }
}

void DecodeDockWidget::AddLogObject( MinimapObject* object, QTreeWidgetItem* parent )
{
    auto data = object->getData();
    auto item = new QTreeWidgetItem( parent, QStringList() << data->name );
    item->setData( 0, Qt::UserRole, qVariantFromValue( data ) );
    for ( int i = 0; i < object->count(); i++ ) {
        AddLogObject( object->getChild( i ), item );
    }
}

void DecodeDockWidget::updatedMinimap( MinimapObject* root )
{

    tree_.clear();
    updateMinimapObjectInfo( QStringLiteral( "" ) );

    if ( root == NULL ) {
        return;
    }

    for ( int i = 0; i < root->count(); i++ ) {
        AddLogObject( root->getChild( i ), &tree_ );
    }
}
void DecodeDockWidget::selectTreeItemById( uint64_t id )
{
    auto iter = QTreeWidgetItemIterator( &tree_ );
    while ( *iter ) {
        auto data = ( *iter )->data( 0, Qt::UserRole ).value<ObjectInfo*>();
        if ( data->id == id ) {
            tree_.setCurrentItem( ( *iter ) );
            return;
        }
        ++iter;
    }
}

DecodeDockWidget::DecodeDockWidget()
    : QDockWidget()
    , decodedTextBox_()
    , comboBox_()
    , process_()
    , tree_()
{
    QString command( "python" );
    QStringList params = QStringList() << "-c"
                                       << "import klogg; klogg.projects()";

    process_.start( command, params );
    process_.waitForFinished();
    QString p_stdout = process_.readAll();
    process_.close();

    comboBox_.addItems( p_stdout.split( " " ) );

    decodedTextBox_.setReadOnly( true );
    decodedTextBox_.setLineWrapMode( QTextEdit::NoWrap );

    QPalette pallet = decodedTextBox_.palette();
    pallet.setColor( QPalette::Base, Qt::lightGray );
    decodedTextBox_.setPalette( pallet );
    //    decodedTextBox_.setTextColor(Qt::yellow);
    decodedTextBox_.setAutoFillBackground( true );

    connect( &comboBox_, SIGNAL( currentTextChanged( const QString& ) ), this,
             SLOT( updateProjectHandler( const QString& ) ) );
    connect( &process_, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), this,
             &DecodeDockWidget::onFinish );

    QWidget* view = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();

    decodedTextBox_.setOpenExternalLinks( false );
    decodedTextBox_.setOpenLinks( false );

    tree_.setColumnCount( 1 );
    tree_.setHeaderHidden( true );
    tree_.setSelectionMode( QAbstractItemView::SingleSelection );
    layout->addWidget( &tree_ );

    layout->addWidget( &comboBox_ );
    layout->addWidget( &decodedTextBox_ );
    layout->setContentsMargins( 1, 1, 1, 1 );
    view->setLayout( layout );

    setWidget( view );
    setFeatures( DockWidgetMovable | DockWidgetFloatable | DockWidgetClosable );
    setWindowTitle( tr( "Log Info" ) );

    connect( &tree_, &QTreeWidget::itemSelectionChanged, [this]() {
        auto item = tree_.currentItem();
        if ( item != nullptr ) {
            auto data = item->data( 0, Qt::UserRole ).value<ObjectInfo*>();
            updateMinimapObjectInfo( data->info );
            emit MinimapObjectChanged( data->line );
        }
    } );
    connect( &decodedTextBox_, &QTextBrowser::anchorClicked, [this]( const QUrl& link ) {
        bool ok;
        uint64_t id = link.toString().toULongLong( &ok );
        if ( ok ) {
            selectTreeItemById( id );
        }
    } );
}

DecodeDockWidget::~DecodeDockWidget()
{
    process_.close();
}

void DecodeDockWidget::updateMinimapObjectInfo( QString& info )
{
    minimapInfo_ = info;
    updatedHtml();
}

void DecodeDockWidget::onFinish( int exitCode, QProcess::ExitStatus exitStatus )
{
    if ( exitCode == 0 ) {
        QString p_stdout = process_.readAll();
        parsedLine_ = p_stdout;
        updatedHtml();
    }

    process_.close();
}

void DecodeDockWidget::updateTextHandler( int index, QString text )
{
    currStr_ = text;
    parseLine();
}

void DecodeDockWidget::updateProjectHandler( const QString& proj )
{
    parseLine();
}

void DecodeDockWidget::applyOptions()
{
    const auto& config = Configuration::get();
    QFont font = config.mainFont();

    LOG( logDEBUG ) << "DecodeDockWidget::applyOptions";

    // Whatever font we use, we should NOT use kerning
    font.setKerning( false );
    font.setFixedPitch( true );
#if QT_VERSION > 0x040700
    // Necessary on systems doing subpixel positionning (e.g. Ubuntu 12.04)
    font.setStyleStrategy( QFont::ForceIntegerMetrics );
#endif

    decodedTextBox_.setFont( font );
    comboBox_.setFont( font );
}

void DecodeDockWidget::updatedHtml() {
    decodedTextBox_.setHtml( minimapInfo_ + "<br><br>" + parsedLine_ );
}

void DecodeDockWidget::parseLine()
{
    parsedLine_ = "";
    updatedHtml();

    auto args = QStringList() << "klogg.py"
                              << "parse-line" << comboBox_.currentText() << currStr_;
    process_.start( QString( "python" ), args );
}
