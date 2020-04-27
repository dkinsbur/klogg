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


struct ObjectInfo {
    QString name;
    QString info;
    uint32_t line;
    uint64_t id;
};
Q_DECLARE_METATYPE( ObjectInfo* )

class MinimapObject {
  public:
    static MinimapObject* loadJson( QString path );

    MinimapObject( ObjectInfo& data, MinimapObject* parent );
    MinimapObject();
    ~MinimapObject();
    int count() const;
    MinimapObject* getChild( int i ) const;
    MinimapObject* getParent() const;
    ObjectInfo* getData();
    MinimapObject* addChild( ObjectInfo& data, int index = -1 );

  private:
    QVector<MinimapObject*> _children;
    ObjectInfo _data;
    MinimapObject* _parent;
};
#endif