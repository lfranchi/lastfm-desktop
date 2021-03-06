

#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>

#include <lastfm/RadioStation.h>
#include <lastfm/XmlQuery.h>
#include <lastfm/Track.h>

#include "lib/unicorn/UnicornSettings.h"

#include "../Services/RadioService/RadioService.h"
#include "../Services/ScrobbleService/ScrobbleService.h"
#include "../Application.h"

#include "PlayableItemWidget.h"
#include "QuickStartWidget.h"

#include "RadioWidget.h"

#define MAX_RECENT_STATIONS 50

RadioWidget::RadioWidget(QWidget *parent)
    :QFrame( parent )
{
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );

    // need to know when we are playing the radio so we can switch between now playing and last playing
    connect( &RadioService::instance(), SIGNAL(tuningIn(RadioStation)), SLOT(onTuningIn(RadioStation) ) );
    connect( &RadioService::instance(), SIGNAL(stopped()), SLOT(onRadioStopped()));
    connect( &ScrobbleService::instance(), SIGNAL(trackStarted(Track,Track)), SLOT(onTrackStarted(Track,Track)) );

    connect( aApp, SIGNAL(sessionChanged(unicorn::Session*)), SLOT(onSessionChanged(unicorn::Session*) ) );
    connect( aApp, SIGNAL(gotUserInfo(lastfm::User)), SLOT(onGotUserInfo(lastfm::User)) );

    changeUser( aApp->currentSession()->userInfo().name() );
}


void
RadioWidget::onSessionChanged( unicorn::Session* session )
{
    changeUser( session->userInfo().name() );
}


void
RadioWidget::onGotUserInfo( const lastfm::User& userDetails )
{
    changeUser( userDetails.name() );
}


void
RadioWidget::changeUser( const QString& newUsername )
{
    if ( !newUsername.isEmpty() && ( m_currentUsername != newUsername ) )
    {
        m_currentUsername = newUsername;

        // remove any previous layout
        layout()->removeWidget( m_main );
        delete m_main;

        layout()->addWidget( m_main = new QWidget( this ) );
        QVBoxLayout* layout = new QVBoxLayout( m_main );
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        QuickStartWidget* quickStartWidget = new QuickStartWidget();
        layout->addWidget( quickStartWidget );

        {
            layout->addWidget( ui.nowPlayingFrame = new QFrame( this ) );
            QVBoxLayout* nowPlayingLayout = new QVBoxLayout( ui.nowPlayingFrame );
            nowPlayingLayout->setContentsMargins( 0, 0, 0, 0 );
            nowPlayingLayout->setSpacing( 0 );

            QFrame* splitter = new QFrame( this );
            nowPlayingLayout->addWidget( splitter );
            splitter->setObjectName( "splitter" );

            nowPlayingLayout->addWidget( ui.nowPlaying = new QLabel( tr("Last Station"), this ) );
            ui.nowPlaying->setObjectName( "title" );
            nowPlayingLayout->addWidget( ui.nowPlayingSection = new QFrame( this ) );
            ui.nowPlayingSection->setObjectName( "section" );
            QVBoxLayout* nowPlayingSectionLayout = new QVBoxLayout( ui.nowPlayingSection );
            nowPlayingSectionLayout->setContentsMargins( 0, 0, 0, 0 );
            nowPlayingSectionLayout->setSpacing( 0 );

            unicorn::UserSettings us( newUsername );
            QString stationUrl = us.value( "lastStationUrl", "" ).toString();
            QString stationTitle = us.value( "lastStationTitle", tr( "A Radio Station" ) ).toString();

            RadioStation lastStation( stationUrl );
            lastStation.setTitle( stationTitle );

            nowPlayingSectionLayout->addWidget( ui.lastStation = new PlayableItemWidget( lastStation, stationTitle ) );
            ui.lastStation->setObjectName( "station" );

            if ( stationUrl.isEmpty() )
                ui.nowPlayingFrame->hide();
        }

        {
            QFrame* splitter = new QFrame( this );
            layout->addWidget( splitter );
            splitter->setObjectName( "splitter" );

            QLabel* title = new QLabel( tr("Personal Stations"), this );
            layout->addWidget( title );
            title->setObjectName( "title" );
            layout->addWidget( ui.personal = new QFrame( this ) );
            ui.personal->setObjectName( "section" );
            QVBoxLayout* personalLayout = new QVBoxLayout( ui.personal );
            personalLayout->setContentsMargins( 0, 0, 0, 0 );
            personalLayout->setSpacing( 0 );
            personalLayout->addWidget( ui.library = new PlayableItemWidget( RadioStation::library( User( newUsername ) ), tr( "My Library Radio" ), tr( "Music you know and love" ) ) );
            ui.library->setObjectName( "library" );
            personalLayout->addWidget( ui.mix = new PlayableItemWidget( RadioStation::mix( User( newUsername ) ), tr( "My Mix Radio" ), tr( "Your library plus new music" ) ) );
            ui.mix->setObjectName( "mix" );
            personalLayout->addWidget( ui.rec = new PlayableItemWidget( RadioStation::recommendations( User( newUsername ) ), tr( "My Recommended Radio" ), tr( "New music from Last.fm" ) ) );
            ui.rec->setObjectName( "rec" );
        }

        {
            QFrame* splitter = new QFrame( this );
            layout->addWidget( splitter );
            splitter->setObjectName( "splitter" );

            QLabel* title = new QLabel( tr("Network Stations"), this ) ;
            layout->addWidget( title );
            title->setObjectName( "title" );
            layout->addWidget( ui.network = new QFrame( this ) );
            ui.network->setObjectName( "section" );
            QVBoxLayout* networkLayout = new QVBoxLayout( ui.network );
            networkLayout->setContentsMargins( 0, 0, 0, 0 );
            networkLayout->setSpacing( 0 );
            networkLayout->addWidget( ui.friends = new PlayableItemWidget( RadioStation::friends( User( newUsername ) ), tr( "My Friends' Radio" ), tr( "Music your friends like" ) ) );
            ui.friends->setObjectName( "friends" );
            networkLayout->addWidget( ui.neighbours = new PlayableItemWidget( RadioStation::neighbourhood( User( newUsername ) ), tr( "My Neighbourhood Radio" ), tr ( "Music from listeners like you" ) ) );
            ui.neighbours->setObjectName( "neighbours" );
        }

        {
            QFrame* splitter = new QFrame( this );
            layout->addWidget( splitter );
            splitter->setObjectName( "splitter" );

            QLabel* title = new QLabel( tr("Recent Stations"), this ) ;
            layout->addWidget( title );
            title->setObjectName( "title" );
            layout->addWidget( ui.recentStations = new QFrame( this ) );
            ui.recentStations->setObjectName( "section" );

            QVBoxLayout* layout = new QVBoxLayout( ui.recentStations );
            layout->setContentsMargins( 0, 0, 0, 0 );
            layout->setSpacing( 0 );
        }

        // fetch recent stations
        connect( User( newUsername ).getRecentStations( MAX_RECENT_STATIONS ), SIGNAL(finished()), SLOT(onGotRecentStations()));

        layout->addStretch( 1 );
    }
}

void
RadioWidget::onGotRecentStations()
{
    lastfm::XmlQuery lfm;

    if ( lfm.parse( qobject_cast<QNetworkReply*>(sender())->readAll() ) )
    {
        foreach ( const lastfm::XmlQuery& station, lfm["recentstations"].children("station") )
        {
            QString stationUrl = station["url"].text();

            if ( !stationUrl.startsWith( "lastfm://user/" + User().name() ) )
            {
                PlayableItemWidget* item = new PlayableItemWidget( RadioStation( stationUrl ), station["name"].text() );
                item->setObjectName( "station" );
                ui.recentStations->layout()->addWidget( item );
            }
        }
    }
    else
    {
        qDebug() << lfm.parseError().message() << lfm.parseError().enumValue();
    }
}

void
RadioWidget::onTuningIn( const RadioStation& station )
{
    // Save this as the last station played
    ui.nowPlaying->setText( tr( "Now Playing" ) );
    ui.lastStation->setStation( station.url(), station.title().isEmpty() ? tr( "A Radio Station" ) : station.title() );

    if ( !station.url().isEmpty() )
        ui.nowPlayingFrame->show();

    // insert at the front of the list

    if ( ui.recentStations && ui.recentStations->layout()
         && !station.url().isEmpty()
         && !station.url().startsWith( "lastfm://user/" + User().name() ) )
    {
        // if it exists already remove it
        for ( int i = 0 ; i < ui.recentStations->layout()->count() ; ++i )
        {
            if ( station.url() == qobject_cast<PlayableItemWidget*>(ui.recentStations->layout()->itemAt( i )->widget())->station().url() )
            {
                QLayoutItem* item = ui.recentStations->layout()->takeAt( i );
                item->widget()->deleteLater();
                delete item;
                break;
            }
        }

        // insert the new one at the beginning
        PlayableItemWidget* newItem = new PlayableItemWidget( station, station.title(), "", this );
        newItem->setObjectName( "station" );
        qobject_cast<QBoxLayout*>(ui.recentStations->layout())->insertWidget( 0, newItem );

        // limit the stations
        if ( ui.recentStations->layout()->count() > MAX_RECENT_STATIONS )
        {
            QLayoutItem* item = ui.recentStations->layout()->takeAt( ui.recentStations->layout()->count() - 1 );
            item->widget()->deleteLater();
            delete item;
        }
    }
}

void
RadioWidget::onRadioStopped()
{
    ui.nowPlaying->setText( tr( "Last Station" ) );
}

void
RadioWidget::onTrackStarted( const Track& track, const Track& /*oldTrack*/ )
{
    // if a track starts and it's not a radio track, we are no longer listening to the radio
    if ( track.source() == Track::LastFmRadio )
    {
        ui.nowPlaying->setText( tr( "Now Playing" ) );
    }
    else
    {
        ui.nowPlaying->setText( tr( "Last Station" ) );
    }
}
