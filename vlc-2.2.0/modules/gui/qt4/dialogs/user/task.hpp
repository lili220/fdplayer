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

	void addUploadItem(const QString file, int process = 0, const QString state = "Uploading");
	void addDownloadItem(const QString file, int process = 0, const QString state = "Downloading");

	QModelIndex getUploadItemIndex(const QString file);
	QModelIndex getDownloadItemIndex(const QString file);

	void updateUploadItem( const QString file, int process = 0, const QString state = "Uploading");
	void updateDownloadItem( const QString file, int process = 0, const QString state = "Downloading");

public slots:
    friend class    Singleton<TaskDialog>;
	friend class UserOption;

private:
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
