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

#include <QFile.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

void parseJsonObject( QJsonObject& obj, LogObjectItem* parent)
{
    LogObject o;
    o.line = obj[ "line" ].toInt(0);
    o.name = obj[ "name" ].toString("");
    LogObjectItem* parsedItem = parent->addChild( o );

    auto children = obj[ "children" ].toArray();
    for ( int i=0; i < children.count(); i++ ) {
        parseJsonObject( children.at( i ).toObject(), parsedItem );
    }
}

LogObjectItem* LogObjectItem::loadJson( QString path )
{
    QFile infoFile( path );

    if ( !infoFile.open( QIODevice::ReadOnly ) ) {
        return nullptr;
    }

    QByteArray data = infoFile.readAll();

    QJsonDocument loadDoc( QJsonDocument::fromJson( data ) );

    LogObjectItem* root = new LogObjectItem();
    bool success = false;

    auto children = loadDoc.array();
    for ( int i=0; i < children.count(); i++ ) {
        parseJsonObject( children.at( i ).toObject(), root );
    }

    success = true;

    if ( !success ) {
        delete root;
        root = nullptr;
    }

    return root;
}

LogObjectItem::LogObjectItem( LogObject& data, LogObjectItem* parent )
    : _data( data )
    , _parent( parent )
{
}
LogObjectItem::LogObjectItem()
    : _data()
    , _parent( nullptr )
{
}

LogObjectItem::~LogObjectItem()
{
    qDeleteAll( _children );
}

int LogObjectItem::count() const
{
    return _children.count();
}

LogObjectItem* LogObjectItem::getChild( int i ) const
{
    if ( i < 0 || i >= _children.count() ) {
        return nullptr;
    }
    return _children.at( i );
}

LogObjectItem* LogObjectItem::getParent() const
{
    return _parent;
}

LogObject& LogObjectItem::getData()
{
    return _data;
}

LogObjectItem* LogObjectItem::addChild( LogObject& data, int index )
{
    auto child = new LogObjectItem( data, this );
    if ( index < 0 ) {
        _children.append( child );
    }
    else {
        _children.insert( index, child );
    }
    return child;
}

// class MiniMap {
//  public:
//    MiniMap( QJsonDocument& );
//    ~MiniMap();
//
//  private:
//};
//
// MiniMap::MiniMap( QJsonDocument& json ){ Json }
//
// MiniMap::~MiniMap()
//{
//}
//
//#include <QMessageBox.h>
//TreeWidget::TreeWidget()
//    : QDockWidget()
//    , tree_()
//{
//    QWidget* view = new QWidget();
//    QVBoxLayout* layout = new QVBoxLayout();
//    tree_.setColumnCount( 1 );
//    tree_.setHeaderHidden( true );
//
//    layout->addWidget( &tree_ );
//    layout->setContentsMargins( 1, 1, 1, 1 );
//    view->setLayout( layout );
//
//   // connect( &tree_, &QTreeWidget::itemDoubleClicked, [this]( QTreeWidgetItem* item, int column ) {
//   //     if ( item != nullptr ) {
//   //         JumpTo( item->data( 0, Qt::UserRole ).toInt() );
//   //     }
//   // } );
//    connect( &tree_, &QTreeWidget::itemSelectionChanged, [this]() {
//        auto item = tree_.currentItem();
//        if ( item != nullptr ) {
//            JumpTo( item->data( 0, Qt::UserRole ).toInt() );
//        }
//    } );
//
//    setWidget( view );
//    setFeatures( DockWidgetMovable | DockWidgetFloatable | DockWidgetClosable );
//    setWindowTitle( tr( "Log Layout" ) );
//    
//    auto root = LogObjectItem::loadJson( QStringLiteral("C:\\Users\\dkinsbur\\git\\klogg\\build\\src\\app\\test.json" ) );
//    UpdateTreeInfo( *root );
//}
//TreeWidget::~TreeWidget() {}

void DecodeDockWidget::AddLogObject( LogObjectItem* object, QTreeWidget* parent )
{
    auto data = object->getData();
    auto item = new QTreeWidgetItem( parent, QStringList() << data.name );
    item->setData( 0, Qt::UserRole, data.line );
    for ( int i = 0; i < object->count(); i++ ) {
        AddLogObject( object->getChild( i ), item );
    }
}

void DecodeDockWidget::AddLogObject( LogObjectItem* object, QTreeWidgetItem* parent )
{
    auto data = object->getData();
    auto item = new QTreeWidgetItem( parent, QStringList() << data.name );
    item->setData( 0, Qt::UserRole, data.line );
    for ( int i = 0; i < object->count(); i++ ) {
        AddLogObject( object->getChild( i ), item );
    }
}

void DecodeDockWidget::UpdateTreeInfo( LogObjectItem& root )
{

    tree_.clear();

    for ( int i = 0; i < root.count(); i++ ) {
        AddLogObject( root.getChild( i ), &tree_ );
    }
    //    tree_.insertTopLevelItems( 0, items );
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

    tree_.setColumnCount( 1 );
    tree_.setHeaderHidden( true );
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
            JumpTo( item->data( 0, Qt::UserRole ).toInt() );
        }
    } );

    auto root = LogObjectItem::loadJson(
        QStringLiteral( "C:\\Users\\dkinsbur\\git\\klogg\\build\\src\\app\\test.json" ) );
    UpdateTreeInfo( *root );
}

DecodeDockWidget::~DecodeDockWidget()
{
    process_.close();
}

void DecodeDockWidget::onFinish( int exitCode, QProcess::ExitStatus exitStatus )
{
    if ( exitCode == 0 ) {
        QString p_stdout = process_.readAll();
        decodedTextBox_.setHtml( p_stdout );
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

void DecodeDockWidget::parseLine()
{
    auto args = QStringList() << "klogg.py"
                              << "parse-line" << comboBox_.currentText() << currStr_;
    process_.start( QString( "python" ), args );
}
