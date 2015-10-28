/*****************************************************************************
 * task.hpp : User tasks dialogs
 ****************************************************************************
 * Copyright (C) 2007 the VideoLAN team
 * $Id: 3ef58fee45f31c058f30d38d44fbfd2d270fbab8 $
 *
 * Authors: Jean-Baptiste Kempf <jb (at) videolan.org>
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

#ifndef QVLC_TASK_DIALOG_H_
#define QVLC_TASK_DIALOG_H_ 1

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt4.hpp"

#include "util/qvlcframe.hpp"
#include "util/singleton.hpp"
#include "useroption.hpp"
#include <QSettings>
#include <QList>
#include <QMenu>
#include <QTimer>
#include <pthread.h>


class QEvent;
class QMessageBox;
class QProgressDialog;
class QSplitter;
class QListWidget;
class QStackedWidget;
class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QListWidgetItem;
class QTableWidget;
class QModelIndex;

class Task
{
public:
	Task()
	{
	}
	Task( const QString _fileName, int _process, const QString _state, int _uid )
		: fileName( _fileName ), process( _process ), state( _state ), uid( _uid )
	{
	}

	QString fileName;
	QString state;
	int process;
	int uid;
};

class TaskSelector : public QVLCFrame
{
    Q_OBJECT
public:
    TaskSelector( intf_thread_t * );
	QListWidget* getListWidget();

public slots:
    virtual void close() { toggleVisible(); }

private:
	QListWidget *listWidget;
    virtual ~TaskSelector();
};

class TaskDialog : public QVLCFrame, public Singleton<TaskDialog>
{
    Q_OBJECT
public:
    TaskDialog( intf_thread_t * );
    virtual ~TaskDialog();

	QTreeView* initDownloadTreeView();
	QTreeView* initUploadTreeView();

	QStandardItemModel *getDownloadModel(){ return downloadModel; }
	QStandardItemModel *getUploadModel(){ return uploadModel; }

	void addUploadItem(const QString file, int process = 0, const QString state = "Uploading", int uid = -1, const QString path = "", pthread_t thread_id = 0, int index = -1 );
	void addDownloadItem(const QString file, int process = 0, const QString state = "Downloading", int uid = -1, const QString url = "", pthread_t thread_id = 0, int index = -1 );

	QModelIndex getUploadItemIndex(const QString file);
	QModelIndex getDownloadItemIndex(const QString file);

	int getUploadItemProcess( const QString filename );
	int getDownloadItemProcess( const QString filename );

	/*初始化任务窗口任务项*/
	void initUploadItems(QSettings &settings, int uid);//读取登陆用户的上传文件信息
	void initDownloadItems(QSettings &settings, int uid);//读取登陆用户的下载文件信息

	/*更新task窗口项的进度信息*/
	void updateUploadItem( const QString file, int process = 0, const QString state = "Uploading");
	void updateDownloadItem( const QString file, int process = 0, const QString state = "Downloading");

	void updateUploadTasks();//更新所有上传任务的状态
	void updateDownloadTasks();//更新所有下载任务的状态

	/*information for QSettings*/
	QString getOrganization(){ return organization; };
	QString getApplication() { return application; };

	QString buildKeyString(const QString type, int uid, const QString file);
	QString buildValueString(const QString file, int process, const QString state, const QString url);

	/* 增加/删除QSettings配置信息*/
	void saveNewTask(const QString type, int uid, const QString file, int process, const QString state, const QString url/*用于上传时，此参数存储上传文件的路径*/ );
	void deleteTask(const QString type, int uid, const QString file);

	/*右键菜单*/
	void contextMenuEvent(QContextMenuEvent* e);

	/*设置/获取定时器运行状态*/
	bool isTimerRun(){ return b_timerRun == true; }
	void setTimerState(bool state){ b_timerRun = state; }

	void startUpdateTimer(int intval ) { updateTimer->start(intval*1000); setTimerState(true); }
	void tryStopUpdateTimer()
	{
		if(nDownloadTasks > 0 || nUploadTasks > 0)
			return;

		printf("Now Stop Timer\n");
		updateTimer->stop();
		setTimerState(false);
	}

	/*上传任务数增加/减少1个,并根据任务数量调整定时器状态*/
	void addUploadTask();
	void delUploadTask();

	/*下载任务数增加/减少1个,并根据任务数量调整定时器状态*/
	void addDownloadTask();
	void delDownloadTask();


public slots:
    friend class    Singleton<TaskDialog>;
	friend class UserOption;

	void stopItemTask();
	void continueItemTask();
	void deleteItemTask();

	/*双击停止或开始上传/下载任务*/
	void toggleUploadState(const QModelIndex&);
	void toggleDownloadState(const QModelIndex&);

	void updateTask();

protected:
//	void timerEvent(QTimerEvent *event);
//	int timerId;

signals:
	//void timeout();
private:
	/*Information for QSettings*/
	QString organization;
	QString application;

	QSplitter *leftSplitter;
	TaskSelector *selector;
	QStackedWidget *mainWidget;

	/* 下载 */
	QTreeView *downloadTree;
	QStandardItemModel *downloadModel;

	/*上传*/
	QTreeView *uploadTree;
	QStandardItemModel *uploadModel;

	/*QTimer*/
	QTimer *updateTimer;
	int nDownloadTasks;
	int nUploadTasks;
	bool b_timerRun;//定时器是否运行的flag

};

#endif
