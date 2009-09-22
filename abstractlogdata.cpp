/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "abstractlogdata.h"

AbstractLogData::AbstractLogData()
{
    maxLength = 0;
}

// Simple wrapper in order to use a clean Template Method
QString AbstractLogData::getLineString(int line) const
{
    return doGetLineString(line);
}

// Simple wrapper in order to use a clean Template Method
int AbstractLogData::getNbLine() const
{
    return doGetNbLine();
}

int AbstractLogData::getMaxLength() const
{
    return maxLength;
}