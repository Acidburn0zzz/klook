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

#include "declarativeviewer.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <kmimetypetrader.h>

#include "video.h"
#include "audio.h"
#include "text.h"
#include "previewgenerator.h"
#include "previewprovider.h"
#include "mimeprovider.h"
#include "filemodel.h"
#include "mimeprovider.h"
#include "workerthread.h"

#include "plasma/windoweffects.h"


#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

static int header_height = 27;
static int border_width = 2;
static int height_offset = 27;
static int arrowIconWidth = 29;
static int arrowIconHeight = 16;

static int min_width = 600;
static int min_height = 427;

DeclarativeViewer::DeclarativeViewer( QWidget* parent )
    : QDeclarativeView( parent )
    , m_lastMousePosition( 0, 0 )
    , m_isSingleMode( true )
    , m_moving( false )
    , m_resize( false )
    , m_startFullScreen( false )
    , m_isEmbedded( false )
    , m_isGallery( false )
    , m_currentFile( 0 )
    , m_region( FRAME_REGION )
    , m_mediaObject( 0 )
    , m_videoWidget( 0 )
    , m_width( min_width )
    , m_height( min_height )
    , m_compositing( false )
    , m_thread( 0 )
{
    setOptimizationFlags( QGraphicsView::DontSavePainterState );
    setViewportUpdateMode( QGraphicsView::BoundingRectViewportUpdate );

    setMouseTracking( true );

    connect( engine(), SIGNAL( quit() ), SLOT( close() ) );

    engine()->addImageProvider( "preview", new PreviewProvider );
    engine()->addImageProvider( "mime", new MimeProvider );

    setResizeMode( QDeclarativeView::SizeRootObjectToView );

    setMinimumWidth( m_width );
    setMinimumHeight( m_height );

    m_previewGenerator = PreviewGenerator::createInstance();

    ListItem* prototypeItem = new ListItem( this );
    m_fileModel = new FileModel( prototypeItem, this );
    PreviewGenerator::createInstance()->setModel( m_fileModel );

    connect( this, SIGNAL( fileData( QVariant, QVariant ) ), m_fileModel, SLOT( append( QVariant, QVariant ) ) );

    setRegisterTypes();

    //Remove standart KDE title
    setWindowFlags( Qt::CustomizeWindowHint | Qt::FramelessWindowHint );

    setAttribute( Qt::WA_TranslucentBackground );
    setAutoFillBackground( false );
    setStyleSheet( "background:transparent;" );

    skipTaskBar();

    setViewMode( Single );

    connect( qApp, SIGNAL( focusChanged( QWidget*, QWidget* ) ), this, SLOT( focusChanged( QWidget*, QWidget* ) ) );

    Plasma::WindowEffects::overrideShadow( this->winId(), true );
    Plasma::WindowEffects::enableBlurBehind( winId() );
}

DeclarativeViewer::~DeclarativeViewer()
{
    qDeleteAll( m_files );

    delete m_thread;
}

void DeclarativeViewer::init( const QStringList& urls, bool embedded, const QRect& rc )
{
    m_isEmbedded = embedded;
    m_rcIcon = rc;
    m_urls = urls;

    m_previewGenerator->setFiles( m_urls );
    rootContext()->setContextProperty( "embedded", m_isEmbedded );

}

//Check whether the KDE effects are included
bool DeclarativeViewer::checkComposite()
{
    QDBusInterface remoteApp( "org.kde.kwin", "/KWin", "org.kde.KWin" );
    QDBusReply<bool> reply = remoteApp.call( "compositingActive" );
    if ( reply.isValid() )
    {
        m_compositing = reply.value();
        return reply.value();
    }
    qDebug() << "DBus reply is not valid";
    m_compositing = false;
    return false;
}

void DeclarativeViewer::setRegisterTypes()
{
    qmlRegisterType<MyVideo>( "Widgets", 1, 0, "Video" );
    qmlRegisterType<MyAudio>( "Widgets", 1, 0, "Audio" );
    qmlRegisterType<MyText>( "Widgets", 1, 0, "PlainText" );

    QDesktopWidget dw;
    QRect r = dw.screenGeometry( this );
    rootContext()->setContextProperty( "DWigth", r.width() );
    rootContext()->setContextProperty( "DHeight", r.height() );
    rootContext()->setContextProperty( "fileModel", m_fileModel );
    rootContext()->setContextProperty( "previewGenerator", PreviewGenerator::createInstance() );
    rootContext()->setContextProperty( "cppObject",  this );
    rootContext()->setContextProperty( "effects", "on" );
    rootContext()->setContextProperty( "actualSize", "off" );
    rootContext()->setContextProperty( "openText", ki18n( "Open in..." ).toString() );
    rootContext()->setContextProperty( "fileName", "" );
    rootContext()->setContextProperty( "fileUrl", "" );
    rootContext()->setContextProperty( "fileType", "Undefined" );
    rootContext()->setContextProperty( "viewMode", "single" );
    rootContext()->setContextProperty( "embedded", m_isEmbedded );
    rootContext()->setContextProperty( "embeddedLayout", "null" );
    rootContext()->setContextProperty( "arrowX", .0 );
    rootContext()->setContextProperty( "arrowY", .0 );
    rootContext()->setContextProperty( "effects", ( checkComposite() ) ? "on" : "off" );

    rootContext()->setContextProperty( "artistStr", ki18n( "Artist:" ).toString() );
    rootContext()->setContextProperty( "totalTimeStr", ki18n( "Total time:" ).toString() );
    rootContext()->setContextProperty( "folderStr", ki18n( "Folder" ).toString() );
    rootContext()->setContextProperty( "lastModifiedStr", ki18n( "Last Modified:" ).toString() );
    rootContext()->setContextProperty( "sizeStr", ki18n( "Size:" ).toString() );
    rootContext()->setContextProperty( "elementsStr", ki18n( "Elements:" ).toString() );
}

void DeclarativeViewer::startWorkingThread()
{
    delete m_thread;
    m_thread = new WorkerThread( m_urls );

    connect( m_thread, SIGNAL( fileProcessed( const File* ) ), SLOT( newFileProcessed( const File* ) ) );
    connect( m_thread, SIGNAL( fail() ), SLOT( showNoFilesNotification() ) );

    m_thread->start();
}

void DeclarativeViewer::createVideoObject( const QString& filePath )
{
    delete m_videoWidget;
    delete m_mediaObject;

    m_mediaObject = new Phonon::MediaObject();
    m_mediaObject->setCurrentSource( QUrl( filePath ) );
    m_videoWidget = new Phonon::VideoWidget();
    m_videoWidget->hide();

    Phonon::createPath( m_mediaObject, m_videoWidget );
    QObject::connect( m_mediaObject, SIGNAL( metaDataChanged() ), this, SLOT( onMetaDataChanged() ) );
}

void DeclarativeViewer::onMetaDataChanged()
{
    if ( m_videoWidget )
    {
        m_width = m_videoWidget->sizeHint().width();
        m_height = m_videoWidget->sizeHint().height();

        delete m_videoWidget;
        m_videoWidget = 0;

        QSize sz = calculateViewSize( QSize( m_width, m_height ) );
        sz.setHeight( sz.height() + ( m_isEmbedded ? 0 : height_offset ) );

        if ( ( m_startFullScreen ) && ( sz == size() ) )
        {
            showFullScreen();
            emit setFullScreenState();
        }
        else
        {
            centerWidget( sz );
        }
    }

    m_startFullScreen = false;
    if ( m_mediaObject )
    {
        QObject::disconnect( m_mediaObject, SIGNAL( metaDataChanged() ), this, SLOT( onMetaDataChanged() ) );

        delete m_mediaObject;
        m_mediaObject = 0;
    }
}

void DeclarativeViewer::setActualSize()
{
    updateSize( m_currentFile );
}

void DeclarativeViewer::setActualSizeParam( bool value )
{
    Q_UNUSED( value )
}

void DeclarativeViewer::setFullScreen()
{
    if ( isFullScreen() )
    {
        showNormal();
        setActualSize();
    }
    else if ( ( geometry().size() == getActualSize() ) || m_isGallery )
    {
        showFullScreen();
        emit setFullScreenState();
    }
    else
    {
        m_startFullScreen = true;
        setActualSize();
    }
}

QSize DeclarativeViewer::getActualSize()
{    
    if ( m_currentFile == 0 )
    {
        return QSize();
    }
    if ( m_currentFile->type() == File::Video )
    {
        return QSize();
    }
    else if ( m_currentFile->type() == File::Image )
    {
        QPixmap pixmap( m_currentFile->name() );
        m_width = pixmap.width();
        m_height = pixmap.height();
        QSize sz = calculateViewSize( QSize( m_width, m_height ) );
        sz.setHeight( sz.height() + ( m_isEmbedded ? 0 : height_offset ) );
        return sz;
    }
    else if ( m_currentFile->type() == File::Audio )
        return QSize( min_width, min_height );
    else if ( m_currentFile->type() == File::Directory )
        return QSize( min_width + 100, min_height );
    else
    {
        QSize size = getTextWindowSize( m_currentFile->name() );
        return size;
    }

    return QSize();
}

QSize DeclarativeViewer::calculateViewSize( const QSize& sz )
{
    QSize szItem = sz;
    QDesktopWidget dw;
    QRect rectDesktop = dw.screenGeometry( this );

    int wDesktop = rectDesktop.width() * 8 / 10;
    int hDesktop = rectDesktop.height() * 8 / 10;

    szItem = inscribedRectToRect( sz, QSize( wDesktop, hDesktop ) );

    szItem.setWidth(qMax(szItem.width(), minimumWidth()));
    szItem.setHeight(qMax(szItem.height(), minimumHeight()));

    return szItem;
}

// function inscribing rect sz1 into rect sz2
QSize DeclarativeViewer::inscribedRectToRect( const QSize& sz1, const QSize& sz2 )
{
    QSize sz = sz1;

    if ( ( sz.height() > sz2.height() ) || ( sz.width() > sz2.width() ) )
    {
        sz.scale( sz2, Qt::KeepAspectRatio );
    }

    return sz;
}


void DeclarativeViewer::showWidget( const QSize& sz )
{
    if ( ( m_startFullScreen ) && ( sz == size() ) )
    {
        showFullScreen();
        emit setFullScreenState();
    }
    else
    {
        centerWidget( sz );
    }
    m_startFullScreen = false;
}

void DeclarativeViewer::updateSize( const File* file )
{    
    if ( file == 0 )
    {
        m_startFullScreen = false;
        return;
    }

    if ( file->type() == File::Video )
    {
        createVideoObject( file->name() );
    }
    else if ( file->type() == File::Image )
    {
        QPixmap pixmap( file->name() );

        m_width = pixmap.width();
        m_height = pixmap.height();

        QSize sz = calculateViewSize( QSize( m_width, m_height ) );
        sz.setHeight( sz.height() + ( m_isEmbedded ? 0 : height_offset ) );
        showWidget( sz );
    }
    else if ( ( file->type() == File::Audio ) ||
              ( file->type() == File::Directory ) )
    {
        int width = min_width;
        int height = min_height;
        if ( file->type() == File::Directory )
            width += 100;

        QSize sz( width, height );
        showWidget( sz );
    }
    else if ( file->type() == File::Txt )
    {
        QSize sz = getTextWindowSize( file->name() );
        showWidget( sz );
    }
}

void DeclarativeViewer::centerWidget( const QSize& sz )
{
    QDesktopWidget dw;
    QRect rectDesktop = dw.screenGeometry( this );
    QSize sz1 = sz;    

    if ( m_isEmbedded )
    {
        int iconOffset = 5;
        int desktopMargin = 70;

        QRect top(rectDesktop.x() + desktopMargin,
                  rectDesktop.y() + desktopMargin ,
                  rectDesktop.width() - 2 * desktopMargin ,
                  m_rcIcon.y() - desktopMargin - iconOffset );

        QRect left(rectDesktop.x() + desktopMargin,
                   rectDesktop.y() + desktopMargin,
                   m_rcIcon.x() - iconOffset - rectDesktop.x() - desktopMargin,
                   rectDesktop.height()- 2 * desktopMargin);

        QRect right(m_rcIcon.topRight().x(),
                    rectDesktop.y() + desktopMargin,
                    rectDesktop.width() - m_rcIcon.topRight().x() - desktopMargin - iconOffset,
                    rectDesktop.height() - 2 * desktopMargin);

        int topArea = top.width()*top.height();
        int leftArea = left.width()*left.height();
        int rightArea = right.width()*right.height();
        qDebug() << top << left << right;
        qDebug() << topArea << leftArea << rightArea;
        if ( ( topArea > leftArea ) && ( topArea > rightArea ) )
        {            
            sz1.setHeight(sz1.height() + arrowIconHeight);
            sz1 = inscribedRectToRect( sz1, top.size() );
            int x = m_rcIcon.x() + m_rcIcon.width()/2 - sz1.width()/2;
            int y = m_rcIcon.y() - iconOffset - sz1.height();
            x = qMax(x , desktopMargin);
            x = qMin(x, top.topRight().x() - sz1.width());
            QRect rect(x,y,sz1.width(),sz1.height());
            rootContext()->setContextProperty( "embeddedLayout", "top" );
            rootContext()->setContextProperty( "arrowX", m_rcIcon.x() + m_rcIcon.width()/2 - arrowIconWidth/2 -rect.x());
            rootContext()->setContextProperty( "arrowY", rect.height() - arrowIconHeight);
            m_posArrow = TOP;            
            setGeometry(rect);
        }
        else if ( leftArea > rightArea )
        {
            sz1.setWidth(sz1.width()+ arrowIconHeight);
            sz1 = inscribedRectToRect( sz1, left.size() );
            int x = m_rcIcon.x() - sz1.width() - iconOffset;
            int y = m_rcIcon.y() + m_rcIcon.height()/2 - sz1.height()/2;
            y = qMax(y , desktopMargin);
            y = qMin(y, left.bottomLeft().y() -sz1.height() );
            QRect rect(x,y,sz1.width(),sz1.height());
            rootContext()->setContextProperty( "embeddedLayout", "left" );
            rootContext()->setContextProperty( "arrowX", rect.width() - arrowIconHeight);
            rootContext()->setContextProperty( "arrowY", m_rcIcon.y() + m_rcIcon.height()/2 - arrowIconWidth/2 - rect.y());
            m_posArrow = RIGHT;
            setGeometry(rect);
        }
        else
        {
            sz1.setWidth(sz1.width()+ arrowIconHeight);
            sz1 = inscribedRectToRect( sz1, right.size() );
            int x = m_rcIcon.topRight().x() + iconOffset;
            int y = m_rcIcon.y() + m_rcIcon.height()/2 - sz1.height()/2;
            y = qMax(y , desktopMargin);
            y = qMin(y, right.bottomLeft().y() - sz1.height() );
            QRect rect(x,y,sz1.width(),sz1.height());
            rootContext()->setContextProperty( "embeddedLayout", "right" );
            rootContext()->setContextProperty( "arrowX", .0);
            rootContext()->setContextProperty( "arrowY", m_rcIcon.y() + m_rcIcon.height()/2 - arrowIconWidth/2 - rect.y());
            m_posArrow = LEFT;        
            setGeometry(rect);
        }
        emit setEmbeddedState();
    }
    else
    {
        int w = sz.width();
        int h = sz.height();

        QRect rect( ( rectDesktop.width() - w ) / 2,
                    ( rectDesktop.height() - h ) / 2,
                    w, h );
        rect.moveTop( rect.y() - height_offset / 2 );

        setGeometry( rect );
    }
}

void DeclarativeViewer::changeContent()
{
    if ( !m_currentFile  ) return;

    ViewMode mode = ( m_files.size() == 1 ) ? Single : Multi;
    setViewMode( mode );

    KService::Ptr ptr = KMimeTypeTrader::self()->preferredService( m_currentFile->mime() );
    if ( ptr.isNull() )
        rootContext()->setContextProperty( "openText", ki18n( "Open" ).toString() );
    else
    {
        KService *serv = ptr.data();
        rootContext()->setContextProperty( "openText",  ( ki18n( "Open in " ).toString() + serv->name() ) );
    }

    QFileInfo fi( m_currentFile->name() );
    rootContext()->setContextProperty( "fileName", fi.fileName() );
    rootContext()->setContextProperty( "fileUrl", m_currentFile->name() );
    rootContext()->setContextProperty( "fileType", File::fileTypeToString( m_currentFile->type() ) );
}

void DeclarativeViewer::updateContent( int index )
{
    if (index == -1)
    {
        rootContext()->setContextProperty( "openText",  ki18n( "Open in..." ).toString() );
        rootContext()->setContextProperty( "fileName",  ki18n( "Elements: " ).toString() + QString::number( m_files.count() ) );
    }
    else
    {
        m_currentFile = m_files.at( index );
        changeContent();
    }
}

WidgetRegion DeclarativeViewer::calculateWindowRegion( const QPoint& mousePos )
{
    /*
      To do:
      add embedded arrow regions
    */

    QPointF pos;
    pos = mousePos;
    QRectF r = rect();
    QRectF headerTitle;
    QRectF header1( 6 + 42 * 2, border_width + 1, 60, header_height - border_width + 1 );
    QRectF header2( r.width() - 42 * 2 - 12 - 6, border_width + 1, 12, header_height- border_width + 1 );
    QRegion headerR;

    if ( m_isSingleMode )
    {
        headerTitle.setRect( border_width + 1,
                             border_width + 1,
                             r.width() - border_width - 270,
                             header_height - border_width + 1 );
        headerR = headerR.united( headerTitle.toRect() );
        headerR = headerR.united( header2.toRect() );
    }
    else
    {
        headerTitle.setRect( 6 + 42 * 3 + 12,
                             border_width + 1,
                             r.width() - 414,
                             header_height - border_width + 1 );
        headerR = headerR.united( headerTitle.toRect() );
        headerR = headerR.united( header1.toRect() );
        headerR = headerR.united( header2.toRect() );
    }

    QRectF topBorder( 0, 0, r.width(), border_width );
    QRectF leftBorder( 0, 0, border_width, r.height() );
    QRectF rightBorder( r.width()-border_width, 0, border_width, r.height() );
    QRectF bottomBorder( 0, r.height()-border_width, r.width(), border_width );

    if ( headerR.contains( pos.toPoint() ) )
    {
        return HEADER_REGION;
    }
    else if ( topBorder.contains( pos ) )
    {
        if ( leftBorder.contains( pos ) )
        {
            return TOP_LEFT_CORNER_REGION;
        }
        else if ( rightBorder.contains( pos ) )
        {
            return TOP_RIGHT_CORNER_REGION;
        }
        else
        {
            return TOP_BORDER_REGION;
        }
    }
    else if ( bottomBorder.contains( pos ) )
    {
        if ( leftBorder.contains( pos ) )
        {
            return BOTTOM_LEFT_CORNER_REGION;
        }
        else if ( rightBorder.contains( pos ) )
        {
            return BOTTOM_RIGHT_CORNER_REGION;
        }
        else
        {
            return BOTTOM_BORDER_REGION;
        }
    }
    else if ( leftBorder.contains( pos ) )
    {
        return LEFT_BORDER_REGION;
    }
    else if ( rightBorder.contains( pos ) )
    {
        return RIGHT_BORDER_REGION;
    }

    return FRAME_REGION;
}

void DeclarativeViewer::mousePressEvent( QMouseEvent* event )
{
    if (!m_isEmbedded)
    {
        if ( !isFullScreen() )
        {
            m_region = calculateWindowRegion( event->pos() );
            if ( event->button() == Qt::LeftButton )
            {
                if ( m_region == HEADER_REGION )
                {
                    m_moving = true;
                    m_lastMousePosition = event->globalPos();
                    setCursor( QCursor( Qt::SizeAllCursor ) );
                }
                else if ( m_region != FRAME_REGION )
                {
                    m_resize = true;
                    m_lastMousePosition = event->globalPos();
                }
            }
            event->accept();
        }
    }
    QDeclarativeView::mousePressEvent(event);
}

void DeclarativeViewer::mouseMoveEvent( QMouseEvent* event )
{
    QDeclarativeView::mouseMoveEvent( event );
    if ( isFullScreen() )
    {
        return;
    }

    if ( m_isEmbedded )
    {
        return;
    }

    if ( event->buttons().testFlag( Qt::LeftButton ) && m_moving )
    {
        if ( viewport()->cursor().shape() != ( Qt::SizeAllCursor ) )
            viewport()->setCursor( QCursor( Qt::SizeAllCursor ) );
        window()->move( window()->pos() + ( event->globalPos() - m_lastMousePosition ) );
        m_lastMousePosition = event->globalPos();
    }
    else if ( event->buttons().testFlag( Qt::LeftButton ) && m_resize )
    {
        QPoint p = mapToGlobal( event->pos()) - geometry().topLeft();
        QPoint offset = event->globalPos() - m_lastMousePosition;
        switch ( m_region )
        {
        case LEFT_BORDER_REGION:
            if ( offset.x() * ( -1 ) + width() > minimumWidth() )
            {
                window()->move( ( window()->pos() + offset ).x(), y() );
                resize( offset.x() * ( -1 ) + width(), height() );
                m_lastMousePosition = event->globalPos();
            }
            break;
        case TOP_BORDER_REGION:
            if (offset.y()*(-1) + height() > minimumHeight())
            {
                window()->move( x(), (window()->pos() + offset).y() );
                resize( width(), offset.y() * ( -1 ) + height() );
                m_lastMousePosition = event->globalPos();
            }
            break;
        case TOP_LEFT_CORNER_REGION:
            if ( offset.x() * ( -1 ) + width() > minimumWidth() )
            {
                window()->move( ( window()->pos() + offset).x(), y() );
                resize( offset.x() * ( -1 ) + width(), height() );
                m_lastMousePosition.setX( event->globalPos().x() );
            }
            if ( offset.y() * ( -1 ) + height() > minimumHeight() )
            {
                window()->move( x(), ( window()->pos() + offset ).y() );
                resize( width(), offset.y() * ( -1 ) + height() );
                m_lastMousePosition.setY( event->globalPos().y() );
            }
            break;
        case TOP_RIGHT_CORNER_REGION:
            resize( p.x(), height() );
            if ( offset.y() * ( -1 ) + height() > minimumHeight() )
            {
                window()->move( x(), ( window()->pos() + offset ).y() );
                resize( width(), offset.y() * ( -1 ) + height() );
                m_lastMousePosition.setY( event->globalPos().y() );
            }
            break;
        case BOTTOM_LEFT_CORNER_REGION:
            resize( width() , p.y() );
            if ( offset.x() * ( -1 ) + width() > minimumWidth() )
            {
                window()->move( ( window()->pos() + offset ).x(), y() );
                resize( offset.x() * ( -1 ) + width(), height() );
                m_lastMousePosition.setX( event->globalPos().x() );
            }
            break;
        case RIGHT_BORDER_REGION:
            resize( p.x(), height() );
            break;
        case BOTTOM_BORDER_REGION:
            resize( width() , p.y() );
            break;
        default:
            resize( p.x(), p.y() );
        }
    }
    else
    {
        m_region = calculateWindowRegion( event->pos() );
        if ( ( m_region == TOP_BORDER_REGION ) || ( m_region == BOTTOM_BORDER_REGION ) )
        {
            if ( viewport()->cursor().shape() != ( Qt::SizeVerCursor ) )
            {
                viewport()->setCursor( QCursor(Qt::SizeVerCursor) );
            }
        }
        else if ( ( m_region == LEFT_BORDER_REGION ) || ( m_region == RIGHT_BORDER_REGION ) )
        {
            if ( viewport()->cursor().shape() != ( Qt::SizeHorCursor ) )
            {
                viewport()->setCursor( QCursor(Qt::SizeHorCursor ) );
            }
        }
        else if ( ( m_region == TOP_LEFT_CORNER_REGION ) || ( m_region == BOTTOM_RIGHT_CORNER_REGION ) )
        {
            if ( viewport()->cursor().shape() != ( Qt::SizeFDiagCursor ) )
            {
                viewport()->setCursor( QCursor( Qt::SizeFDiagCursor ) );
            }
        }
        else if ( ( m_region == TOP_RIGHT_CORNER_REGION ) || ( m_region == BOTTOM_LEFT_CORNER_REGION ) )
        {
            if ( viewport()->cursor().shape() != ( Qt::SizeBDiagCursor ) )
            {
                viewport()->setCursor( QCursor( Qt::SizeBDiagCursor ) );
            }
        }
        else if ( viewport()->cursor().shape() != ( Qt::ArrowCursor ) )
        {
            viewport()->setCursor( QCursor( Qt::ArrowCursor ) );
        }
    }
}

void DeclarativeViewer::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !isFullScreen() )
    {
        if ( m_moving )
        {
            m_moving = false;
            viewport()->setCursor( QCursor( Qt::ArrowCursor ) );
        }

        if ( m_resize )
        {
            m_resize = false;
            viewport()->setCursor( QCursor( Qt::ArrowCursor ) );
        }
        event->accept();
    }
    QDeclarativeView::mouseReleaseEvent( event );
}


void DeclarativeViewer::skipTaskBar()
{
    //Skip Taskbar
    Display* dpy = QX11Info::display();
    Atom state[3];
    state[0] = XInternAtom( dpy, "_NET_WM_STATE_SKIP_PAGER", false );
    state[1] = XInternAtom( dpy, "_NET_WM_STATE_SKIP_TASKBAR", false );
    state[2] = XInternAtom( dpy, "_NET_WM_STATE_STICKY", false );
    XChangeProperty( QX11Info::display(), winId(), XInternAtom( dpy, "_NET_WM_STATE", False ), XA_ATOM, 32, PropModeReplace, ( unsigned char* )state, 3 );
}

void DeclarativeViewer::newFileProcessed( const File *file )
{
    if ( m_files.empty() )
    {
        m_currentFile = const_cast<File *>( file );
        changeContent();
        setActualSize();
        show();
        activateWindow();

        rootContext()->setContextProperty( "viewMode", ( ( m_urls.count() == 1 ) ? "single" : "multi" ) );
        emit setStartWindow();

        skipTaskBar();
    }
    else {
        setViewMode( Multi );
    }

    m_files.append( file );
    emit fileData( QVariant( file->name() ), QVariant( file->type() ) );
}

void DeclarativeViewer::showNoFilesNotification()
{
    close();
}

void DeclarativeViewer::setViewMode( DeclarativeViewer::ViewMode mode )
{
    m_isSingleMode = ( mode == Single );
    rootContext()->setContextProperty( "viewMode", ( ( mode == Single ) ? "single" : "multi" ) );
}

void DeclarativeViewer::restart()
{
    hide();

    qDeleteAll( m_files );
    m_files.clear();

    m_previewGenerator->setFiles( m_urls );

    startWorkingThread();

    m_fileModel->reset();

}

void DeclarativeViewer::onSetGallery( bool isGallery )
{
    m_isGallery = isGallery;
}

QSize DeclarativeViewer::getTextWindowSize(QString url)
{
    QFile f( url );
    if ( !f.open(QIODevice::ReadOnly | QIODevice::Text) )
        return QSize();

    QString text = f.readAll();
    QTextDocument* doc = new QTextDocument( text );
    QSize size =  doc->documentLayout()->documentSize().toSize();
    delete doc;
    size.setWidth( size.width() + 34 );
    size.setHeight( size.height() + 10 );

    QDesktopWidget dw;
    QRect rectDesktop = dw.screenGeometry( this );
    m_width = rectDesktop.width();
    m_height = rectDesktop.height();

    if ( size.width() > m_width * 0.8 )
    {
        size.setWidth( m_width * 0.8 );
    }
    if ( size.height() > m_height * 0.8 )
    {
        size.setHeight( m_height * 0.8 );
    }

    size.setHeight( size.height() + ( m_isEmbedded ? 0 : height_offset ) );

    size.setWidth( qMax(size.width(), minimumWidth()) ) ;
    size.setHeight( qMax(size.height(), minimumHeight()) );

    return size;
}

void DeclarativeViewer::focusChanged( QWidget*, QWidget* now )
{    
    if ( m_isEmbedded )
    {
        if ( !now )
            this->close();
    }   
}

void DeclarativeViewer::setEmbedded( bool state )
{
    if ( state )
    {
        setMinimumSize( 50, 50 );
    }
    else
    {
        setMinimumSize( min_width, min_height );
    }

    m_isEmbedded = state;
}

void DeclarativeViewer::setRectIcon( const QRect& rc )
{
    m_rcIcon = rc;
}

void DeclarativeViewer::setUrls( const QStringList& urls )
{
    m_urls = urls;
}

