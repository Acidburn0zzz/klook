/* KLook
 * Copyright (c) 2011-2012 ROSA  <support@rosalab.ru>
 * Authors: Julia Mineeva, Evgeniy Auzhin, Sergey Borovkov.
 * License: GPLv3
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 3,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FILE_H
#define FILE_H

#include <QtCore/QObject>
#include <QtCore/QUrl>

class File : public QObject
{
    Q_OBJECT
    Q_ENUMS(FileType)
public:
    enum FileType
    {
        Undefined = 0,
        Image,
        Video,
        Txt,
        Directory,
        Audio,
        OkularFile
    };

    File( QObject* parent );
    File( QUrl url = QUrl(), FileType type = Undefined, const QString& mime = "", QObject* parent = 0 );

    QUrl url() const;
    void setUrl( QUrl url );

    File::FileType type() const;
    void setType(FileType type);

    QString mime() const;
    void setMime(const QString &mime);


private:
    QUrl m_url;
    FileType m_type;
    QString m_mime;

};

#endif // FILE_H
