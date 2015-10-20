/*****************************************************************************
 * user.cpp : User dialogs
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
#include "dialogs/user/task.hpp"
#include "dialogs/user/useroption.hpp"
#include "util/qt_dirs.hpp"
#include <QProgressDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QProgressBar>
#include <QStringList>
#include <QSplitter>
#include <QLabel>
#include <QListWidget>
#include <QGridLayout>
#include <QStackedWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QDebug>

#include <python2.7/Python.h>

TaskSelector::TaskSelector( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	setContentsMargins( 0, 3, 0, 3 );
	
    setWindowTitle( qtr( "TaskSelector" ) );
    setWindowRole( "vlc-taskselector" );
    //setMinimumSize( 350, 300 );
	//setMaximumWidth( 120 );

	listWidget = new QListWidget( this );
	listWidget->addItem( qtr("我的下载") );
	listWidget->addItem( qtr("我的上传") );

	restoreWidgetPosition( "UserShareSelector", QSize( 500, 450 ) );
}

TaskSelector::~TaskSelector()
{
    saveWidgetPosition( "TaskSelector" );
}

QListWidget* TaskSelector::getListWidget()
{
	return listWidget;
}


TaskDialog::TaskDialog( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	qDebug() << "func:" << __func__;
	setWindowTitle( qtr( "我的任务" ) );

	/* Left side */
	QGridLayout *mainLayout = new QGridLayout;
	selector = new TaskSelector( _p_intf );
	leftSplitter = new QSplitter( Qt::Vertical );
	leftSplitter->addWidget( selector );
	leftSplitter->setMinimumWidth( 100 );
	leftSplitter->setMaximumWidth( 120 );

	/* Right side */
	downloadTree = initDownloadTreeView();
	uploadTree = initUploadTreeView();
	mainWidget = new QStackedWidget( this );
	mainWidget->addWidget( downloadTree );
	mainWidget->addWidget( uploadTree );

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget( leftSplitter );
	layout->setSpacing( 10 );
	layout->addWidget( mainWidget );
	mainLayout->addLayout( layout, 0, 1 );

	setLayout( mainLayout );

	connect( selector->getListWidget(), SIGNAL( currentRowChanged(int)), mainWidget, SLOT(setCurrentIndex(int)));
}

TaskDialog::~TaskDialog()
{
    //saveWidgetPosition( "Task" );
}

QTreeView* TaskDialog::initDownloadTreeView()
{
	qDebug() << "func:" << __func__;

	QTreeView *tree = new QTreeView;
	//QStandardItemModel *model = new QStandardItemModel( 0, 3, this );
	downloadModel = new QStandardItemModel( 0, 4, this );
	downloadModel->setHeaderData( 0, Qt::Horizontal, qtr("文件名") );
	downloadModel->setHeaderData( 1, Qt::Horizontal, qtr("进度") );
	downloadModel->setHeaderData( 2, Qt::Horizontal, qtr("状态") );
	downloadModel->setHeaderData( 3, Qt::Horizontal, qtr("id") );
	tree->setModel( downloadModel );
	tree->hideColumn(3);//隐藏列，存储下载用户的id信息,用于标记是哪个用户对文件的操作

#if 0
	addDownloadItem("downloadtest1.mp4", 10, "Downloading...");//todo delete
	addDownloadItem("downloadtest2.mp4", 20, "Downloading...");//todo delete
	addDownloadItem("downloadtest3.mp4", 30, "Downloading...");//todo delete
	QModelIndex index = getDownloadItemIndex("downloadtest2.mp4");
	if(index.isValid())
	{
		printf("index of download data:%s\n", index.data().toString().toStdString().c_str());
	}
	updateDownloadItem("downloadtest2.mp4", 100, "DownloadFinished");
#endif

	return tree;
}

QTreeView* TaskDialog::initUploadTreeView()
{
	qDebug() << "func:" << __func__;

	QTreeView *tree = new QTreeView;
	//QStandardItemModel *model = new QStandardItemModel( 0, 3, this );
	uploadModel = new QStandardItemModel( 0, 4, this );
	uploadModel->setHeaderData( 0, Qt::Horizontal, qtr("文件名") );
	uploadModel->setHeaderData( 1, Qt::Horizontal, qtr("进度") );
	uploadModel->setHeaderData( 2, Qt::Horizontal, qtr("状态") );
	uploadModel->setHeaderData( 3, Qt::Horizontal, qtr("id") );
	tree->setModel( uploadModel );
	tree->hideColumn(3);//隐藏列，存储下载用户的id信息,用于标记是哪个用户对文件的操作

	initUploadItems();//添加登陆用户的相关上传文件记录
#if 0
	addUploadItem("uploadtest1.mp4", 10, "Uploading...");//todo delete
	addUploadItem("uploadtest2.mp4", 20, "Uploading...");//todo delete
	addUploadItem("uploadtest3.mp4", 30, "Uploading...");//todo delete
	//uploadModel->takeRow(1);
	QModelIndex index = getUploadItemIndex("uploadtest5.mp4");
	if(index.isValid())
	{
		printf("index of upload data:%s\n", index.data().toString().toStdString().c_str());
	}

	updateUploadItem("uploadtest1.mp4", 100, "UploadFinished");
#endif

	return tree;
}

void TaskDialog::initUploadItems()
{
	QList<Task> uploadTasks;
	int size = uploadSettings.beginReadArray("");
}

void TaskDialog::addUploadItem( const QString filename, int process, const QString state )
{
	int uid = UserOption::getInstance( p_intf )->getLUid();
	QStandardItemModel *model = getUploadModel();
	QStandardItem *item = new QStandardItem( filename );
	model->appendRow( item );
	QString pro = QString::number(process).append("%");
	model->setItem(model->indexFromItem(item).row(), 1, new QStandardItem(pro));
	model->setItem(model->indexFromItem(item).row(), 2, new QStandardItem(state));
	model->setItem(model->indexFromItem(item).row(), 3, new QStandardItem(QString::number(uid)));
}

void TaskDialog::addDownloadItem(const QString filename, int process, const QString state )
{
	int uid = UserOption::getInstance( p_intf )->getLUid();
	QStandardItemModel *model = getDownloadModel();
	QStandardItem *item = new QStandardItem( filename );
	model->appendRow( item );
	QString pro = QString::number(process).append("%");
	model->setItem(model->indexFromItem(item).row(), 1, new QStandardItem(pro));
	model->setItem(model->indexFromItem(item).row(), 2, new QStandardItem(state));
	model->setItem(model->indexFromItem(item).row(), 3, new QStandardItem(QString::number(uid)));
}

QModelIndex TaskDialog::getUploadItemIndex( const QString filename )
{
	QStandardItemModel *model = getUploadModel();
	QModelIndex index;
	printf("upload rowcount:%d\n", model->rowCount());
	int rowCount = model->rowCount();
	int i;
	for(i = 0; i < rowCount; i++)
	{
		QModelIndex tmpindex = model->index(i, 0, QModelIndex());
		printf( "upload index.data = %s\n", tmpindex.data().toString().toStdString().c_str() );
		if( tmpindex.data().toString() == filename )
		{
			printf("%s is at %d row\n", filename.toStdString().c_str(), i);
			index = tmpindex;
			break;
		}
	}

	return index;
}

QModelIndex TaskDialog::getDownloadItemIndex( const QString filename )
{
	QStandardItemModel *model = getDownloadModel();
	QModelIndex index;
	printf("download rowcount:%d\n", model->rowCount());
	int rowCount = model->rowCount();
	int i;
	for(i = 0; i < rowCount; i++)
	{
		QModelIndex tmpindex = model->index(i, 0, QModelIndex());
		printf("download index.data = %s\n", tmpindex.data().toString().toStdString().c_str());
		if(tmpindex.data().toString() == filename)
		{
			printf("%s is at %d row\n", filename.toStdString().c_str(), i);
			index = tmpindex;
			break;
		}
	}

	return index;
}

void TaskDialog::updateUploadItem( const QString filename, int process, const QString state )
{
	QModelIndex index = getUploadItemIndex( filename );
	if(!index.isValid())
	{
		printf("Can't find uploaditem named %s\n", filename.toStdString().c_str());
		return;
	}

	QStandardItemModel *model = getUploadModel();
	QString pro = QString::number(process).append("%");
	model->setItem(index.row(), 1, new QStandardItem(pro));
	model->setItem(index.row(), 2, new QStandardItem(state));
}

void TaskDialog::updateDownloadItem( const QString filename, int process, const QString state )
{
	QModelIndex index = getDownloadItemIndex( filename );
	if(!index.isValid())
	{
		printf("Can't find downloaditem named %s\n", filename.toStdString().c_str());
		return;
	}

	QStandardItemModel *model = getDownloadModel();
	QString pro = QString::number(process).append("%");
	model->setItem(index.row(), 1, new QStandardItem(pro));
	model->setItem(index.row(), 2, new QStandardItem(state));
}
