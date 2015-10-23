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
	organization = "FangDePlayer";
	application = "TaskDialog";

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

	connect(downloadTree, SIGNAL(doubleClicked(const QModelIndex)), this, SLOT(toggleDownloadState(const QModelIndex&)));
	connect(uploadTree, SIGNAL(doubleClicked(const QModelIndex)), this, SLOT(toggleUploadState(const QModelIndex&)));
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
	downloadModel = new QStandardItemModel( 0, 6, this );
	downloadModel->setHeaderData( 0, Qt::Horizontal, qtr("文件名") );
	downloadModel->setHeaderData( 1, Qt::Horizontal, qtr("进度") );
	downloadModel->setHeaderData( 2, Qt::Horizontal, qtr("状态") );
	downloadModel->setHeaderData( 3, Qt::Horizontal, qtr("id") );
	downloadModel->setHeaderData( 4, Qt::Horizontal, qtr("url") );
	downloadModel->setHeaderData( 5, Qt::Horizontal, qtr("threadId") );
	tree->setModel( downloadModel );
	tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
#if 0
	tree->hideColumn(3);//隐藏列，存储下载用户的id信息,用于标记是哪个用户对文件的操作
	tree->hideColumn(4);
	tree->hideColumn(5);
#endif

	int uid = UserOption::getInstance( p_intf )->getLUid();
	QSettings settings(organization, application);
	initDownloadItems(settings, uid);//添加登陆用户的相关上传文件记录

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
	uploadModel = new QStandardItemModel( 0, 6, this );
	uploadModel->setHeaderData( 0, Qt::Horizontal, qtr("文件名") );
	uploadModel->setHeaderData( 1, Qt::Horizontal, qtr("进度") );
	uploadModel->setHeaderData( 2, Qt::Horizontal, qtr("状态") );
	uploadModel->setHeaderData( 3, Qt::Horizontal, qtr("id") );
	uploadModel->setHeaderData( 4, Qt::Horizontal, qtr("filePath") );
	uploadModel->setHeaderData( 5, Qt::Horizontal, qtr("threadId") );
	tree->setModel( uploadModel );
	tree->setEditTriggers(QAbstractItemView::NoEditTriggers);//禁止编辑
#if 0
	tree->hideColumn(3);//隐藏列，存储下载用户的id信息,用于标记是哪个用户对文件的操作
	tree->hideColumn(4);
	tree->hideColumn(5);
#endif

	int uid = UserOption::getInstance( p_intf )->getLUid();
	QSettings settings(organization, application);
	initUploadItems(settings, uid);//添加登陆用户的相关上传文件记录
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

void TaskDialog::initUploadItems(QSettings &settings, int uid)
{
	QString uploadkey = "upload-";
	uploadkey.append(QString::number(uid));

	settings.beginGroup("");
	foreach(QString key, settings.childKeys())
	{
		if(key.startsWith(uploadkey))
		{
			QString tmp = settings.value(key).toString();
			qDebug() << "upload tmp: " << tmp;
			QStringList list = tmp.split("-");
			QString file = list.at(0);
			int process = list.at(1).toInt();
			QString state = list.at(2);
			QString path = list.at(3);
			if(process == 100)
				addUploadItem(file, process, state, uid, path);
			else
				addUploadItem(file, process, qtr("已停止"), uid, path);
		}
	}
	settings.endGroup();
}

void TaskDialog::initDownloadItems(QSettings &settings, int uid)
{
	QString downloadkey = "download-";
	downloadkey.append(QString::number(uid));
	
	settings.beginGroup("");
	foreach(QString key, settings.childKeys())
	{
		if(key.startsWith(downloadkey))
		{
			QString tmp = settings.value(key).toString();
			qDebug() << "download tmp: " << tmp;
			QStringList list = tmp.split("-");
			QString file= list.at(0);
			int process = list.at(1).toInt();
			QString state = list.at(2);
			QString url = list.at(3);
			if(process == 100)
				addDownloadItem(file, process, state, uid, url);
			else
				addDownloadItem(file, process, qtr("已停止"), uid, url);
		}
	}
	settings.endGroup();
}

void TaskDialog::addUploadItem( const QString filename, int process, const QString state, int uid, const QString path, pthread_t thread_id )
{
	qDebug() << __func__ << "thread_id = " << thread_id;
	QString pro = QString::number(process).append("%");
	QStandardItemModel *model = getUploadModel();

	QModelIndex index = getUploadItemIndex(filename);
	if(index.isValid())
	{
		model->setItem(index.row(), 1, new QStandardItem(pro));
		model->setItem(index.row(), 2, new QStandardItem(state));
		model->setItem(index.row(), 3, new QStandardItem(QString::number(uid)));
		model->setItem(index.row(), 4, new QStandardItem(path));
		if(thread_id > 0)
			model->setItem(index.row(), 5, new QStandardItem(QString::number(thread_id)));
	}
	else
	{
		QStandardItem *item = new QStandardItem( filename );
		model->appendRow( item );
		model->setItem(model->indexFromItem(item).row(), 1, new QStandardItem(pro));
		model->setItem(model->indexFromItem(item).row(), 2, new QStandardItem(state));
		model->setItem(model->indexFromItem(item).row(), 3, new QStandardItem(QString::number(uid)));
		model->setItem(model->indexFromItem(item).row(), 4, new QStandardItem(path));
		if(thread_id > 0)
			model->setItem(model->indexFromItem(item).row(), 5, new QStandardItem(QString::number(thread_id)));
	}
}

void TaskDialog::addDownloadItem(const QString filename, int process, const QString state , int uid, const QString url, pthread_t thread_id )
{
	qDebug() << __func__ << "thread_id = " << thread_id;
	QString pro = QString::number(process).append("%");
	QStandardItemModel *model = getDownloadModel();

	QModelIndex index = getDownloadItemIndex(filename);
	if(index.isValid())
	{
		model->setItem(index.row(), 1, new QStandardItem(pro));
		model->setItem(index.row(), 2, new QStandardItem(state));
		model->setItem(index.row(), 3, new QStandardItem(QString::number(uid)));
		model->setItem(index.row(), 4, new QStandardItem(url));
		if(thread_id > 0)
			model->setItem(index.row(), 5, new QStandardItem(QString::number(thread_id)));
	}
	else
	{
		QStandardItem *item = new QStandardItem( filename );
		model->appendRow( item );
		model->setItem(model->indexFromItem(item).row(), 1, new QStandardItem(pro));
		model->setItem(model->indexFromItem(item).row(), 2, new QStandardItem(state));
		model->setItem(model->indexFromItem(item).row(), 3, new QStandardItem(QString::number(uid)));
		model->setItem(model->indexFromItem(item).row(), 4, new QStandardItem(url));
		if(thread_id > 0)
			model->setItem(model->indexFromItem(item).row(), 5, new QStandardItem(QString::number(thread_id)));
	}
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

QString TaskDialog::buildKeyString(const QString type, int uid, const QString file)
{
	/*
	 * type-uid-filename
	 * eg. "upload-2-aaa.mp4"
	 * */
	QString key = type;
	key.append("-").append(QString::number(uid)).append("-").append(file);

	return key;
}

QString TaskDialog::buildValueString(const QString file, int process, const QString state, const QString url)
{
	/*
	 * filename-process-state
	 * eg. upload: "aaa.mp4-30-Uploading-/home/nfschina/test.mp4"
	 * eg. download:"aaa.mp4-30-Uploading-http://192.168.7.97:80/download/157/test.mp4"
	 * */
	QString value = file;
	value.append("-").append(QString::number(process)).append("-").append(state).append("-").append(url);

	return value;
}

void TaskDialog::saveNewTask(const QString type, int uid, const QString file, int process, const QString state, const QString url)
{
	QSettings settings(organization, application);
	QString key = buildKeyString(type, uid, file);
	QString value = buildValueString(file, process, state, url);
	settings.setValue(key, value);
}

void TaskDialog::deleteTask(const QString type, int uid, const QString file)
{
	QSettings settings(organization, application);
	QString key = buildKeyString(type, uid, file);
	if(settings.contains(key))
		settings.remove(key);
}

void TaskDialog::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = new QMenu();
	QAction *stopAction = new QAction(qtr("停止"), menu);
	menu->addAction(stopAction);

	QAction *continueAction = new QAction(qtr("继续"), menu);
	menu->addAction(continueAction);

	QAction *deleteAction = new QAction(qtr("删除任务"), menu);
	menu->addAction(deleteAction);

	connect(stopAction, SIGNAL(triggered(bool)), this, SLOT(stopItemTask()));
	connect(continueAction, SIGNAL(triggered(bool)), this, SLOT(continueItemTask()));
	connect(deleteAction, SIGNAL(triggered(bool)), this, SLOT(deleteItemTask()));

	menu->exec(event->globalPos());
}

void TaskDialog::stopItemTask()
{
	qDebug() << __func__;
	/*stop download or upload task*/
	if(downloadTree == mainWidget->currentWidget())
	{
		QModelIndex index = downloadTree->currentIndex();
		QAbstractItemModel *model = (QAbstractItemModel*)index.model();
		QString file = model->index(index.row(), 0).data().toString();
		pthread_t thread_id = model->index(index.row(), 5).data().toInt();
		pthread_cancel(thread_id);

		/*update item state for Task Window*/
		int process = model->index(index.row(), 1).data().toInt();
		QString state = qtr("已停止");
		qDebug() << file << "->thread_id = " << thread_id;
		updateDownloadItem(file, process, state);
	}
	else if(uploadTree == mainWidget->currentWidget())
	{
		QModelIndex index = uploadTree->currentIndex();
		QAbstractItemModel *model = (QAbstractItemModel*)index.model();
		QString file = model->index(index.row(), 0).data().toString();
		pthread_t thread_id = model->index(index.row(), 5).data().toInt();
		pthread_cancel(thread_id);

		/*update item state for Task Window*/
		int process = model->index(index.row(), 1).data().toInt();
		QString state = qtr("已停止");
		qDebug() << file << "->thread_id = " << thread_id;
		updateUploadItem(file, process, state);
	}
}

void TaskDialog::continueItemTask()
{
	qDebug() << __func__;
	/*continue download/upload task*/
	if(downloadTree == mainWidget->currentWidget())
	{
		QModelIndex index = downloadTree->currentIndex();
		QAbstractItemModel *model = (QAbstractItemModel*)index.model();
		QString file = model->index(index.row(), 0).data().toString();
		QString url = model->index(index.row(), 4).data().toString();
		UserOption *user = UserOption::getInstance(p_intf);
		user->downloadCloudShareFile(url, file);
	}
	else if(uploadTree == mainWidget->currentWidget())
	{
		QModelIndex index = uploadTree->currentIndex();
		QAbstractItemModel *model = (QAbstractItemModel*)index.model();
		QString file = model->index(index.row(), 0).data().toString();
		int uid = model->index(index.row(), 3).data().toInt();
		QString filepath = model->index(index.row(), 4).data().toString();
		UserOption *user = UserOption::getInstance(p_intf);
		user->nfschina_upLoad(uid, file.toStdString().c_str(), filepath.toStdString().c_str());
	}

	/*update item state form Task Window*/
}

void TaskDialog::deleteItemTask()
{
	qDebug() << __func__;
	/*delete download/upload task*/
	if(downloadTree == mainWidget->currentWidget())
	{
		QModelIndex index = downloadTree->currentIndex();
		QAbstractItemModel *model = (QAbstractItemModel*)index.model();
		QString file = model->index(index.row(), 0).data().toString();
		QString state = model->index(index.row(), 2).data().toString();
		int uid = model->index(index.row(), 3).data().toInt();

		/*如果选中文件正在下载，先停止文件下载*/
		if(state.startsWith(qtr("下载中")))
		{
			pthread_t thread_id = model->index(index.row(), 5).data().toInt();
			pthread_cancel(thread_id);
		}

		/*delete item information from QSettings*/
		deleteTask("download", uid, file);

		/*delete item from Task window*/
		model->removeRow(index.row());

	}
	else if(uploadTree == mainWidget->currentWidget())
	{
		QModelIndex index = uploadTree->currentIndex();
		QAbstractItemModel *model = (QAbstractItemModel*)index.model();
		QString file = model->index(index.row(), 0).data().toString();
		QString state = model->index(index.row(), 2).data().toString();
		int uid = model->index(index.row(), 3).data().toInt();
		/*stop upload task if the selected file is Uploading....*/
		if(state.startsWith(qtr("上传中")))
		{
			pthread_t thread_id = model->index(index.row(), 5).data().toInt();
			pthread_cancel(thread_id);
		}
		/*delete item information from QSettings*/
		deleteTask("upload", uid, file);
		
		/*delete item from Task Window*/
		model->removeRow(index.row());
	}
}

void TaskDialog::toggleUploadState(const QModelIndex &index)
{
	UserOption *user = UserOption::getInstance(p_intf);
	QAbstractItemModel *model = (QAbstractItemModel*)index.model();
	QString state = model->index(index.row(), 2).data().toString();
	if(state.startsWith(qtr("已停止")))
	{
		QString file = model->index(index.row(), 0).data().toString();
		int uid = model->index(index.row(), 3).data().toInt();
		QString filepath = model->index(index.row(), 4).data().toString();
		user->nfschina_upLoad(uid, file.toStdString().c_str(), filepath.toStdString().c_str());
	}
	else if(state.startsWith(qtr("上传中")))
	{
		QString file = model->index(index.row(), 0).data().toString();
		int process = model->index(index.row(), 1).data().toInt();
		QString newstate = qtr("已停止");
		updateUploadItem(file, process, newstate);

		pthread_t thread_id = model->index(index.row(), 5).data().toInt();
		pthread_cancel(thread_id);
	}
}

void TaskDialog::toggleDownloadState(const QModelIndex &index)
{
	UserOption *user = UserOption::getInstance(p_intf);
	QAbstractItemModel *model = (QAbstractItemModel*)index.model();
	QString state = model->index(index.row(), 2).data().toString();
	if(state.startsWith(qtr("已停止")))
	{
		QString file = model->index(index.row(), 0).data().toString();
		QString url = model->index(index.row(), 4).data().toString();
		user->downloadCloudShareFile(url, file);
	}
	else if(state.startsWith(qtr("下载中")))
	{
		QString file = model->index(index.row(), 0).data().toString();
		int process = model->index(index.row(), 1).data().toInt();
		QString newstate = qtr("已停止");
		updateDownloadItem(file, process, newstate);

		pthread_t thread_id = model->index(index.row(), 5).data().toInt();
		pthread_cancel(thread_id);
	}
}

