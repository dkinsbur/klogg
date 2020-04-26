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

// class TreeItem {
//  public:
void TreeItem::Add( TreeItem* child )
{
    children.append( child );
}
int TreeItem::Count() const
{
    return children.count();
}
TreeItem* TreeItem::Child( int i ) const
{
    if ( i >= 0 && i < children.count() ) {
        return children.at( i );
    }
    return NULL;
}

//  private:
//    QVector<TreeItem*> children;
//    TreeItemData data;
//};
#include <QMessageBox.h>
TreeWidget::TreeWidget()
    : QDockWidget()
    , tree_()
{
    QWidget* view = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    tree_.setColumnCount( 1 );
    tree_.setHeaderHidden( true );
    QList<QTreeWidgetItem*> items;

    auto boot1 = new QTreeWidgetItem( (QTreeWidget*)0, QStringList() << "boot 1" );
    auto boot2 = new QTreeWidgetItem( (QTreeWidget*)0, QStringList() << "boot 2" );
    auto boot3 = new QTreeWidgetItem( (QTreeWidget*)0, QStringList() << "boot 3" );
    items << boot1 << boot2 << boot3;
    boot1->setData( 0, Qt::UserRole, 5 );
    boot2->setData( 0, Qt::UserRole, 50 );
    boot3->setData( 0, Qt::UserRole, 100 );
    tree_.insertTopLevelItems( 0, items );

    layout->addWidget( &tree_ );
    layout->setContentsMargins( 1, 1, 1, 1 );
    view->setLayout( layout );

    connect( &tree_, &QTreeWidget::itemDoubleClicked, [this]( QTreeWidgetItem* item, int column ) {
        if ( item != nullptr ) {

            //QMessageBox msgBox;
            //msgBox.setText( QString::number(item->data( 0, Qt::UserRole ).toInt() ) );
            //msgBox.exec();
            JumpTo( item->data( 0, Qt::UserRole ).toInt() );
        }
    } );

    setWidget( view );
    setFeatures( DockWidgetMovable | DockWidgetFloatable | DockWidgetClosable );
    setWindowTitle( tr( "Log Layout" ) );
}
TreeWidget::~TreeWidget() {}
void TreeWidget::UpdateTreeInfo( TreeItem& root ) {}

DecodeDockWidget::DecodeDockWidget()
    : QDockWidget()
    , decodedTextBox_()
    , comboBox_()
    , process_()
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

    layout->addWidget( &comboBox_ );
    layout->addWidget( &decodedTextBox_ );
    layout->setContentsMargins( 1, 1, 1, 1 );
    view->setLayout( layout );

    setWidget( view );
    setFeatures( DockWidgetMovable | DockWidgetFloatable | DockWidgetClosable );
    setWindowTitle( tr( "Log Info" ) );
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
