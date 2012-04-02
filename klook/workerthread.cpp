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

#include "workerthread.h"

#include <QMetaType>
#include <QDir>
#include <QImageReader>
#include <KDE/Phonon/BackendCapabilities>

#include <kmimetype.h>

WorkerThread::WorkerThread( const QStringList& urls, QObject *parent )
    : QThread( parent )
    , isFound( false )
    , urls( urls )
    , supportedImageFormats( QImageReader::supportedImageFormats() )
{
    m_mimeTypes = Phonon::BackendCapabilities::availableMimeTypes();
}

void WorkerThread::run()
{
    processUrl( urls );
    if ( !isFound )
        emit fail();
}

void WorkerThread::processUrl( const QStringList &urls )
{
    QString path;

    foreach ( const QString& str, urls )
    {
        if ( !QDir::isAbsolutePath( str ) )
            path = QDir::currentPath() + "/" + str;
        else
            path = str;

        if ( QFile::exists( path ) )
        {
            QString mime = getMime( str );
            File::FileType type = getType( mime );
            if ( type != File::Undefined )
            {
                const File *file = new File( path, type, mime );
                emit fileProcessed(file);
                isFound = true;
            }
            else
                qWarning() << "Unsupported file " << str;
        } else
            qWarning() << "File not found " << str;
    }
}

QString WorkerThread::getMime( const QString& st ) const
{
    KMimeType::Ptr ptr =  KMimeType::findByFileContent( st );
    KMimeType* type = ptr.data();
    qDebug() << "getMime(): " << type->name();
    return type->name();
}

File::FileType WorkerThread::getType( const QString& mime ) const
{
    int delimiter = mime.indexOf( '/' );

    File::FileType type = File::Undefined;
    if ( delimiter != -1 )
    {
        QString left = mime.left( delimiter );

        if ( left == QLatin1String( "image" ) )
        {
            QString right = mime.mid(delimiter + 1);
            if(supportedImageFormats.contains( right.toLatin1()) || right == "x-xpixmap" )
                type = File::Image;
            else if ( supportedImageFormats.contains( "svg" ) && ( right == ( "svg+xml" ) || right == ( "svg+xml-compressed" ) ) )
                type = File::Image;
        }
        else if ( left == QLatin1String( "video" ) )
        {
            QString right = mime.mid( delimiter + 1 );
#if 1
            if ( m_mimeTypes.contains( mime ) )
                type = File::Video;

            if ( type == File::Undefined )
            {
                if ( right == QLatin1String( "3gpp" ) ||
                     right == QLatin1String( "mp4" ) ||
                     right == QLatin1String( "x-theora+ogg" ) )
                    type = File::Video;
            }
#else
            if ( right == QLatin1String("x-msvideo") ||
                 right == QLatin1String("mp4") ||
                 right == QLatin1String("3gpp") ||
                 right == QLatin1String("x-flv") ||
                 right == QLatin1String("quicktime") )
                type = File::Video;
#endif
        }
        else if ( left == QLatin1String( "text" ) ||
                  mime == QLatin1String( "application/x-zerosize" ) ||
                  mime == QLatin1String( "application/x-shellscript" ) )
            type = File::Txt;
        else if( mime.mid(delimiter + 1) == QLatin1String("directory"))
        {
            type = File::Directory;
        }

    }

    if ( type == File::Undefined )
    {
        if ( mime == QLatin1String( "application/x-matroska" ) ||
             mime == QLatin1String( "application/vnd.rn-realmedia" ) )
            type = File::Video;
        else if( mime == QLatin1String( "bmp" ) )
            type = File::Image;
    }

    return type;
}
