/*****************************************************************************
 * usershare.hpp : User Share dialogs
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

#ifndef QVLC_USERSHARE_DIALOG_H_
#define QVLC_USERSHARE_DIALOG_H_ 1

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt4.hpp"

#include "util/qvlcframe.hpp"
#include "util/singleton.hpp"

/* include Open header*/
#include "recents.hpp"

class QEvent;
class QListWidget;
class QSplitter;
class QStackedWidget;
class QTreeView;
class QLabel;
class QPushButton;
class QModelIndex;
class QStandardItemModel;
class QDir;
class QStandardItem;
class QFileInfo;
class UserOption;
class QListWidgetItem;

class UploadArgs
{
	public:
		UploadArgs( int uid, QString& _file, QString& _filepath );
	int uid;
	QString file;
	QString filePath;
};

class UserShareSelector : public QVLCFrame
{
    Q_OBJECT
public:
    UserShareSelector( intf_thread_t * );
	QListWidget* getListWidget();

public slots:
    virtual void close() { toggleVisible(); }

private:
	QListWidget *listWidget;
    virtual ~UserShareSelector();
};

class UserShareDialog : public QVLCFrame, public Singleton<UserShareDialog>
{
	Q_OBJECT

	public:
		UserShareDialog( intf_thread_t * );
		void contextMenuEvent(QContextMenuEvent* e);
		QTreeView* initLocalShareTreeView();
		QTreeView* initServerShareTreeView();
		QStandardItem* addRow(QStandardItemModel* model, const QFileInfo& file );
		QStandardItem* addRow(QStandardItemModel* model, const QString& file );
		QStandardItem* addRow(QStandardItem* model, const QFileInfo& file );
		void addDirEntries(QDir* dir , QStandardItem* itemModel );

		void setConfigPath( QString path = "../sbin/minidlna.conf"){ configPath = path; }
		QString getConfigPath(){ return configPath; }
		void setSharePath( QString path = "~/share" ){ sharePath = path; }
		QString getSharePath();

		public slots:
			friend class Singleton<UserShareDialog>;

		virtual void close() { toggleVisible(); }

		void updateLocalShareDialog();
		void updateServerShareDialog();
		//void showUserDialog();

		void addLocalShareFile();
		void delLocalShareFile();
		void upLoadShareFile();
		void downLoadShareFile();
		void deleteRemoteShareFile();
		QString getFilePath( QString basedir );
		void removeFile( QString file );

	void playShareFile(const QModelIndex& );
	void playShareFile();
	void pausePlaying();
	//void stopPlaying();

signals:
	void localShareFileChanged();
	void serverShareFileChanged();

private:
		virtual ~UserShareDialog();

		pthread_t threadUpload;
		//UserOption *user;
		//QLabel *title;
		QSplitter *leftSplitter;
		QSplitter *split;
		UserShareSelector *selector;
		QStackedWidget *mainWidget;

		/*Local Share Widget */
		QTreeView *localShareTree;
		QStandardItemModel *localShareModel;

		/*serverSharetree*/
		QTreeView *serverShareTree;
		QStandardItemModel *serverShareModel;

		bool isPause;
		bool isPlaying;
		/*file share path*/
		QString configPath;//path for minidlna.conf
		QString sharePath;//midia_dir's value in minidlna.conf file
};
#endif
