/*****************************************************************************
 * usershare.cpp : User Share dialogs
 ****************************************************************************
 * Copyright (C) 2007 the VideoLAN team
 * $Id: 453753b28a4b397fb5afed3a3f626a243a4b381c $
 *
 * Authors: Jean-Baptiste Kempf <jb (at) videolan.org>
 *          Rémi Duraffort <ivoire (at) via.ecp.fr>
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

#include "qt4.hpp"
#include "dialogs/user/usershare.hpp"
#include "dialogs/user/login.hpp"
#include "dialogs/user/useroption.hpp"
#include "util/qt_dirs.hpp"

#include <vlc_about.h>
#include <vlc_intf_strings.h>

#ifdef UPDATE_CHECK
# include <vlc_update.h>
#endif

#include <QTextBrowser>
#include <QTabWidget>
#include <QLabel>
#include <QString>
#include <QDialogButtonBox>
#include <QEvent>
#include <QDate>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QTreeWidget>
#include <QFrame>
#include <QSplitter>
#include <QDirModel>
#include <QStandardItemModel>
#include <QListWidget>
#include <QStackedWidget>
#include <QMenu>

#include <QDebug>

#include <vlc_network.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>


#include <assert.h>

UploadArgs::UploadArgs( int _uid, QString& _file, QString& _filepath )
	: uid( _uid ), file( _file ) , filePath( _filepath )
{

}

DownloadArgs::DownloadArgs( QString& _url, QString& _file, int _block )
	: url( _url ), file( _file ), block( _block )
{

}

UserShareSelector::UserShareSelector( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	setContentsMargins( 0, 3, 0, 3 );
	
    setWindowTitle( qtr( "UserShareSelector" ) );
    setWindowRole( "vlc-usershareselector" );
    //setMinimumSize( 350, 300 );
	//setMaximumWidth( 120 );

	listWidget = new QListWidget( this );
	listWidget->addItem( qtr("我的共享文件") );
	listWidget->addItem( qtr("云端共享文件") );

	//restoreWidgetPosition( "UserShareSelector", QSize( 500, 450 ) );
}

UserShareSelector::~UserShareSelector()
{
    saveWidgetPosition( "UserShareSelector" );
}

QListWidget* UserShareSelector::getListWidget()
{
	return listWidget;
}

UserShareDialog::UserShareDialog( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	setWindowTitle( qtr("共享文件窗口") );

	/*Top Welecom Message and Logout Button */
	//QGridLayout *mainLayout = new QGridLayout( this );
	QGridLayout *mainLayout = new QGridLayout;

	/* Midle side  Main Windown */
	selector = new UserShareSelector( _p_intf );
	leftSplitter = new QSplitter( Qt::Vertical );
	leftSplitter->addWidget( selector );
	leftSplitter->setMinimumWidth( 100 );
	leftSplitter->setMaximumWidth( 120 );

	/*set the localshare(minidlna) config file path */
	if( access("../sbin/minidlna.conf", F_OK ) >= 0 )
		setConfigPath( "../sbin/minidlna.conf" );
	else if( access("./minidlna-1.1.4/minidlna.conf", F_OK ) >= 0 )
		setConfigPath( "./minidlna-1.1.4/minidlna.conf" );
	else
		qDebug() << " minidlna.conf not found!";

	qDebug() << getConfigPath();

	/* set the Localshare and netshare path according to localshare config file */
	setSharePath( getConfigPath() );

	qDebug() << getSharePath();

	/* Main Windows */
	isPlaying = false;
	localShareTree = initLocalShareTreeView();
	serverShareTree = initServerShareTreeView();

	mainWidget = new QStackedWidget( this );
	mainWidget->addWidget( localShareTree );
	mainWidget->addWidget( serverShareTree );

	QHBoxLayout *layout = new QHBoxLayout( this );
	layout->addWidget( leftSplitter );
	layout->setSpacing( 10 );
	layout->addWidget( mainWidget );
	mainLayout->addLayout(layout, 0, 1);

	setLayout( mainLayout );

	connect( selector->getListWidget(), SIGNAL(currentRowChanged(int)), mainWidget, SLOT(setCurrentIndex(int)) );
	connect( this, SIGNAL( localShareFileChanged() ), this, SLOT(updateLocalShareDialog()) );
	connect( this, SIGNAL( serverShareFileChanged() ), this, SLOT(updateServerShareDialog()) );
	connect( localShareTree, SIGNAL(doubleClicked( const QModelIndex )), this, SLOT( playShareFile(const QModelIndex& ) ) );
}

UserShareDialog::~UserShareDialog()
{
    saveWidgetPosition( "UserShareDialog" );
}

QTreeView* UserShareDialog::initLocalShareTreeView()
{
	QTreeView *tree = new QTreeView;
	QStandardItemModel *model = new QStandardItemModel(0, 5, this);
	model->setHeaderData( 0, Qt::Horizontal, qtr("文件名") );
	model->setHeaderData( 1, Qt::Horizontal, qtr("类  型") );
	model->setHeaderData( 2, Qt::Horizontal, qtr("大  小") );
	model->setHeaderData( 3, Qt::Horizontal, qtr("修改日期") );
	model->setHeaderData( 4, Qt::Horizontal, qtr("路  径") );

	QString path = getSharePath();
	QDir *dir = new QDir( path );
	//QDir *dir = new QDir("/home/lili/share/");

	QFileInfoList entries = dir->entryInfoList();
	foreach(const QFileInfo &file, entries)
	{
		if( file.isDir() )
		{
			if(file.fileName() == "." || file.fileName() == ".." )
				continue;

			QStandardItem *itemDir = addRow( model, file );
			QDir *dir = new QDir( file.filePath() );
			addDirEntries( dir , itemDir );
		}
		else
		{
			addRow( model, file );
		}
	}

	tree->setModel( model );

	return tree;
}

QTreeView* UserShareDialog::initServerShareTreeView()
{
	QTreeView *tree = new QTreeView;
	//QStandardItemModel *model = new QStandardItemModel(0, 5, this);
	QStandardItemModel *model = new QStandardItemModel(0, 5);
	model->setHeaderData( 0, Qt::Horizontal, qtr("文件名") );
	model->setHeaderData( 1, Qt::Horizontal, qtr("类  型") );
	model->setHeaderData( 2, Qt::Horizontal, qtr("大  小") );
	model->setHeaderData( 3, Qt::Horizontal, qtr("修改日期") );
	model->setHeaderData( 4, Qt::Horizontal, qtr("路  径") );

	//getServerShareItems();
	UserOption *user = UserOption::getInstance( p_intf );
	if( !user->isLoaded() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "云端共享提示" ),
				qtr( "用户尚未登陆, 将无法查看云端共享文件！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();
	}

	if( user->isLogin() )
	{
		qDebug() << __func__ << ":" << __LINE__;
		user->nfschina_listMyFile( user->getLUid() );
		QList<QString> list = user->getFileList();
		foreach( const QString& file, list )
		{
			addRow( model, file );
		}
	}

	tree->setModel( model );
	return tree;
}



void UserShareDialog::addDirEntries( QDir* dir, QStandardItem* itemParent )
{
	QFileInfoList entries = dir->entryInfoList();
	foreach( const QFileInfo &file, entries )
	{
		if( file.isDir() )
		{
			if(file.fileName() == "." || file.fileName() == ".." )
				continue;

			QStandardItem *itemDir = addRow( itemParent, file );
			QDir *subdir = new QDir( file.filePath() );

			addDirEntries( subdir , itemDir );
		}
		else
		{
			//QStandardItem *itemFile = addRow( itemParent, file );
			addRow( itemParent, file );
		}
	}
}

QString UserShareDialog::getSharePath()
{
	QString configFile = getConfigPath();
	//QString cmd;
	char cmd[1024] = {0};
	sprintf( cmd, "sed -n --silent '/^media_dir=/p' ");
	strcat( cmd, configFile.toStdString().c_str() );
	strcat( cmd, "  | awk -F, '{print $2}'" );
	//cmd << "sed -n --silent '/^media_dir=/p' " << configFile << " | awk -F, '{print $2}'";
	//qDebug() << "cmd:" << cmd;
	QString path;
	QString cmdbuf = cmd;
	//qDebug() << "cmdbuf:" << cmdbuf;
	FILE * pathFile= popen( cmdbuf.toStdString().c_str(), "r" );
	if( !pathFile )
		return NULL;

	char buf[1024] = {0};
	fread( buf, sizeof(char), sizeof(buf), pathFile );
	pclose( pathFile );
	int len = strlen( buf );
	if( buf[ len-1 ] == '\r' || buf[ len-1 ] == '\n' )
		buf[ len-1 ] = '\0';
	path = buf;

	return path;
}

QStandardItem* UserShareDialog::addRow( QStandardItem* itemParent, const QFileInfo& file )
{
	QStandardItem *item = new QStandardItem( file.fileName() );
	itemParent->appendRow( item );
	if( file.isDir() )
		itemParent->setChild( item->index( ).row(), 1, new QStandardItem( qtr("目录") ) );
	else
		itemParent->setChild( item->index( ).row(), 1, new QStandardItem( qtr("目录") ) );

	itemParent->setChild( item->index( ).row(), 2, new QStandardItem( QString::number(file.size(), 10) ) );
	itemParent->setChild( item->index( ).row(), 3, new QStandardItem( file.lastModified().toString()) );
	itemParent->setChild( item->index( ).row(), 4, new QStandardItem( file.filePath()) );

	return item;
}

QStandardItem* UserShareDialog::addRow( QStandardItemModel* model, const QFileInfo& file )
{
	QStandardItem *item = new QStandardItem( file.fileName() );
	model->appendRow( item );

	if( file.isDir() )
		model->setItem( model->indexFromItem( item ).row(), 1, new QStandardItem( qtr("目录") ) );
	else
		model->setItem( model->indexFromItem( item ).row(), 1, new QStandardItem( qtr("文件") ) );

	model->setItem( model->indexFromItem( item ).row(), 2, new QStandardItem( QString::number(file.size(), 10) ) );
	model->setItem( model->indexFromItem( item ).row(), 3, new QStandardItem( file.lastModified().toString()) );
	model->setItem( model->indexFromItem( item ).row(), 4, new QStandardItem( file.filePath()) );

	return item;
}

QStandardItem* UserShareDialog::addRow( QStandardItemModel* model, const QString& file )
{
	QStandardItem *item = new QStandardItem( file );
	model->appendRow( item );

	model->setItem( model->indexFromItem( item ).row(), 1, new QStandardItem( qtr("文件") ) );

	//model->setItem( model->indexFromItem( item ).row(), 2, new QStandardItem( QString::number(file.size(), 10) ) );
	//model->setItem( model->indexFromItem( item ).row(), 3, new QStandardItem( file.lastModified().toString()) );
	//model->setItem( model->indexFromItem( item ).row(), 4, new QStandardItem( file.filePath()) );

	return item;
}

void UserShareDialog::updateLocalShareDialog()
{
	if( localShareTree )
	{
		mainWidget->removeWidget( localShareTree );
		delete localShareTree;
		localShareTree = initLocalShareTreeView();
		mainWidget->insertWidget( 0, localShareTree );
		mainWidget->setCurrentWidget( localShareTree );
	}
}

void UserShareDialog::updateServerShareDialog()
{
	
	if( serverShareTree )
	{
		mainWidget->removeWidget( serverShareTree );
		delete serverShareTree;
		serverShareTree = initServerShareTreeView();
		mainWidget->insertWidget( 1, serverShareTree );
		mainWidget->setCurrentWidget( serverShareTree );
	}
}

void UserShareDialog::contextMenuEvent(QContextMenuEvent* e)
{
	//TODO 应该在类中增加menu成员，生成menu时先检查是否已生成，如果生成先销毁，再重新生成
	QMenu *menu = new QMenu();
	QAction *addaction = new QAction( qtr("增加本地共享文件"), menu );
	menu->addAction( addaction );
	if( serverShareTree == mainWidget->currentWidget() )
		addaction->setEnabled( false );

	QAction *delaction = new QAction( qtr("删除本地共享文件"), menu );
	menu->addAction( delaction );
	if( serverShareTree == mainWidget->currentWidget() )
		delaction->setEnabled( false );

	QAction *uploadaction = new QAction( qtr("上传为网络共享文件"), menu );
	menu->addAction( uploadaction );
	if( serverShareTree == mainWidget->currentWidget() )
		uploadaction->setEnabled( false );

	menu->addSeparator();

	QAction *downloadaction = new QAction( qtr("下载共享文件到本地"), menu );
	menu->addAction( downloadaction );
	if( localShareTree == mainWidget->currentWidget() )
		downloadaction->setEnabled( false );

	QAction *remotedelaction = new QAction( qtr("删除网络共享文件"), menu );
	menu->addAction( remotedelaction );
	if( localShareTree == mainWidget->currentWidget() )
		remotedelaction->setEnabled( false );

	menu->addSeparator();

	QAction *playaction = new QAction( qtr("播放"), menu );
	menu->addAction( playaction );
	if( isPlaying )
		playaction->setText( qtr("停止") );
	else
		playaction->setText( qtr("播放") );

	QAction *pauseaction = new QAction( qtr("暂停"), menu );
	menu->addAction( pauseaction );
	if( isPause )
		pauseaction->setText( qtr("继续") );
	else
		pauseaction->setText( qtr("暂停") );
	if( !isPlaying )// if no media is playing this action is unselecteable
		pauseaction->setEnabled( false );
#if 0
	QAction *stopaction = new QAction( qtr("停止"), menu );
	menu->addAction( stopaction );
#endif

	connect( addaction, SIGNAL(triggered(bool)), this, SLOT(addLocalShareFile()) );
	connect( delaction, SIGNAL(triggered(bool)), this, SLOT(delLocalShareFile()) );
	connect( uploadaction, SIGNAL(triggered(bool)), this, SLOT(upLoadShareFile()) );
	connect( downloadaction, SIGNAL(triggered(bool)), this, SLOT(downLoadShareFile()) );
	connect( remotedelaction, SIGNAL(triggered(bool)), this, SLOT(deleteRemoteShareFile()) );

	connect( playaction, SIGNAL(triggered(bool)), this, SLOT(playShareFile()) );
	connect( pauseaction, SIGNAL(triggered(bool)), this, SLOT(pausePlaying()) );
	//connect( stopaction, SIGNAL(triggered(bool)), this, SLOT(stopPlaying()) );

	menu->exec( e->globalPos() );
}

void UserShareDialog::playShareFile( const QModelIndex& index )
{
	QAbstractItemModel *m = (QAbstractItemModel*)index.model();
	QString file = m->index(index.row(), 4 ).data().toString();
	QString url = toURI( toNativeSeparators( file ) );
	Open::openMRL( UserShareDialog::getInstance()->p_intf, url, true, true );

}

void UserShareDialog::playShareFile()
{
#if 0
	Open::openMRL( p_intf, "http://192.168.7.88/html/video/baofengyu.mp4", true, true);
	return;
#endif
	/*if any media is playing , stop it */
	isPlaying = !isPlaying;
	qDebug() << "isPlaying = " << isPlaying;
	if( !isPlaying )
	{
		qDebug() << " stopPlaying...";
		playlist_Stop( THEPL );
		return ;
	}

	/*if no media is playing, play the selected media file */
	bool first = true;
	bool pl = true;

	if( localShareTree == mainWidget->currentWidget() )
	{
		qDebug() << " localShareTree";

		QModelIndex index = localShareTree->currentIndex();
		if( !index.isValid() )
			return ;

		QAbstractItemModel *m = (QAbstractItemModel*)index.model();
		QString file = m->index(index.row(), 4 ).data().toString();
		QString url = toURI( toNativeSeparators( file ) );
		qDebug() << "url:" << url ;

		//RecentsMRL::getInstance()->addRecent( url );
		int ret = Open::openMRL( p_intf, url, first, pl);
		qDebug() << "open result: " << ret;
		first = false;
	}
	else if( serverShareTree == mainWidget->currentWidget() )
	{
		qDebug() << "serverShareTree";
		QModelIndex index = serverShareTree->currentIndex();
		if( !index.isValid() )
			return ;

		QAbstractItemModel *m = (QAbstractItemModel*)index.model();
		QString file = m->index(index.row(), 0 ).data().toString();
		UserOption *user = UserOption::getInstance( p_intf );
		
		QString url = user->nfschina_download( user->getLUid(), file );

		qDebug() << "playing file:" << file ;
		qDebug() << "url:" << url ;
		Open::openMRL( p_intf, url, first, pl);
		first = false;
	}

	isPause = false;
}

void UserShareDialog::pausePlaying()
{
	qDebug() << "pausePlaying....";
	//RecentsMRL::getInstance()->addRecent("file:///home/lili/share/baofengyu.mp4");
	isPause = !isPause;
	qDebug() << "isPause = " << isPause;
	playlist_Pause( THEPL );

}
#if 0
void UserShareDialog::stopPlaying()
{
	qDebug() << " stopPlaying...";
	playlist_Stop( THEPL );
}
#endif

QString UserShareDialog::getFilePath( QString basedir )
{
	QModelIndex temp = localShareTree->currentIndex();
	temp = temp.sibling(temp.row(), 0 );
	QString file;
	file = localShareTree->model()->itemData( temp ).values()[0].toString();

	return  basedir + file;
}

void UserShareDialog::removeFile( QString file )
{
	QFile::remove( file );
}

void UserShareDialog::addLocalShareFile()
{
	if(localShareTree != mainWidget->currentWidget() )
		return ;

	//QStringList files = QFileDialog::getOpenFileNames( this, qtr( "Select one or multiple files" ), "/home/lili") ;
	QStringList files = QFileDialog::getOpenFileNames( this, qtr( "Select one or multiple files" ), "~") ;
	foreach( const QString &file, files )
	{
		QString dest;
		if( getSharePath().endsWith('/') )
			dest = getSharePath() + QFileInfo( file ).fileName();
		else
			dest = getSharePath().append('/') + QFileInfo( file ).fileName();
		qDebug() << "add file path:" << dest;
		QFile::link(file, dest );
	}

	emit localShareFileChanged();
}

void UserShareDialog::delLocalShareFile( )
{
	if( localShareTree != mainWidget->currentWidget() )
		return ;

	QModelIndex index = localShareTree->currentIndex();
	if( !index.isValid() )
		return ;

	QAbstractItemModel *m = (QAbstractItemModel*)index.model();
	QString file = m->index(index.row(), 4 ).data().toString();
	qDebug() << "deling file:" << file;
	removeFile( file );

	emit localShareFileChanged();
}

static void *uploadThread( void *arg )
{
	qDebug() << __func__;
	UploadArgs *upInfo = (UploadArgs *) arg;
	qDebug() << " userid = " << upInfo->uid;
	qDebug() << "file = " << upInfo->file;
	qDebug() << "filePath = " << upInfo->filePath;
	UserOption *user = UserOption::getInstance();
	int ret = user->nfschina_upLoad( upInfo->uid, upInfo->file, upInfo->filePath );
	return (void*) ret;
}

static void *downloadThread( void *arg )
{
	qDebug() << __func__;
	DownloadArgs *downInfo = (DownloadArgs *) arg;
	qDebug() << "download url : " << downInfo->url;
	qDebug() << " download file :" << downInfo->file;
	qDebug() << " download blocksize:" << downInfo->block;

	PyObject *pName = PyString_FromString( "download" );
	PyObject *pModule = PyImport_Import( pName );
	if( !pModule )
	{
		printf( "can't find download.py\n" );
		return (void*)false;
	}

	PyObject *pDict = PyModule_GetDict( pModule );
	if( !pDict )
	{
		printf( "can't get dict from download.py\n" );
		return (void*)false;
	}

	PyObject *pFunc = PyDict_GetItemString( pDict, "paxel" );
	if( !pFunc || !PyCallable_Check( pFunc ) )
	{
		printf( "can't find function [paxel]" );
		return (void*)false;
	}

	PyObject *pArgs = PyTuple_New( 3 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "s", downInfo->url.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", downInfo->file.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "i", downInfo->block ) );

	PyObject_CallObject( pFunc, pArgs );

	Py_DECREF( pName );
	Py_DECREF( pArgs );
	Py_DECREF( pModule );

	Py_Finalize();

	return (void*)true;
}

void UserShareDialog::upLoadShareFile()
{
	/*do nothing if currentWidget is not localshare Window */
	if( localShareTree != mainWidget->currentWidget() )
		return;

	QModelIndex index = localShareTree->currentIndex();
	if( !index.isValid() )
		return ;

	QAbstractItemModel *m = (QAbstractItemModel*)index.model();
	QString file = m->index(index.row(), 0 ).data().toString();
	QString filepath = m->index(index.row(), 4 ).data().toString();
	qDebug() << "upload file:" << file;
	qDebug() << "upload filepath:" << filepath;
	UserOption *user = UserOption::getInstance( p_intf );
	if( user->isLoaded() )
	{
		UploadArgs *args = new UploadArgs( user->getLUid(), file, filepath );
		if( pthread_create( &threadUpload, NULL, uploadThread, (void*)args ) != 0 )
			qDebug() << "pthread_create err!";
		qDebug() << "mainthread id = " << pthread_self();
		void *result;
		pthread_join( threadUpload, &result );
		qDebug() << "upload result = " << *(int*)result;

		if( *(int*)result  >= 0 ) //upload success
			emit serverShareFileChanged();
#if 0
		int ret = user->nfschina_upLoad( user->getLUid(), file, filepath );
		emit serverShareFileChanged();
#endif
	}
}

void UserShareDialog::downLoadShareFile()
{
	/* if current widget is not serverShare window , return directly*/
	if(serverShareTree !=  mainWidget->currentWidget() )
		return ;

	QModelIndex index = serverShareTree->currentIndex();
	if( !index.isValid() )
		return ;

	QAbstractItemModel *m = (QAbstractItemModel*)index.model();
	QString file = m->index(index.row(), 0 ).data().toString();
	UserOption *user = UserOption::getInstance( p_intf );
	QString url = user->nfschina_download( user->getLUid(), file );

	//QAbstractItemModel *m = (QAbstractItemModel*)index.model();
	//QString file = m->index(index.row(), 0 ).data().toString();
	//QString filepath = m->index(index.row(), 4 ).data().toString();
	qDebug() << "upload file:" << file;
	qDebug() << "upload url:" << url;
	//UserOption *user = UserOption::getInstance( p_intf );
	if( user->isLoaded() )
	{
		DownloadArgs *args = new DownloadArgs( url, file, 4 );
		if( pthread_create( &threadDownload, NULL, downloadThread, (void*)args ) != 0 )
			qDebug() << "pthread_create err!";
		qDebug() << "mainthread id = " << pthread_self();
		void *result;
		pthread_join( threadDownload, &result );
		qDebug() << "upload result = " << *(int*)result;

		if( *(int*)result  >= 0 ) //upload success
			emit serverShareFileChanged();
	}
}

void UserShareDialog::deleteRemoteShareFile()
{
	/* if current widget is not serverShare window , return directly*/
	if( serverShareTree != mainWidget->currentWidget() )
		return ;

	QModelIndex index = serverShareTree->currentIndex();
	if( !index.isValid() )
		return ;

	QAbstractItemModel *m = (QAbstractItemModel*)index.model();
	QString file = m->index(index.row(), 0 ).data().toString();

	UserOption *user = UserOption::getInstance( p_intf );
	if( user->isLoaded() )
	{
		int ret = user->nfschina_delete( user->getLUid(), file );
		qDebug() << "delete " << file << "result: " << ret;
		emit serverShareFileChanged();
	}
}
