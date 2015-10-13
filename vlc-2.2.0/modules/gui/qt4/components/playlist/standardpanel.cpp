/*****************************************************************************
 * standardpanel.cpp : The "standard" playlist panel : just a treeview
 ****************************************************************************
 * Copyright © 2000-2010 VideoLAN
 * $Id: 19c67dc87931324ce62719c9386df5c35fd027f3 $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "components/playlist/standardpanel.hpp"

#include "components/playlist/vlc_model.hpp"      /* VLCModel */
#include "components/playlist/playlist_model.hpp" /* PLModel */
#include "components/playlist/views.hpp"          /* 3 views */
#include "components/playlist/selector.hpp"       /* PLSelector */
#include "util/animators.hpp"                     /* PixmapAnimator */
#include "menus.hpp"                              /* Popup */
#include "input_manager.hpp"                      /* THEMIM */
#include "dialogs_provider.hpp"                   /* THEDP */
#include "recents.hpp"                            /* RecentMRL */
#include "dialogs/playlist.hpp"                   /* Playlist Dialog */
#include "dialogs/mediainfo.hpp"                  /* MediaInfoDialog */
#include "util/qt_dirs.hpp"

#include <vlc_services_discovery.h>               /* SD_CMD_SEARCH */
#include <vlc_intf_strings.h>                     /* POP_ */

#include "dialogs/user/useroption.hpp"                   /* add by lili */
#include "../../../../services_discovery/user.hpp"

#define I_NEW_DIR \
    I_DIR_OR_FOLDER( N_("Create Directory"), N_( "Create Folder" ) )
#define I_NEW_DIR_NAME \
    I_DIR_OR_FOLDER( N_( "Enter name for new directory:" ), \
                     N_( "Enter name for new folder:" ) )

#define I_RENAME_DIR \
    I_DIR_OR_FOLDER( N_("Rename Directory"), N_( "Rename Folder" ) )
#define I_RENAME_DIR_NAME \
    I_DIR_OR_FOLDER( N_( "Enter a new name for the directory:" ), \
                     N_( "Enter a new name for the folder:" ) )

#include <QHeaderView>
#include <QMenu>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QStackedLayout>
#include <QSignalMapper>
#include <QSettings>
#include <QStylePainter>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QMessageBox>

#include <assert.h>

#include <vlc_modules.h>

#include <python2.7/Python.h>

typedef list<string> LISTSTRING;
/* local helper */
inline QModelIndex popupIndex( QAbstractItemView *view );

StandardPLPanel::StandardPLPanel( PlaylistWidget *_parent,
                                  intf_thread_t *_p_intf,
                                  playlist_item_t *p_root,
                                  PLSelector *_p_selector,
                                  VLCModel *_p_model )
                : QWidget( _parent ),
                  model( _p_model ),
                  p_intf( _p_intf ),
                  p_selector( _p_selector )
{
    viewStack = new QStackedLayout( this );
    viewStack->setSpacing( 0 ); viewStack->setMargin( 0 );
    setMinimumWidth( 300 );

    iconView    = NULL;
    treeView    = NULL;
    listView    = NULL;
    picFlowView = NULL;

    currentRootIndexPLId  = -1;
    lastActivatedPLItemId     = -1;

    QList<QString> frames;
    frames << ":/util/wait1";
    frames << ":/util/wait2";
    frames << ":/util/wait3";
    frames << ":/util/wait4";
    spinnerAnimation = new PixmapAnimator( this, frames );
    CONNECT( spinnerAnimation, pixmapReady( const QPixmap & ), this, updateViewport() );

    /* Saved Settings */
    int i_savedViewMode = getSettings()->value( "Playlist/view-mode", TREE_VIEW ).toInt();
    i_zoom = getSettings()->value( "Playlist/zoom", 0 ).toInt();

    showView( i_savedViewMode );

    DCONNECT( THEMIM, leafBecameParent( int ),
              this, browseInto( int ) );

    CONNECT( model, currentIndexChanged( const QModelIndex& ),
             this, handleExpansion( const QModelIndex& ) );
    CONNECT( model, rootIndexChanged(), this, browseInto() );

    setRootItem( p_root, false );
}

StandardPLPanel::~StandardPLPanel()
{
    getSettings()->beginGroup("Playlist");
    if( treeView )
        getSettings()->setValue( "headerStateV2", treeView->header()->saveState() );
    getSettings()->setValue( "view-mode", currentViewIndex() );
    getSettings()->setValue( "zoom", i_zoom );
    getSettings()->endGroup();
}

/* Unused anymore, but might be useful, like in right-click menu */
void StandardPLPanel::gotoPlayingItem()
{
    currentView->scrollTo( model->currentIndex() );
}

void StandardPLPanel::handleExpansion( const QModelIndex& index )
{
	printf( "%s\n", __func__ );
    assert( currentView );
    if( currentRootIndexPLId != -1 && currentRootIndexPLId != model->itemId( index.parent(), PLAYLIST_ID ) )
	{
		printf( "-----------------line:%d---------------------\n", __LINE__ );
        browseInto( index.parent() );
	}
	printf( "-----------------line:%d---------------------\n", __LINE__ );
    currentView->scrollTo( index );
}

void StandardPLPanel::popupPlView( const QPoint &point )
{
    QPoint globalPoint = currentView->viewport()->mapToGlobal( point );
    QModelIndex index = currentView->indexAt( point );
    if ( !index.isValid() )
    {
        currentView->clearSelection();
    }
    else if ( ! currentView->selectionModel()->selectedIndexes().contains( index ) )
    {
        currentView->selectionModel()->select( index, QItemSelectionModel::Select );
    }

    if( !popup( globalPoint ) ) VLCMenuBar::PopupMenu( p_intf, true );
}

/*********** Popup *********/
bool StandardPLPanel::popup( const QPoint &point )
{
    QModelIndex index = popupIndex( currentView ); /* index for menu logic only. Do not store.*/
    VLCModel *model = qobject_cast<VLCModel *>(currentView->model());

#define ADD_MENU_ENTRY( icon, title, act ) \
    if ( model->isSupportedAction( act, index ) )\
    {\
    action = menu.addAction( icon, title ); \
    container.action = act; \
    action->setData( QVariant::fromValue( container ) );\
    }

    /* */
    QMenu menu;
    QAction *action;
    VLCModelSubInterface::actionsContainerType container;

    /* Play/Stream/Info static actions */

    ADD_MENU_ENTRY( QIcon( ":/menu/play" ), qtr(I_POP_PLAY),
                    VLCModelSubInterface::ACTION_PLAY )

    ADD_MENU_ENTRY( QIcon( ":/menu/stream" ), qtr(I_POP_STREAM),
                    VLCModelSubInterface::ACTION_STREAM )

    ADD_MENU_ENTRY( QIcon(), qtr(I_POP_SAVE),
                    VLCModelSubInterface::ACTION_SAVE );

    ADD_MENU_ENTRY( QIcon( ":/menu/info" ), qtr(I_POP_INFO),
                    VLCModelSubInterface::ACTION_INFO );

    menu.addSeparator();

    ADD_MENU_ENTRY( QIcon( ":/type/folder-grey" ), qtr(I_POP_EXPLORE),
                    VLCModelSubInterface::ACTION_EXPLORE );

    QIcon addIcon( ":/buttons/playlist/playlist_add" );

    ADD_MENU_ENTRY( addIcon, qtr(I_POP_NEWFOLDER),
                    VLCModelSubInterface::ACTION_CREATENODE )

    ADD_MENU_ENTRY( QIcon(), qtr(I_POP_RENAMEFOLDER),
                    VLCModelSubInterface::ACTION_RENAMENODE )

    menu.addSeparator();
    /* In PL or ML, allow to add a file/folder */
    ADD_MENU_ENTRY( addIcon, qtr(I_PL_ADDF),
                    VLCModelSubInterface::ACTION_ENQUEUEFILE )

    ADD_MENU_ENTRY( addIcon, qtr(I_PL_ADDDIR),
                    VLCModelSubInterface::ACTION_ENQUEUEDIR )

    ADD_MENU_ENTRY( addIcon, qtr(I_OP_ADVOP),
                    VLCModelSubInterface::ACTION_ENQUEUEGENERIC )

    ADD_MENU_ENTRY( QIcon(), qtr(I_PL_ADDPL),
                    VLCModelSubInterface::ACTION_ADDTOPLAYLIST );

    menu.addSeparator();
    ADD_MENU_ENTRY( QIcon(), qtr( I_PL_SAVE ),
                    VLCModelSubInterface::ACTION_SAVETOPLAYLIST );

    menu.addSeparator();

    /* Item removal */

    ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr(I_POP_DEL),
                    VLCModelSubInterface::ACTION_REMOVE );

    ADD_MENU_ENTRY( QIcon( ":/toolbar/clear" ), qtr("Clear the playlist"),
                    VLCModelSubInterface::ACTION_CLEAR );

    menu.addSeparator();

    /* Playlist sorting */
    /*wangpei*/
    // if ( model->isSupportedAction( VLCModelSubInterface::ACTION_SORT, index ) )
    // {
    //     QMenu *sortingMenu = new QMenu( qtr( "Sort by" ) );
    //     /* Choose what columns to show in sorting menu, not sure if this should be configurable*/
    //     QList<int> sortingColumns;
    //     sortingColumns << COLUMN_TITLE << COLUMN_ARTIST << COLUMN_ALBUM << COLUMN_TRACK_NUMBER << COLUMN_URI;
    //     container.action = VLCModelSubInterface::ACTION_SORT;
    //     foreach( int Column, sortingColumns )
    //     {
    //         action = sortingMenu->addAction( qfu( psz_column_title( Column ) ) + " " + qtr("Ascending") );
    //         container.column = model->columnFromMeta(Column) + 1;
    //         action->setData( QVariant::fromValue( container ) );

    //         action = sortingMenu->addAction( qfu( psz_column_title( Column ) ) + " " + qtr("Descending") );
    //         container.column = -1 * (model->columnFromMeta(Column)+1);
    //         action->setData( QVariant::fromValue( container ) );
    //     }
    //     menu.addMenu( sortingMenu );
    // }

    /* Zoom */
    /*wangpei*/
    // QMenu *zoomMenu = new QMenu( qtr( "Display size" ) );
    // zoomMenu->addAction( qtr( "Increase" ), this, SLOT( increaseZoom() ) );
    // zoomMenu->addAction( qtr( "Decrease" ), this, SLOT( decreaseZoom() ) );
    // menu.addMenu( zoomMenu );

    /*wangpei*/
    if (p_selector->getCurrentItemCategory() == LOCALSHARE )
    {
        //menu.addAction( qtr( "ADD LOCAL SHARE" ), this, SLOT( increaseZoom() ) );
        //menu.addAction( qtr( "增加本地共享文件" ), this, SLOT( addLocalShareFile() ) );
        //menu.addAction( qtr( "DELETE LOCAL SHARE" ), this, SLOT( increaseZoom() ) );
#if 1
		//ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr(I_POP_ADDLOCAL),
		ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr("增加本地共享文件"),
				VLCModelSubInterface::ACTION_ADDLOCAL );
		//ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr(I_POP_DELLOCAL),
		ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr("删除本地共享文件"),
				VLCModelSubInterface::ACTION_DELLOCAL );
#endif
    }

    if (p_selector->getCurrentItemCategory() == CLOUDSHARE )
    {
        //menu.addAction( qtr( "ADD CLOUD SHARE" ), this, SLOT( increaseZoom() ) );
        //menu.addAction( qtr( "DELETE ClOUD SHARE" ), this, SLOT( increaseZoom() ) );

		ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr("上传云共享文件"), VLCModelSubInterface::ACTION_ADDCLOUD );

		ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr("下载云共享文件"), VLCModelSubInterface::ACTION_DWNCLOUD );

		ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr("删除云共享文件"), VLCModelSubInterface::ACTION_DELCLOUD );
    }

    if (p_selector->getCurrentItemCategory() == LANSHARE )
    {
        menu.addAction( qtr( "ADD LAN SHARE" ), this, SLOT( increaseZoom() ) );
        menu.addAction( qtr( "DELETE LAN SHARE" ), this, SLOT( increaseZoom() ) );
    }
    if (p_selector->getCurrentItemCategory() == REMOTESHARE )
    {
        //menu.addAction( qtr( "ADD REMOTE SHARE" ), this, SLOT( increaseZoom() ) );
        //menu.addAction( qtr( "DELETE REMOTE SHARE" ), this, SLOT( increaseZoom() ) );
        ADD_MENU_ENTRY( QIcon( ":/buttons/playlist/playlist_remove" ), qtr("获取远端文件列表"),
                VLCModelSubInterface::ACTION_ADDSHARE );
    }

    CONNECT( &menu, triggered( QAction * ), this, popupAction( QAction * ) );

    // menu.addMenu( StandardPLPanel::viewSelectionMenu( this ) );      /*wangpei*/

    /* Display and forward the result */
    if( !menu.isEmpty() )
    {
        menu.exec( point ); return true;
    }
    else return false;

#undef ADD_MENU_ENTRY
}

void StandardPLPanel::popupAction( QAction *action )
{
	//printf( "---------------------%s-----------------------\n", __func__ );
    VLCModel *model = qobject_cast<VLCModel *>(currentView->model());
    VLCModelSubInterface::actionsContainerType a =
            action->data().value<VLCModelSubInterface::actionsContainerType>();
    QModelIndexList list = currentView->selectionModel()->selectedRows();
    QModelIndex index = popupIndex( currentView );
    char *path = NULL;
    OpenDialog *dialog;
    QString temp;
    QStringList uris;
    bool ok;
	/*add by lili*/
	char configPath[1024];
	memset( configPath, 0, sizeof(configPath));
	if( access("../sbin/minidlna.conf", F_OK ) >= 0 )
		sprintf( configPath, "../sbin/minidlna.conf" );
	else if( access("./minidlna-1.1.4/minidlna.conf", F_OK ) >= 0 )
		sprintf( configPath, "./minidlna-1.1.4/minidlna.conf" );

	char cmdbuf[1024];
	memset( cmdbuf, 0, sizeof(configPath));

	sprintf( cmdbuf, "sed -n --silent '/^media_dir=/p' " );
	strcat( cmdbuf, configPath );
	strcat( cmdbuf, "  | awk -F, '{print $2}'" );
	FILE * pathFile;
	pathFile = popen( cmdbuf, "r" );
	if( !pathFile )
	{
		printf( "popen failed !\n" );
		return;
	}

	char sharePath[1024];
	memset( sharePath, 0, sizeof(sharePath) );
	size_t unit, buflen;
	unit = sizeof( char );
	buflen = sizeof( sharePath );
	fread( sharePath, unit, buflen, pathFile );
	pclose( pathFile );
	int len;
	len = strlen( sharePath );
	if( sharePath[ len-1 ] == '\r' || sharePath[ len-1 ] == '\n' )
		sharePath[ len-1 ] = '\0';
	/*add by lili end */

	/* first try to complete actions requiring missing parameters thru UI dialogs */
	switch( a.action )
	{
		case VLCModelSubInterface::ACTION_INFO:
			/* locally handled only */
			if( index.isValid() )
			{
				input_item_t* p_input = model->getInputItem( index );
				MediaInfoDialog *mid = new MediaInfoDialog( p_intf, p_input );
				mid->setParent( PlaylistDialog::getInstance( p_intf ),
						Qt::Dialog );
				mid->show();
			}
			break;

		case VLCModelSubInterface::ACTION_EXPLORE:
			/* locally handled only */
			temp = model->getURI( index );
			if( ! temp.isEmpty() ) path = make_path( temp.toLatin1().constData() );
			if( path == NULL ) return;
			QDesktopServices::openUrl(
					QUrl::fromLocalFile( QFileInfo( qfu( path ) ).absolutePath() ) );
			free( path );
			break;

		case VLCModelSubInterface::ACTION_STREAM:
			/* locally handled only */
			temp = model->getURI( index );
			if ( ! temp.isEmpty() )
				THEDP->streamingDialog( NULL, temp, false );
			break;

		case VLCModelSubInterface::ACTION_SAVE:
			/* locally handled only */
			temp = model->getURI( index );
			if ( ! temp.isEmpty() )
				THEDP->streamingDialog( NULL, temp );
			break;

		case VLCModelSubInterface::ACTION_CREATENODE:
			temp = QInputDialog::getText( PlaylistDialog::getInstance( p_intf ),
					qtr( I_NEW_DIR ), qtr( I_NEW_DIR_NAME ),
					QLineEdit::Normal, QString(), &ok);
			if ( !ok ) return;
			model->createNode( index, temp );
			break;

		case VLCModelSubInterface::ACTION_RENAMENODE:
			temp = QInputDialog::getText( PlaylistDialog::getInstance( p_intf ),
					qtr( I_RENAME_DIR ), qtr( I_RENAME_DIR_NAME ),
					QLineEdit::Normal, model->getTitle( index ), &ok);
			if ( !ok ) return;
			model->renameNode( index, temp );
			break;

		case VLCModelSubInterface::ACTION_ENQUEUEFILE:
			uris = THEDP->showSimpleOpen();
			if ( uris.isEmpty() ) return;
			uris.sort();
			foreach( const QString &file, uris )
				a.uris << qtu( toURI( toNativeSeparators( file ) ) );
			action->setData( QVariant::fromValue( a ) );
			if ( model->action( action, list ) )
				foreach( const QString &file, a.uris )
					RecentsMRL::getInstance( p_intf )->addRecent( file );
			break;

		case VLCModelSubInterface::ACTION_ENQUEUEDIR:
			temp = DialogsProvider::getDirectoryDialog( p_intf );
			if ( temp.isEmpty() ) return;
			a.uris << temp;
			action->setData( QVariant::fromValue( a ) );
			model->action( action, list );
			break;

		case VLCModelSubInterface::ACTION_ENQUEUEGENERIC:
			dialog = OpenDialog::getInstance( this, p_intf, false, SELECT, true, true );
			dialog->showTab( OPEN_FILE_TAB );
			dialog->exec(); /* make it modal */
			a.uris = dialog->getMRLs( false );
			a.options = dialog->getOptions();
			if ( a.uris.isEmpty() ) return;
			action->setData( QVariant::fromValue( a ) );
			if ( model->action( action, list ) )
				foreach( const QString &file, a.uris )
					RecentsMRL::getInstance( p_intf )->addRecent( file );
			break;

		case VLCModelSubInterface::ACTION_SAVETOPLAYLIST:
			THEDP->savePlayingToPlaylist();
			break;

			/*add by lili*/
		case VLCModelSubInterface::ACTION_DELLOCAL:
			{
			QString file = " rm ";
			file.append(qtu(QString(sharePath)));
			file.append("/");
			file.append( qtu(index.data().toString()) );
			printf("qtu index.data:%s\n", qtu(index.data().toString()));
			printf( "del file:%s\n", qtu(file) );
			printf("file no qtu:%s\n", file.toStdString().c_str() );
			system( file.toStdString().c_str() );

			
#if 0		
			char file[1024] ;
			memset( file, 0, sizeof(file));
			sprintf( file, "%s/", qtu(QString(sharePath)) );
			//strcat( file, index.data().toString().toStdString().c_str() );
			strcat( file, qtu(index.data().toString()) );
			printf("del file %s\n", qtu(QString(file)));
#endif
			//QFile::remove( file.toStdString().c_str()  );
			
			model->action( action, list );
			}
			break;

		case VLCModelSubInterface::ACTION_ADDLOCAL:
        //pl_item = playlist_ChildSearchName( THEPL->p_root, qtu( item->data(0, LONGNAME_ROLE ).toString() ) );

			uris = THEDP->showSimpleOpen();
			if ( uris.isEmpty() ) return;
			uris.sort();
			foreach( const QString &file, uris )
			{
				a.uris << qtu( toURI( toNativeSeparators( file ) ) );
				action->setData( QVariant::fromValue( a ) );

				/*copy selected file to share dir */
				//QString filename = file.right( file.count() - file.lastIndexOf("/") - 1 ).toStdString().c_str();
				QString filename = qtu(QString(file.right( file.count() - file.lastIndexOf("/") - 1 )));
				QString cmd = "link ";
				cmd.append(qtu(file));
				cmd.append( " " );
				cmd.append( qtu(QString(sharePath)) );
				cmd.append( "/");
				cmd.append( filename );
				QString dstfile = qtu(QString(sharePath));
				dstfile.append( "/");
				dstfile.append( filename );
                                printf("dstfile is:%s\n",dstfile.toStdString().c_str() );
                                if (access(dstfile.toStdString().c_str(),0) == 0){
				    QMessageBox msgBox( QMessageBox::Information,
							qtr( "增加本地共享文件" ),
							qtr( "目标文件已经存在！" ),
							QMessageBox::Ok,
							NULL );
				    msgBox.exec();
                                    return;
                                }
                                system( cmd.toStdString().c_str() );
				
				/*update Window items */
				QString url = "file://";
				url.append( qtu(QString(sharePath)) );
				url.append( "/" );
				url.append( filename );

				input_item_t *item = input_item_NewWithType ( url.toStdString().c_str(), filename.toStdString().c_str(), 0, NULL, 0, -1, ITEM_TYPE_CARD);

				//playlist_item_t *play_item = playlist_ChildSearchName( THEPL->p_root, "Local share" );
				playlist_item_t *play_item = playlist_ItemGetById( THEPL, model->itemId( index, PLAYLIST_ID ) );
				if( play_item == NULL )
					printf( "-------line:%d:play_item is NULL-------\n", __LINE__ );
				else
				{
					printf( "play_item id=[%d] i_children = %d addr=[%p] p_parent=[%p]  proot=[%p] \n",play_item->i_id, play_item->i_children,play_item,play_item->p_parent,THEPL->p_root);
					if (play_item->i_children >= 0)
						playlist_NodeAddInput( THEPL, item, play_item, PLAYLIST_APPEND, PLAYLIST_END, false);
					else
						playlist_NodeAddInput( THEPL, item, play_item->p_parent, PLAYLIST_APPEND, PLAYLIST_END, false);
				}
			}
			break;
		case VLCModelSubInterface::ACTION_ADDCLOUD:
			{
				UserOption *user = UserOption::getInstance( p_intf );
				int uid = user->getLUid();
				QString uidstr; uidstr.setNum( uid );

				uris = THEDP->showSimpleOpen();
				if ( uris.isEmpty() ) return;
				uris.sort();
				foreach( const QString &file, uris )
				{
					a.uris << qtu( toURI( toNativeSeparators( file ) ) );
					action->setData( QVariant::fromValue( a ) );

					/* Get local file information */
					QString filename = qtu(QString(file.right( file.count() - file.lastIndexOf("/") - 1 )));
					QString url = "http://192.168.7.97/download/";
					url.append( uidstr );
					url.append( "/" );
					url.append( filename );

					/*upload selected file to server */
					printf( "before upload: %s\n", file.toStdString().c_str() );
					printf( "filename:%s\n", filename.toStdString().c_str());
					QString upfile = filename;
					/*continue if upload failed*/
					printf( "upfile:%s\n", upfile.toStdString().c_str());
					//if( user->nfschina_upLoad( uid, upfile.toStdString().c_str(), file.toStdString().c_str() ) == 0 )
					 user->nfschina_upLoad( uid, upfile.toStdString().c_str(), file.toStdString().c_str() );

					 /*add the selected file to current window*/
					input_item_t *item = input_item_NewWithType ( url.toStdString().c_str(), filename.toStdString().c_str(), 0, NULL, 0, -1, ITEM_TYPE_FILE);
					playlist_item_t *play_item = playlist_ItemGetById( THEPL, model->itemId( index, PLAYLIST_ID ) );
					if( play_item == NULL )
						printf( "-------line:%d:play_item is NULL-------\n", __LINE__ );
					else
					{
						if (play_item->i_children >= 0)
							playlist_NodeAddInput( THEPL, item, play_item, PLAYLIST_APPEND, PLAYLIST_END, false);
						else
							playlist_NodeAddInput( THEPL, item, play_item->p_parent, PLAYLIST_APPEND, PLAYLIST_END, false);
					}
				}
			}
			break;
		case VLCModelSubInterface::ACTION_DELCLOUD:
			{
				/*tell server to delete the selected file*/
				QString file = index.data().toString();
				UserOption *user = UserOption::getInstance( p_intf );
				user->nfschina_delete( user->getLUid(), file );

				/*remove the selected item from current window*/
				playlist_item_t *play_item = playlist_ItemGetById( THEPL, model->itemId( index, PLAYLIST_ID ) );
				if( play_item == NULL )
					printf( "can't get root item for cloudshare module!\n" );
				else
					playlist_NodeDelete( THEPL, play_item, true , true );
			}
			break;

		case VLCModelSubInterface::ACTION_DWNCLOUD:
			{
				/* get the name of the selected file*/
				QString file = index.data().toString();
				printf( "down cloud file:%s\n", file.toStdString().c_str());

				/*get the URL of the selected file*/
				UserOption *user = UserOption::getInstance( p_intf );
				int uid = user->getLUid();
				QString url = user->nfschina_download( uid, file );
				printf( "download cloudfile url:%s\n", url.toStdString().c_str() );
				if( url == NULL )
				{
					printf( "get url for %s from server failed!\n", file.toStdString().c_str() );
					return;
				}

				/*download the selected file*/
				QString dest = qtu( QString(sharePath) );
				dest.append( "/" );
				dest.append( file );
				user->downloadCloudShareFile( url, file );
			}
			break;
        case VLCModelSubInterface::ACTION_ADDSHARE:
            {
                printf("%d\n", __LINE__);
                if(model->rootIndex() == index.parent())
                    printf("index is root index\n");
                else
                    return;

		  playlist_item_t *play_item = playlist_ItemGetById( THEPL, model->itemId( index, PLAYLIST_ID ) );
                if( play_item == NULL )
                    return;
	         if(play_item->i_children > 0)
		      return;
			 
                PyObject *pName1,*pModule1,*msg1,*pRetValue1,*pArgs1;
	  	  UserOption *user = NULL;

		  QString serverip = model->getURI( index);

		  int mid = 0;
		  char user_info[128] = {0};
		  char service_ip[128] = {0};
		  char cmid[128] = {0};
		  char sid[128] = {0};
		  char *p = NULL;
		  memcpy(user_info,  serverip.toStdString().c_str(), strlen(serverip.toStdString().c_str()));
                printf("get share user info = %s\n", user_info);

		  p = strrchr(user_info, 32);
		  memcpy(sid, p+1, strlen(p));
		  memcpy(service_ip, user_info, p-user_info);
		  p = strrchr(service_ip, 32);
                *p = ':';

                PyRun_SimpleString( "import sys" );
                //PyRun_SimpleString( "sys.path.append('./modules/services_discovery')" );
                PyRun_SimpleString( "sys.path.append('./share/python')" );
                pName1 = PyString_FromString("sharefile");
                pModule1 = PyImport_Import(pName1);

                msg1 = PyObject_GetAttrString(pModule1,"getfile");
                
                if(msg1 == NULL)
                {
                    printf("msg is NULL\n");
                    return;
                }

	         user = UserOption::getInstance( p_intf );
                if(user)
                {
                	mid = user->getLUid();
			sprintf(cmid, "%d", mid);
		  }
	         printf("service_ip = %s, cmid = %s, sid = %s\n", service_ip, cmid, sid);

                pArgs1 = PyTuple_New( 3 );
                PyTuple_SetItem( pArgs1, 0, Py_BuildValue( "s", service_ip) );
                PyTuple_SetItem( pArgs1, 1, Py_BuildValue( "s", cmid) );
                PyTuple_SetItem( pArgs1, 2, Py_BuildValue( "s", sid ) );

                pRetValue1 = PyObject_CallObject( msg1, pArgs1 );
                if(pRetValue1 == NULL)
                {
                   printf("%d:pRetValue1 is NULL\n",__LINE__); 
                   return;
                }
                int s = PyList_Size( pRetValue1 );
                printf("%d\n", __LINE__);
                // list<string> msgList;
                LISTSTRING msgList1;
                msgList1.clear();
                LISTSTRING::iterator ii1; 
                for( int i = 0; i < s; i++ )
                {
                    msgList1.push_back(PyString_AsString( PyList_GetItem( pRetValue1, i ) ) );
                }

                printf("msgList1.size = %ld\n", msgList1.size());
                for (ii1 = msgList1.begin(); ii1 != msgList1.end(); ++ii1)
                {
                    printf("%s\n", (*ii1).c_str());
                    QString shareurl = "http://192.168.7.88:8090/transfer/";
		      shareurl.append( cmid );
		      shareurl.append( "/" );
                    shareurl.append( sid);
		      shareurl.append( "/" );
                    shareurl.append( (*ii1).c_str() );
		      RecentsMRL::getInstance( p_intf )->addRecent( shareurl );
                    printf("sharefileurl:%s\n", shareurl.toStdString().c_str());
                    input_item_t *item = input_item_NewWithType ( shareurl.toStdString().c_str(), _((*ii1).c_str()), 0, NULL, 0, -1, ITEM_TYPE_FILE);
                    if ( item == NULL)
		                return;
                    printf("play_item->i_id = %d\n", play_item->i_id);
		      playlist_NodeAddInput( THEPL, item , play_item, PLAYLIST_APPEND, PLAYLIST_END, false );
                }
            }
            break;  
		default:
			model->action( action, list );
	}
}

QMenu* StandardPLPanel::viewSelectionMenu( StandardPLPanel *panel )
{
	QMenu *viewMenu = new QMenu( qtr( "Playlist View Mode" ) );
	QSignalMapper *viewSelectionMapper = new QSignalMapper( viewMenu );
	CONNECT( viewSelectionMapper, mapped( int ), panel, showView( int ) );

	QActionGroup *viewGroup = new QActionGroup( viewMenu );
# define MAX_VIEW StandardPLPanel::VIEW_COUNT
	for( int i = 0; i < MAX_VIEW; i++ )
	{
		QAction *action = viewMenu->addAction( viewNames[i] );
		action->setCheckable( true );
		viewGroup->addAction( action );
		viewSelectionMapper->setMapping( action, i );
		CONNECT( action, triggered(), viewSelectionMapper, map() );
		if( panel->currentViewIndex() == i )
			action->setChecked( true );
	}
	return viewMenu;
}

inline QModelIndex popupIndex( QAbstractItemView *view )
{
	QModelIndexList list = view->selectionModel()->selectedIndexes();
	if ( list.isEmpty() )
		return QModelIndex();
	else
		return list.first();
}

void StandardPLPanel::popupSelectColumn( QPoint )
{
	QMenu menu;
	assert( treeView );

	/* We do not offer the option to hide index 0 column, or
	 * QTreeView will behave weird */
	for( int i = 1 << 1, j = 1; i < COLUMN_END; i <<= 1, j++ )
	{
		QAction* option = menu.addAction( qfu( psz_column_title( i ) ) );
		option->setCheckable( true );
		option->setChecked( !treeView->isColumnHidden( j ) );
		selectColumnsSigMapper->setMapping( option, j );
		CONNECT( option, triggered(), selectColumnsSigMapper, map() );
	}
	menu.exec( QCursor::pos() );
}

void StandardPLPanel::toggleColumnShown( int i )
{
	treeView->setColumnHidden( i, !treeView->isColumnHidden( i ) );
}

/* Search in the playlist */
void StandardPLPanel::search( const QString& searchText )
{
	int type;
	QString name;
	bool can_search;
	p_selector->getCurrentItemInfos( &type, &can_search, &name );

	if( type != SD_TYPE || !can_search )
	{
		bool flat = ( currentView == iconView ||
				currentView == listView ||
				currentView == picFlowView );
		model->filter( searchText,
				flat ? currentView->rootIndex() : QModelIndex(),
				!flat );
	}
}

void StandardPLPanel::searchDelayed( const QString& searchText )
{
	int type;
	QString name;
	bool can_search;
	p_selector->getCurrentItemInfos( &type, &can_search, &name );

	if( type == SD_TYPE && can_search )
	{
		if( !name.isEmpty() && !searchText.isEmpty() )
			playlist_ServicesDiscoveryControl( THEPL, qtu( name ), SD_CMD_SEARCH,
					qtu( searchText ) );
	}
}

/* Set the root of the new Playlist */
/* This activated by the selector selection */
void StandardPLPanel::setRootItem( playlist_item_t *p_item, bool b )
{
	printf( "----------%s:%s:%d--------------\n", __FILE__, __func__, __LINE__ );
	Q_UNUSED( b );
	model->rebuild( p_item );
}

void StandardPLPanel::browseInto( const QModelIndex &index )
{
	printf( "%s\n", __func__ );
	if( currentView == iconView || currentView == listView || currentView == picFlowView )
	{

		currentView->setRootIndex( index );

		/* When going toward root in LocationBar, scroll to the item
		   that was previously as root */
		QModelIndex newIndex = model->indexByPLID(currentRootIndexPLId,0);
		while( newIndex.isValid() && (newIndex.parent() != index) )
			newIndex = newIndex.parent();
		if( newIndex.isValid() )
			currentView->scrollTo( newIndex );

		/* Store new rootindexid*/
		currentRootIndexPLId = model->itemId( index, PLAYLIST_ID );

		model->ensureArtRequested( index );
	}

	emit viewChanged( index );

	/* add by lili */
       if (p_selector->getCurrentItemCategory() == CLOUDSHARE )
	{
		printf( "--------%s:%d------------\n", __func__, __LINE__ );
		createCloudItems( index );
	}

	if (p_selector->getCurrentItemCategory() == REMOTESHARE )
	{
		createRemoteShareItems( index );
	}

	if (p_selector->getCurrentItemCategory() == LOCALSHARE )
	{
		createLocalShareItems( index );
	}

}

/* add by lili */
void StandardPLPanel::createCloudItems( const QModelIndex &index )
{
	printf( "--------%s:%d------------\n", __func__, __LINE__ );
	UserOption *user = UserOption::getInstance( p_intf );
	if( user == NULL )
	{
		printf( "Failed to UerOption::getInstance\n" );
		return;
	}

	if( !user->isLogin() )
	{
		printf( "Please login first!\n" );
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "云端共享提示" ),
				qtr( "尚未登陆,无法获取文件列表!" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();
		return ;
	}

	playlist_item_t *play_item = playlist_ItemGetById( THEPL, model->itemId( index, PLAYLIST_ID ) );
	if( play_item == NULL || play_item->i_children > 0 )
		return;

	int uid = user->getLUid();
	printf("-----%s:uid=%d-----------\n", __func__, uid );
	QList<QString> files = user->nfschina_GetFileList( uid );
	printf( "---------------%s:%d-----------------\n", __func__, __LINE__ );
	foreach( const QString& file, files )
	{
		printf( "file:%s\n", file.toStdString().c_str() );
		QString url = "http://192.168.7.97/download/";
		QString uidstr; uidstr.setNum( uid );
		url.append( uidstr );
		url.append( "/" );
		url.append( qtu(file) );
		printf( "url:%s\n", url.toStdString().c_str() );
		RecentsMRL::getInstance( p_intf )->addRecent( url );
		input_item_t *item = input_item_NewWithType ( qtu(url), qtu(file), 0, NULL, 0, -1, ITEM_TYPE_FILE);
		if ( item == NULL )
			return;
		playlist_NodeAddInput( THEPL, item, play_item, PLAYLIST_APPEND, PLAYLIST_END, false);
	}
}

void StandardPLPanel::createRemoteShareItems( const QModelIndex &index )
{
	PyObject *pName,*pModule,*msg,*pRetValue,*pArgs;
       UserOption *user = NULL;
	int mid = 0;

       printf("createRemoteShareItems\n");
	user = UserOption::getInstance( p_intf );
	if( user == NULL )
	{
		printf( "Failed to UerOption::getInstance\n" );
		return;
	}

	if( !user->isLogin() )
	{
		printf( "Please login first!\n" );
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "远端共享提示" ),
				qtr( "尚未登陆,无法获取在线设备列表!" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();
		return ;
	}

	mid = user->getLUid();

	Py_Initialize();

	if( !Py_IsInitialized() )
	{
	      printf( "Python initialize failed! \n" );
	      return ;
	}
       PyRun_SimpleString( "import sys" );
       //PyRun_SimpleString( "sys.path.append('./modules/services_discovery')" );
       PyRun_SimpleString( "sys.path.append('./share/python')" );
       pName = PyString_FromString("remotemsg");
       pModule = PyImport_Import(pName);

       msg = PyObject_GetAttrString(pModule,"nfschina_msg");
       if(msg == NULL)
       {
            printf("msg is NULL\n");
            return;
       }

       pArgs = PyTuple_New( 2 );
       PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", mid) );
       PyTuple_SetItem( pArgs, 1, Py_BuildValue( "i", 100 ) );
       pRetValue = PyObject_CallObject( msg, pArgs );
       int s = PyList_Size( pRetValue );

	LISTSTRING msgList;
	msgList.clear();
	LISTSTRING::iterator ii; 
	for( int i = 0; i < s; i++ )
	{
	     msgList.push_back(PyString_AsString( PyList_GetItem( pRetValue, i ) ) );
	}

       printf("msgList.size = %d\n", msgList.size());
       playlist_item_t *play_item = playlist_ItemGetById( THEPL, model->itemId( index, PLAYLIST_ID ) );
       if( play_item == NULL ) {
    	    printf( "-------line:%d:play_item is NULL-------\n", __LINE__ );
	    return;
       }
       if( play_item->i_children > 0 )
    	    return;

       for (ii = msgList.begin(); ii != msgList.end(); ++ii)
       {
            printf("msgList = %s\n", (*ii).c_str());
	     
     	     char *p = NULL;
     	     char *p1 = NULL;
     	     char buf[128] = {0};
     	     char serviceip[128] = {0};
     	     char userip[128] = {0};
     	     strcpy(buf, (*ii).c_str());
     	     p = strchr(buf, 32);
     	     p = strchr(p+1, 32);
     	     p = strchr(p+1, 32);
     	     memcpy(serviceip, buf, p-buf);
	     strcpy(userip, p+1);

     	     printf("serviceip = %s\n", serviceip);
	     printf("userip = %s\n", userip);

            input_item_t *item;
            item = input_item_NewWithType (serviceip, userip,
                                  0, NULL, 0, -1, ITEM_TYPE_DIRECTORY);
            if (item == NULL)
        	    return;

	     playlist_NodeAddInput( THEPL, item, play_item, PLAYLIST_APPEND, PLAYLIST_END, false);
        }
	 
}

void StandardPLPanel::createLocalShareItems( const QModelIndex &index )
{	
	UserOption *user = UserOption::getInstance( p_intf );
	printf( "local sharepath == [%s]\n", user->getSharePath().toStdString().c_str() );

	playlist_item_t *play_item = playlist_ItemGetById( THEPL, model->itemId( index, PLAYLIST_ID ) );
       if( play_item == NULL ) {
    	    printf( "-------line:%d:play_item is NULL-------\n", __LINE__ );
	    return;
       }
       if( play_item->i_children > 0 ) {
		    printf( "play_item->i_children >= 0\n" );
    	    return;
	   }
	QString path = user->getSharePath();
	QDir *dir = new QDir( path );

	QFileInfoList entries = dir->entryInfoList();
	foreach(const QFileInfo &file, entries )
	{
		if( file.isDir() )
		{
			printf( "local dir:%s\n",  file.fileName().toStdString().c_str() );
		}
		else
		{
			input_item_t *item;
			QString url = "file://";
			url.append(file.absoluteFilePath());
			RecentsMRL::getInstance( p_intf )->addRecent( url );
			printf( "localshare url:%s\n", url.toStdString().c_str() );
			item = input_item_NewWithType ( url.toUtf8().constData(), (file.fileName()).toUtf8().constData() ,
					0, NULL, 0, 0, ITEM_TYPE_FILE);
			if (item == NULL)
				continue;
			playlist_NodeAddInput( THEPL, item, play_item, PLAYLIST_APPEND, PLAYLIST_END, false);
		}
	}
}


void StandardPLPanel::browseInto()
{
	printf( "-------------%s-------------\n", __func__ );
	browseInto( (currentRootIndexPLId != -1 && currentView != treeView) ?
			model->indexByPLID( currentRootIndexPLId, 0 ) :
			QModelIndex() );
}

void StandardPLPanel::wheelEvent( QWheelEvent *e )
{
	if( e->modifiers() & Qt::ControlModifier ) {
		int numSteps = e->delta() / 8 / 15;
		if( numSteps > 0)
			increaseZoom();
		else if( numSteps < 0)
			decreaseZoom();
	}
	// Accept this event in order to prevent unwanted volume up/down changes
	e->accept();
}

bool StandardPLPanel::eventFilter ( QObject *obj, QEvent * event )
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
		if( keyEvent->key() == Qt::Key_Delete ||
				keyEvent->key() == Qt::Key_Backspace )
		{
			deleteSelection();
			return true;
		}
	}
	else if ( event->type() == QEvent::Paint )
	{/* Warn! Don't filter events from anything else than views ! */
		if ( model->rowCount() == 0 && p_selector->getCurrentItemCategory() == PL_ITEM_TYPE )
		{
			QWidget *viewport = qobject_cast<QWidget *>( obj );
			QStylePainter painter( viewport );
			QPixmap dropzone(":/dropzone");
			QRect rect = viewport->geometry();
			QSize size = rect.size() / 2 - dropzone.size() / 2;
			rect.adjust( 0, size.height(), 0 , 0 );
			painter.drawItemPixmap( rect, Qt::AlignHCenter, dropzone );
			/* now select the zone just below the drop zone and let Qt center
			   the text by itself */
			rect.adjust( 0, dropzone.size().height() + 10, 0, 0 );
			rect.setRight( viewport->geometry().width() );
			rect.setLeft( 0 );
			painter.drawItemText( rect,
					Qt::AlignHCenter,
					palette(),
					true,
					qtr("Playlist is currently empty.\n"
						"Drop a file here or select a "
						"media source from the left."),
					QPalette::Text );
		}
		else if ( spinnerAnimation->state() == PixmapAnimator::Running )
		{
			if ( currentView->model()->rowCount() )
				spinnerAnimation->stop(); /* Trick until SD emits events */
			else
			{
				QWidget *viewport = qobject_cast<QWidget *>( obj );
				QStylePainter painter( viewport );
				QPixmap *spinner = spinnerAnimation->getPixmap();
				QPoint point = viewport->geometry().center();
				point -= QPoint( spinner->size().width() / 2, spinner->size().height() / 2 );
				painter.drawPixmap( point, *spinner );
			}
		}
	}
	return false;
}

void StandardPLPanel::deleteSelection()
{
	QModelIndexList list = currentView->selectionModel()->selectedIndexes();
	model->doDelete( list );
}

void StandardPLPanel::createIconView()
{
	iconView = new PlIconView( model, this );
	iconView->setContextMenuPolicy( Qt::CustomContextMenu );
	CONNECT( iconView, customContextMenuRequested( const QPoint & ),
			this, popupPlView( const QPoint & ) );
	CONNECT( iconView, activated( const QModelIndex & ),
			this, activate( const QModelIndex & ) );
	iconView->installEventFilter( this );
	iconView->viewport()->installEventFilter( this );
	viewStack->addWidget( iconView );
}

void StandardPLPanel::createListView()
{
	listView = new PlListView( model, this );
	listView->setContextMenuPolicy( Qt::CustomContextMenu );
	CONNECT( listView, customContextMenuRequested( const QPoint & ),
			this, popupPlView( const QPoint & ) );
	CONNECT( listView, activated( const QModelIndex & ),
			this, activate( const QModelIndex & ) );
	listView->installEventFilter( this );
	listView->viewport()->installEventFilter( this );
	viewStack->addWidget( listView );
}

void StandardPLPanel::createCoverView()
{
	picFlowView = new PicFlowView( model, this );
	picFlowView->setContextMenuPolicy( Qt::CustomContextMenu );
	CONNECT( picFlowView, customContextMenuRequested( const QPoint & ),
			this, popupPlView( const QPoint & ) );
	CONNECT( picFlowView, activated( const QModelIndex & ),
			this, activate( const QModelIndex & ) );
	viewStack->addWidget( picFlowView );
	picFlowView->installEventFilter( this );
}

void StandardPLPanel::createTreeView()
{
	/* Create and configure the QTreeView */
	treeView = new PlTreeView( model, this );

	/* setModel after setSortingEnabled(true), or the model will sort immediately! */

	/* Connections for the TreeView */
	CONNECT( treeView, activated( const QModelIndex& ),
			this, activate( const QModelIndex& ) );
	CONNECT( treeView->header(), customContextMenuRequested( const QPoint & ),
			this, popupSelectColumn( QPoint ) );
	CONNECT( treeView, customContextMenuRequested( const QPoint & ),
			this, popupPlView( const QPoint & ) );
	treeView->installEventFilter( this );
	treeView->viewport()->installEventFilter( this );

	/* SignalMapper for columns */
	selectColumnsSigMapper = new QSignalMapper( this );
	CONNECT( selectColumnsSigMapper, mapped( int ),
			this, toggleColumnShown( int ) );

	viewStack->addWidget( treeView );
}

void StandardPLPanel::updateZoom( int i )
{
	if ( i < 5 - QApplication::font().pointSize() ) return;
	if ( i > 3 + QApplication::font().pointSize() ) return;
	i_zoom = i;
#define A_ZOOM( view ) \
	if ( view ) \
	qobject_cast<AbstractPlViewItemDelegate*>( view->itemDelegate() )->setZoom( i_zoom )
	/* Can't iterate as picflow & tree aren't using custom delegate */
	A_ZOOM( iconView );
	A_ZOOM( listView );
#undef A_ZOOM
}

void StandardPLPanel::showView( int i_view )
{
	bool b_treeViewCreated = false;

	switch( i_view )
	{
		case ICON_VIEW:
			{
				if( iconView == NULL )
					createIconView();
				currentView = iconView;
				break;
			}
		case LIST_VIEW:
			{
				if( listView == NULL )
					createListView();
				currentView = listView;
				break;
			}
		case PICTUREFLOW_VIEW:
    {
        if( picFlowView == NULL )
            createCoverView();
        currentView = picFlowView;
        break;
    }
    default:
    case TREE_VIEW:
    {
        if( treeView == NULL )
        {
            createTreeView();
            b_treeViewCreated = true;
        }
        currentView = treeView;
        break;
    }
    }

    currentView->setModel( model );

    /* Restoring the header Columns must come after changeModel */
    if( b_treeViewCreated )
    {
        assert( treeView );
        if( getSettings()->contains( "Playlist/headerStateV2" ) )
        {
            treeView->header()->restoreState(getSettings()
                    ->value( "Playlist/headerStateV2" ).toByteArray() );
            /* if there is allready stuff in playlist, we don't sort it and we reset
               sorting */
            if( model->rowCount() )
            {
                treeView->header()->setSortIndicator( -1 , Qt::AscendingOrder );
            }
        }
        else
        {
            for( int m = 1, c = 0; m != COLUMN_END; m <<= 1, c++ )
            {
                treeView->setColumnHidden( c, !( m & COLUMN_DEFAULT ) );
                if( m == COLUMN_TITLE ) treeView->header()->resizeSection( c, 200 );
                else if( m == COLUMN_DURATION ) treeView->header()->resizeSection( c, 80 );
            }
        }
    }

    updateZoom( i_zoom );
    viewStack->setCurrentWidget( currentView );
    browseInto();
    gotoPlayingItem();
}

void StandardPLPanel::setWaiting( bool b )
{
    if ( b )
    {
        spinnerAnimation->setLoopCount( 20 ); /* Trick until SD emits an event */
        spinnerAnimation->start();
    }
    else
        spinnerAnimation->stop();
}

void StandardPLPanel::updateViewport()
{
    /* A single update on parent widget won't work */
    currentView->viewport()->repaint();
}

int StandardPLPanel::currentViewIndex() const
{
    if( currentView == treeView )
        return TREE_VIEW;
    else if( currentView == iconView )
        return ICON_VIEW;
    else if( currentView == listView )
        return LIST_VIEW;
    else
        return PICTUREFLOW_VIEW;
}

void StandardPLPanel::cycleViews()
{
    if( currentView == iconView )
        showView( TREE_VIEW );
    else if( currentView == treeView )
        showView( LIST_VIEW );
    else if( currentView == listView )
#ifndef NDEBUG
        showView( PICTUREFLOW_VIEW  );
    else if( currentView == picFlowView )
#endif
        showView( ICON_VIEW );
    else
        assert( 0 );
}

void StandardPLPanel::activate( const QModelIndex &index )
{
    if( currentView->model() == model )
    {
        /* If we are not a leaf node */
        if( !index.data( VLCModelSubInterface::IsLeafNodeRole ).toBool() )
        {
            if( currentView != treeView )
                browseInto( index );
        }
        else
        {
            playlist_Lock( THEPL );
            playlist_item_t *p_item = playlist_ItemGetById( THEPL, model->itemId( index, PLAYLIST_ID ) );
            if ( p_item )
            {
                p_item->i_flags |= PLAYLIST_SUBITEM_STOP_FLAG;
                lastActivatedPLItemId = p_item->i_id;
            }
            playlist_Unlock( THEPL );
            if ( p_item && index.isValid() )
                model->activateItem( index );
        }
    }
}

void StandardPLPanel::browseInto( int i_pl_item_id )
{
    if( i_pl_item_id != lastActivatedPLItemId ) return;

    QModelIndex index = model->indexByPLID( i_pl_item_id, 0 );

    if( currentView == treeView )
        treeView->setExpanded( index, true );
    else
        browseInto( index );

    lastActivatedPLItemId = -1;
}
