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
#include <QDebug>
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


#include <assert.h>

UserShareSelector::UserShareSelector( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	setContentsMargins( 0, 3, 0, 3 );
	
    setWindowTitle( qtr( "UserShareSelector" ) );
    setWindowRole( "vlc-usershareselector" );
    //setMinimumSize( 350, 300 );
	//setMaximumWidth( 120 );

	listWidget = new QListWidget( this );
	listWidget->addItem( qtr("我的共享文件") );
	//listWidget->addItem( "网络共享文件" );
	listWidget->addItem( qtr("云端共享文件") );
	listWidget->addItem( qtr("共享文件开关") );

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
	QGridLayout *mainLayout = new QGridLayout( this );
	QHBoxLayout *topLayout = new QHBoxLayout;

	title = new QLabel( this );
	//title->setText( "欢迎" + UserDialog::getInstance()->getLUsername() );

	logoutBtn = new QPushButton( this );
	logoutBtn->setText( qtr("退出") );
	logoutBtn->setMaximumSize(80, 40);
	topLayout->addWidget( title );
	topLayout->setSpacing( 10 );
	topLayout->addWidget( logoutBtn );
	mainLayout->addLayout( topLayout, 0, 1 );

	/* Midle side  Main Windown */
	selector = new UserShareSelector( _p_intf );
	leftSplitter = new QSplitter( Qt::Vertical );
	leftSplitter->addWidget( selector );
	leftSplitter->setMinimumWidth( 100 );
	leftSplitter->setMaximumWidth( 120 );

	/* Main Windows */
	localShareTree = initLocalShareTreeView();
	//tree->setRootIndex( model->index( p_intf->p_sys->filepath ) );
	//tree->setRootIndex( model->index( "/home/lili/share" ) );
	
//	QLabel *label2 = new QLabel("窗口2");
	//QLabel *label3 = new QLabel("窗口3");
	serverShareTree = initServerShareTreeView();
	shareControlWidget = new ShareStateWidget( _p_intf );

	//QDirModel *model = new QDirModel(0, 5, this);


	mainWidget = new QStackedWidget( this );

	mainWidget->addWidget( localShareTree );
	//mainWidget->addWidget( label2 );
	mainWidget->addWidget( serverShareTree );
	mainWidget->addWidget( shareControlWidget );

	QHBoxLayout *layout = new QHBoxLayout( this );
	layout->addWidget( leftSplitter );
	layout->setSpacing( 10 );
	layout->addWidget( mainWidget );
	mainLayout->addLayout(layout, 1, 1);

#if 0
	/*buttom add and del Button */
	QHBoxLayout *buttomLayout = new QHBoxLayout( this );
	buttomLayout->addStretch(20);
	QPushButton * delBtn = new QPushButton( this );
	delBtn->setText( "删除" );
	delBtn->setMaximumSize( 80, 40 );
	buttomLayout->addWidget( delBtn );
	QPushButton *addBtn = new QPushButton( this );
	addBtn->setText( "增加" );
	addBtn->setMaximumSize( 80, 40 );
	buttomLayout->addWidget( addBtn );
	mainLayout->addLayout( buttomLayout, 2, 1);
	connect(delBtn, SIGNAL(clicked()), this, SLOT(delShareFile()) );
	connect(addBtn, SIGNAL(clicked()), this, SLOT(addShareFile()) );
#endif

	connect( selector->getListWidget(), SIGNAL(currentRowChanged(int)), mainWidget, SLOT(setCurrentIndex(int)) );
	connect( this, SIGNAL( shareFileChanged() ), this, SLOT(updateUserShareDialog()) );
	//connect( UserDialog::getInstance(), SIGNAL( userChanged() ), this, SLOT(updateUserShareDialog()) );
	connect( logoutBtn, SIGNAL(clicked()), this, SLOT(showUserDialog()) );
	connect( localShareTree, SIGNAL(doubleClicked( const QModelIndex )), this, SLOT( playShareFile(const QModelIndex& ) ) );
}

UserShareDialog::~UserShareDialog()
{
    saveWidgetPosition( "UserShareDialog" );
}

void ShareStateWidget::toggleLocalShareState()
{
	b_localShared = ! b_localShared;

	if( b_localShared )
	{
		localShareState->setText( qtr("局域网共享已开启") );
		localShareBtn->setText( qtr("关闭局域网共享") );
	}
	else
	{
		localShareState->setText( qtr("局域网共享已关闭") );
		localShareBtn->setText( qtr("开启局域网共享") );
	}

	doLocalShare( b_localShared ); //TODO 打开或关闭本地共享的后台工作
}

void ShareStateWidget::toggleNetShareState()
{

	qDebug() << __func__ ;
	b_netShared = !b_netShared;

	if( b_netShared )
	{
		netShareState->setText( qtr("广域网共享已开启") );
		netShareBtn->setText( qtr("关闭广域网共享") );
	}
	else
	{
		netShareState->setText( qtr("广域网共享已关闭") );
		netShareBtn->setText( qtr("开启广域网共享") );
	}

	doNetShare( b_netShared ); //TODO 打开或关闭网络共享的后台工作
}

ShareStateWidget::ShareStateWidget( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	localShareState = new QLabel( this );
	localShareBtn = new QPushButton( this );

	if( b_localShared )
	{
		localShareState->setText( qtr("局域网共享已开启") );
		localShareBtn->setText( qtr("关闭局域网共享") );
	}
	else
	{
		localShareState->setText( "局域网共享已关闭" );
		localShareBtn->setText( "开启局域网共享" );
	}

	localShareBtn->setMaximumSize( 100, 30 );
	localShareState->setBuddy( localShareBtn );

	QHBoxLayout *localShareLayout = new QHBoxLayout;
	localShareLayout->addStretch(5);
	localShareLayout->addWidget( localShareState );
	localShareLayout->addStretch(15);
	localShareLayout->addWidget( localShareBtn );
	localShareLayout->addStretch(5);

	netShareState = new QLabel( this );
	netShareBtn = new QPushButton( this );
	if( b_netShared )
	{
		netShareState->setText( "广域网共享已开启" );
		netShareBtn->setText( "关闭广域网共享" );
	}
	else
	{
		netShareState->setText( "广域网共享已关闭" );
		netShareBtn->setText( "开启广域网共享" );
	}
	netShareBtn->setMaximumSize( 100, 30 );
	netShareState->setBuddy( netShareBtn );

	QHBoxLayout *netShareLayout = new QHBoxLayout;
	netShareLayout->addStretch(5);
	netShareLayout->addWidget( netShareState );
	netShareLayout->addStretch(15);
	netShareLayout->addWidget( netShareBtn );
	netShareLayout->addStretch(5);

	QVBoxLayout *mainLayout = new QVBoxLayout( this );
	mainLayout->addStretch(1);
	mainLayout->addLayout( localShareLayout );
	mainLayout->addLayout( netShareLayout );
	mainLayout->addStretch(19);

	setLayout( mainLayout );

	connect( localShareBtn, SIGNAL(clicked()), this, SLOT( toggleLocalShareState()) );
	connect( netShareBtn, SIGNAL(clicked()), this, SLOT( toggleNetShareState()) );
}

void ShareStateWidget::doLocalShare(bool isOpen )
{
	if( isOpen )
		openLocalShare();
	else
		closeLocalShare();
}

void ShareStateWidget::openLocalShare()
{
	qDebug() << "opening Local Share .......";
	//TODO
}

void ShareStateWidget::closeLocalShare()
{
	qDebug() << "closing Local Share .......";
	//TODO
}

void ShareStateWidget::doNetShare(bool isOpen )
{
	if( isOpen )
		openNetShare();
	else
		closeNetShare();
}

void ShareStateWidget::openNetShare()
{
	qDebug() << "opening Net Share .......";
	//TODO
}

void ShareStateWidget::closeNetShare()
{
	qDebug() << "closing Net Share .......";
	//TODO
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

	QDir *dir = new QDir("/home/lili/share/");

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

void UserShareDialog::updateUserShareDialog()
{
	if( title )
	{
		//title->setText( "欢迎" + UserDialog::getInstance()->getLUsername() );
		//qDebug() << UserDialog::getInstance()->getLUsername();
	}
	
	if( localShareTree )
	{
		mainWidget->removeWidget( localShareTree );
		delete localShareTree;
		localShareTree = initLocalShareTreeView();
		mainWidget->insertWidget( 0, localShareTree );
		mainWidget->setCurrentWidget( localShareTree );
	}

#if 0
	if( localShareTree )
	{
		QDirModel *model = new QDirModel;
		localShareTree->setModel( model );
		localShareTree->setRootIndex( model->index( "/home/lili/share" ) );
	}
#endif
}

void UserShareDialog::showUserDialog()
{
	toggleVisible();
	//UserDialog::getInstance()->setLogin( false );
	//UserDialog::getInstance()->toggleVisible();
}

void UserShareDialog::contextMenuEvent(QContextMenuEvent* e)
{
	//printf("------------------------------func:%s---------------------------------------------------\n", __func__);
	//TODO 应该在类中增加menu成员，生成menu时先检查是否已生成，如果生成先销毁，再重新生成
	QMenu *menu = new QMenu();
	QAction *addaction = new QAction( qtr("增加共享文件"), menu );
	menu->addAction( addaction );

	QAction *delaction = new QAction( qtr("删除共享文件"), menu );
	menu->addAction( delaction );

	menu->addSeparator();

	QAction *uploadaction = new QAction( qtr("上传为共享文件"), menu );
	menu->addAction( uploadaction );

	QAction *downloadaction = new QAction( qtr("下载共享文件"), menu );
	menu->addAction( downloadaction );

	menu->addSeparator();

	QAction *playaction = new QAction( qtr("播放"), menu );
	menu->addAction( playaction );

	QAction *pauseaction = new QAction( qtr("暂停"), menu );
	menu->addAction( pauseaction );
	if( isPause )
		pauseaction->setText( qtr("继续") );

	QAction *stopaction = new QAction( qtr("停止"), menu );
	menu->addAction( stopaction );

	connect( addaction, SIGNAL(triggered(bool)), this, SLOT(addShareFile()) );
	connect( delaction, SIGNAL(triggered(bool)), this, SLOT(delShareFile()) );
	connect( uploadaction, SIGNAL(triggered(bool)), this, SLOT(upLoadShareFile()) );
	connect( downloadaction, SIGNAL(triggered(bool)), this, SLOT(downLoadShareFile()) );

	connect( playaction, SIGNAL(triggered(bool)), this, SLOT(playShareFile()) );
	connect( pauseaction, SIGNAL(triggered(bool)), this, SLOT(pausePlaying()) );
	connect( stopaction, SIGNAL(triggered(bool)), this, SLOT(stopPlaying()) );

	menu->exec( e->globalPos() );
}

void UserShareDialog::playShareFile( const QModelIndex& index )
{
	QAbstractItemModel *m = (QAbstractItemModel*)index.model();
	QString file = m->index(index.row(), 4 ).data().toString();
	QString url = toURI( toNativeSeparators( file ) );
	Open::openMRL( UserShareDialog::getInstance()->p_intf, url, true, true );

#if 0
	for( int columnIndex = 0; columnIndex < m->columnCount(); ++columnIndex )
	{
		QModelIndex x = m->index( index.row(), columnIndex );
		QString s = x.data().toString();
		qDebug() << s;
		QMessageBox::about(this,s,s);
	}
#endif
}

void UserShareDialog::playShareFile()
{
#if 0
	QString file = "http://192.168.7.88:8200/MediaItems/27.flv";
	Open::openMRL( UserShareDialog::getInstance()->p_intf, file, true, true);
#endif

	bool first = true;
	bool pl = true;

	toggleVisible();
	QString file = getFilePath( "/home/lili/share/" );
	QString url = toURI( toNativeSeparators( file ) );
	qDebug() << url;
	Open::openMRL( UserShareDialog::getInstance()->p_intf, url, first, pl);
	first = false;
}

void UserShareDialog::pausePlaying()
{
	qDebug() << "pausePlaying....";
	//RecentsMRL::getInstance()->addRecent("file:///home/lili/share/baofengyu.mp4");
	isPause = !isPause;
	playlist_Pause( THEPL );
	
}

void UserShareDialog::stopPlaying()
{
	qDebug() << " stopPlaying...";
	playlist_Stop( THEPL );
}

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

void UserShareDialog::addShareFile()
{
	QStringList files = QFileDialog::getOpenFileNames( this, qtr( "Select one or multiple files" ), "/home/lili") ;
	foreach( const QString &file, files )
	{
		//qDebug() << QFileInfo( file ).path();
		//qDebug() << QFileInfo( file ).fileName();
		//QString dest = "/home/lili/share/" + file.rightRef(file.count() - file.lastIndexOf('/') - 1 ).toString();
		QString dest = "/home/lili/share/" + QFileInfo( file ).fileName();
		QFile::link(file, dest );
	}

	emit shareFileChanged();
}

void UserShareDialog::delShareFile( )
{
	QModelIndex index = localShareTree->currentIndex();
	if( !index.isValid() )
		return ;

	//QStandardItem* currentItem = localShareModel->itemFromIndex( index );

	QAbstractItemModel *m = (QAbstractItemModel*)index.model();
	QString file = m->index(index.row(), 4 ).data().toString();
	qDebug() << "deling file:" << file;
	removeFile( file );

#if 0
	QDirModel *model = new QDirModel;
	if( model->fileInfo( index ).isDir() )
	{
		//TODO
		//model->rmdir( index );
	}
	else
	{

		QString selectedFile = getFilePath( "/home/lili/share/" );
		removeFile( selectedFile );
	}
#endif


	emit shareFileChanged();
}

void UserShareDialog::upLoadShareFile()
{
}

void UserShareDialog::downLoadShareFile()
{
}
