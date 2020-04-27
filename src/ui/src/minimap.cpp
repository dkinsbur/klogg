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

#include <QFile.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <qdebug.h>
#include "minimap.h"

void parseJsonObject( QJsonObject& obj, MinimapObject* parent )
{
    ObjectInfo o;
    o.id = obj[ "id" ].toDouble( 0 );
    o.line = obj[ "line" ].toInt( 0 );
    o.name = obj[ "name" ].toString( "" );
    o.info = obj[ "info" ].toString( "" );
    MinimapObject* parsedItem = parent->addChild( o );

    auto children = obj[ "children" ].toArray();
    for ( int i = 0; i < children.count(); i++ ) {
        parseJsonObject( children.at( i ).toObject(), parsedItem );
    }
}

MinimapObject* MinimapObject::loadJson( QString path )
{
    QFile infoFile( path );
    MinimapObject* root = new MinimapObject();

    if ( !infoFile.open( QIODevice::ReadOnly ) ) {
        return root;
    }

    QByteArray data = infoFile.readAll();

    QJsonDocument loadDoc( QJsonDocument::fromJson( data ) );
    bool success = false;

    auto children = loadDoc.array();
    for ( int i = 0; i < children.count(); i++ ) {
        parseJsonObject( children.at( i ).toObject(), root );
    }

    return root;
}

MinimapObject::MinimapObject( ObjectInfo& data, MinimapObject* parent )
    : _data( data )
    , _parent( parent )
{
}
MinimapObject::MinimapObject()
    : _data()
    , _parent( nullptr )
{
}

MinimapObject::~MinimapObject()
{
    qDeleteAll( _children );
}

int MinimapObject::count() const
{
    return _children.count();
}

MinimapObject* MinimapObject::getChild( int i ) const
{
    if ( i < 0 || i >= _children.count() ) {
        return nullptr;
    }
    return _children.at( i );
}

MinimapObject* MinimapObject::getParent() const
{
    return _parent;
}

ObjectInfo* MinimapObject::getData()
{
    return &_data;
}

MinimapObject* MinimapObject::addChild( ObjectInfo& data, int index )
{
    auto child = new MinimapObject( data, this );
    if ( index < 0 ) {
        _children.append( child );
    }
    else {
        _children.insert( index, child );
    }
    return child;
}
