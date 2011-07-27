/*
   Copyright 2005-2009 Last.fm Ltd.
      - Primarily authored by Jono Cole and Doug Mansell

   This file is part of the Last.fm Desktop Application Suite.

   lastfm-desktop is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   lastfm-desktop is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with lastfm-desktop.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef METADATA_WIDGET_H_
#define METADATA_WIDGET_H_

#include <lastfm/Track>

#include "lib/unicorn/StylableWidget.h"

class DataListWidget;
class HttpImageWidget;
class QLabel;
class QGroupBox;
class QTextBrowser;

#include <QTextBrowser>
#include <QTextObjectInterface>
#include <QEventLoop>
#include <QApplication>

class WidgetTextObject : public QObject, QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    WidgetTextObject(){};
    virtual QSizeF intrinsicSize(QTextDocument*, int /*posInDocument*/,
                                 const QTextFormat& format) {
        QWidget* widget = qVariantValue<QWidget*>(format.property(1));
        QSize margin( 20, 10 );
        return QSizeF( widget->size() + margin );
    }
    virtual void drawObject(QPainter *painter, const QRectF &rect,
           QTextDocument * /*doc*/, int /*posInDocument*/,
           const QTextFormat &format) {
        QWidget* widget = qVariantValue<QWidget*>(format.property( 1 ));
        widget->render( painter, QPoint( 0, 0 ));
        m_widgetRects[widget] = rect;
    }

    QWidget* widgetAtPoint( const QPoint& p ) {
        QMapIterator<QWidget*, QRectF> i(m_widgetRects);
        while (i.hasNext()) {
            i.next();
            if( i.value().contains(p))
                return i.key();
        }
        return 0;
    }

    QRectF widgetRect( QWidget* w ) {
        return m_widgetRects[w];
    }

protected:
    QMap<QWidget*, QRectF> m_widgetRects;
};


class TB : public QTextBrowser {
    public:
        TB( QWidget* p ) : QTextBrowser( p ), currentHoverWidget(0){
            m_widgetTextObject = new WidgetTextObject;
            document()->documentLayout()->registerHandler( WidgetImageFormat, m_widgetTextObject );
        }
        
        void insertWidget( QWidget* w ) {
            QTextImageFormat widgetImageFormat;
            w->installEventFilter( this );
            widgetImageFormat.setObjectType( WidgetImageFormat );
            widgetImageFormat.setProperty( WidgetData, QVariant::fromValue<QWidget*>( w ) );
            
            QTextCursor cursor = textCursor();
 
            cursor.insertImage( widgetImageFormat, QTextFrameFormat::FloatLeft );
            setTextCursor( cursor );
        }

        bool eventFilter( QObject* o, QEvent* e ) {
            QWidget* w = qobject_cast<QWidget*>( o );
            if( e->type() == QEvent::Paint ||
                e->type() == QEvent::Resize ) {
                update( m_widgetTextObject->widgetRect(w).toRect());
            }

            return false;
        }

    protected:
        WidgetTextObject* m_widgetTextObject;
        void mousePressEvent( QMouseEvent* event ) {
            if(!sendMouseEvent(event)) {
                QTextBrowser::mousePressEvent( event );
            }

        }

        void mouseReleaseEvent( QMouseEvent* event ) {
            if(!sendMouseEvent(event)) {
                QTextBrowser::mouseReleaseEvent( event );
            }
        }

        void mouseMoveEvent( QMouseEvent* event ) {
            if(!sendMouseEvent(event)) {
                QTextBrowser::mouseMoveEvent( event );
            }
        }

        bool sendMouseEvent( QMouseEvent* event ) {
            QWidget* w = m_widgetTextObject->widgetAtPoint( event->pos());
            if( !w ) return false;

            QRectF wRect = m_widgetTextObject->widgetRect( w );
            QPoint pos = event->pos() - wRect.toRect().topLeft();

            QWidget* childWidget = w->childAt( event->pos());
            if( !childWidget ) {
                childWidget = w;
            } else {
                pos = childWidget->mapTo( w, pos );
            }

            QMouseEvent* widgetMouseEvent = new QMouseEvent( event->type(), pos, event->button(), event->buttons(), event->modifiers());

            QCoreApplication::postEvent( childWidget, widgetMouseEvent );
            event->accept();
            return true;
        }
        enum WidgetProperties { WidgetData = 1 };
        enum { WidgetImageFormat = QTextFormat::UserObject + 1 };
        QWidget* currentHoverWidget;
};

class MetadataWidget : public StylableWidget
{
    Q_OBJECT
public:
    MetadataWidget( const Track& track, QWidget* p = 0 );
    ~MetadataWidget() { delete ui.artist.image; }

    class ScrobbleControls* scrobbleControls() const { return ui.track.scrobbleControls; }

    QWidget* basicInfoWidget();

private slots:

    void onTrackGotInfo(const QByteArray& data);
    void onAlbumGotInfo();
    void onArtistGotInfo();
    void onArtistGotEvents();
    void onTrackGotTopFans();
    
    void onTrackGotYourTags();
    void onTrackGotPopTags();

    void onFinished();

    void onTrackCorrected( QString correction );

    void onAnchorClicked( const QUrl& link );
    void onBioChanged( const QSizeF& );

    void listItemClicked( const QModelIndex& );

    void onScrobblesCached( const QList<lastfm::Track>& tracks );
    void onScrobbleStatusChanged();

signals:
    void lovedStateChanged(bool loved);
    void backClicked();

private:
    void setTrackDetails( const Track& track );

protected:
    void setupTrackStats( QWidget* w );
    void setupTrackDetails( QWidget* w );
    void setupTrackTags( QWidget* w );
    void setupUi();
    void fetchTrackInfo();

    struct {
         class QScrollArea* scrollArea;
         class QPushButton* backButton;

         struct {
             class QLabel* title;
             class QLabel* album;
             class QLabel* artist;
             
             class ScrobbleControls* scrobbleControls;
             class QLabel* yourScrobbles;
             class QLabel* totalScrobbles;
             class QLabel* listeners;

             class HttpImageWidget* albumImage;
             class DataListWidget* yourTags;
             class DataListWidget* popTags;
         } track;

         struct {
            class QLabel* artist;
            class TB* bio;
            class BannerWidget* banner;
            class HttpImageWidget* image;
         } artist;
    } ui;

    Track m_track;
    int m_scrobbles;
    int m_userListens;

    struct {
        class LfmListModel* similarArtists;
        class LfmListModel* listeningNow;
    } model;

    QNetworkReply* m_trackInfoReply;

    QNetworkReply* m_albumInfoReply;
    QNetworkReply* m_artistInfoReply;
    QNetworkReply* m_artistEventsReply;
    QNetworkReply* m_trackTopFansReply;
    QNetworkReply* m_trackTagsReply;
};

#endif //METADATA_WIDGET_H_