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

#include "audio.h"

#include <QFileInfo>

#include <KDE/Phonon/MediaObject>
#include <KDE/Phonon/VideoWidget>
#include <KDE/Phonon/AudioOutput>

MyAudio::MyAudio( QDeclarativeItem* parent )
    : QDeclarativeItem( parent )
    , m_isPreview( false )
    , m_isReady( false )
{
    m_mediaObject = Phonon::createPlayer( Phonon::MusicCategory );
    m_mediaObject->setTickInterval( 1000 );

    QObject::connect( m_mediaObject, SIGNAL( tick( qint64 ) ), SLOT( onTicked( qint64 ) ) );
    QObject::connect( m_mediaObject, SIGNAL( totalTimeChanged( qint64 ) ), SLOT( onTotalTimeChanged( qint64 ) ) );
    QObject::connect( m_mediaObject, SIGNAL( finished() ), SLOT( onFinished() ) );
    QObject::connect( m_mediaObject, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ),
                      SLOT( stateChanged( Phonon::State, Phonon::State ) ) );
}

MyAudio::~MyAudio()
{
    delete m_mediaObject;
}

void MyAudio::setPause()
{
    if ( !isVisible() )
        pause();
    else
        play();
}

bool MyAudio::isReady()
{
    return m_isReady;
}

void MyAudio::setReady( bool b )
{
    m_isReady = b;
    emit ready();
}

void MyAudio::onFinished()
{
    emit playFinished();
    setPosition( 0 );
}

QString MyAudio::source() const
{
    return m_mediaObject->currentSource().url().toString();
}

void MyAudio::setSource( const QString& source )
{
    QUrl url( source );

    m_mediaObject->setCurrentSource( url );
    emit sourceChanged();
}

qlonglong MyAudio::position() const
{
    return m_mediaObject->currentTime();
}

void MyAudio::setPosition( qlonglong pos )
{
    m_mediaObject->seek( pos );
    emit positionChanged();
}

qlonglong MyAudio::totalTime() const
{
    return m_mediaObject->totalTime();
}

QTime MyAudio::duration()
{
    QTime t;
    t = t.addMSecs( m_mediaObject->totalTime() );
    return t;
}

void MyAudio::onTotalTimeChanged( qint64 t )
{
    Q_UNUSED( t )
    emit totalTimeChanged();
}

void MyAudio::onTicked( qint64 t )
{
    emit ticked( QVariant( t ) );
}

bool MyAudio::playing() const
{
    return ( m_mediaObject->state() == Phonon::PlayingState );
}

bool MyAudio::paused() const
{
    return ( m_mediaObject->state() == Phonon::PausedState );
}

void MyAudio::play()
{
    m_mediaObject->play();
}

void MyAudio::pause()
{
    m_mediaObject->pause();
}

void MyAudio::play_or_pause()
{
    if ( playing() )
        pause();
    else
        play();
}

bool MyAudio::isPreview() const
{
    return m_isPreview;
}

void MyAudio::setPreview( bool preview )
{
    m_isPreview = preview;
}

void MyAudio::stateChanged( Phonon::State newState, Phonon::State oldState )
{
    Q_UNUSED( newState )
    if ( oldState == Phonon::LoadingState )
    {
        setReady( true );
    }
}

QString MyAudio::artist()
{
    QString artists;
    QStringList listArtists = m_mediaObject->metaData( "ARTIST" );
    if ( !listArtists.empty() )
        artists = listArtists.join( ", " );

    return artists;
}

QString MyAudio::album()
{
    QString albums;
    QStringList listAlbums = m_mediaObject->metaData( "ALBUM" );
    if ( !listAlbums.empty() )
        albums = listAlbums.join( ", " );

    return albums;
}

QString MyAudio::title()
{
    QString titles;
    QStringList listTitles = m_mediaObject->metaData( "TITLE" );
    if ( !listTitles.empty() )
        titles = listTitles.join( ", " );

    if ( titles.isEmpty() )
    {
        QFileInfo fi( m_mediaObject->currentSource().url().path() );
        titles = fi.fileName();
    }

    return titles;
}

QString MyAudio::composer()
{
    QString composers;
    QStringList listcomposers = m_mediaObject->metaData( "COMPOSER" );
    if ( !listcomposers.empty() )
        composers = listcomposers.join( ", " );

    return composers.toUtf8();
}

QString MyAudio::genre()
{
    QString genres;
    QStringList listgenres = m_mediaObject->metaData( "GENRE" );
    if ( !listgenres.empty() )
        genres = listgenres.join( ", " );

    return genres;
}

