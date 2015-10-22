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

	void addUploadItem(const QString file, int process = 0, const QString state = "Uploading", int uid = -1, const QString path = "", unsigned int thread_id = 0 );
	void addDownloadItem(const QString file, int process = 0, const QString state = "Downloading", int uid = -1, const QString url = "", unsigned int thread_id = 0 );

	QModelIndex getUploadItemIndex(const QString file);
	QModelIndex getDownloadItemIndex(const QString file);

	/*初始化任务窗口任务项*/
	void initUploadItems(QSettings &settings, int uid);//读取登陆用户的上传文件信息
	void initDownloadItems(QSettings &settings, int uid);//读取登陆用户的下载文件信息

	/*更新task窗口项的进度信息*/
	void updateUploadItem( const QString file, int process = 0, const QString state = "Uploading");
	void updateDownloadItem( const QString file, int process = 0, const QString state = "Downloading");

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

public slots:
    friend class    Singleton<TaskDialog>;
	friend class UserOption;

	void stopTask();
	void continueTask();
	void deleteTask();

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
};

#endif
