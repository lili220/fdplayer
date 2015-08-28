/*****************************************************************************
 * useroption.hpp : User option class
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

#ifndef QVLC_USEROPTION_DIALOG_H_
#define QVLC_USEROPTION_DIALOG_H_ 1

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt4.hpp"

#include "util/qvlcframe.hpp"
#include "util/singleton.hpp"
#include <python2.7/Python.h>

class QEvent;
class QPushButton;
class QCheckBox;
class QMessageBox;
class QLineEdit;

#if 0
class ThreadKeepAlive : public QThread
{
	Q_OBJECT
public:
		ThreadKeepAlive( intf_thread_t _p_intf, QString ip, int port, int uid, bool share );
		void stop();
protected:
		void run();
private:
		intf_thread_t *p_intf;
		volatile bool stopped;
		QString localIp;
		int localPort;

		int userid;
		bool b_share;

		PyObject *pModule;
		PyObject *pArgs;
		PyObject *pRetValue;
		PyObject *keepalive;
};
#endif

class UserOption : public QObject, /*public QVLCFrame,*/ public Singleton<UserOption>
{
    Q_OBJECT
public:
    UserOption( intf_thread_t * _p_intf );
    virtual ~UserOption();
	bool init();
	bool isLoaded(){ return b_load == true; }
	bool isLogin(){ return b_login == true; }
	void setLogin( bool state ){ b_login = state; }
	int nfschina_registor( QString username, QString password );
	//int nfschina_login( QString username, QString password, bool b_share, QString ip, QString port );
	int nfschina_login( QString username, QString password );
	int nfschina_logout( int userid );
	int nfschina_keeponline( int userid, bool b_share );//droped
	void nfschina_keepalive(QString localIp, int localPort, int userid, bool is_share, QString ServerUrl);
	int nfschina_upLoad( int userid, QString filename, QString filepath );
	void nfschina_listMyFile( int userid );
	int nfschina_delete( int userid, QString filename );
	int nfschina_download( int userid, QString filename );

	QList<QString> getFileList(){ return fileList; }
	int getRUid(){ return ruid; }
	int getLUid() {return luid; }
	void setRUid( int uid ) { ruid = uid; }
	void setLUid( int uid ) { luid = uid; }

	void setServerIp( QString ip ){ serverIp = ip; }
	void setServerPort( int port ){ serverPort = port; }
	QString getServerIp(){ return serverIp; }
	int getServerPort(){ return serverPort; }
	void setServerUrl( QString url ){ serverUrl = url; }
	QString getServerUrl(){ return serverUrl; }
	
	void setLocalShared( bool state ){ b_localShared = state; }
	void setNetShared( bool state ){ b_netShared = state; }
	bool getLocalShared(){ return b_localShared == true; }
	bool getNetShared(){ return b_netShared == true; }
#if 1
public slots:
	void toggleLocalShared( bool state = false );
	void toggleNetShared( bool state = false );
#endif

protected:
    friend class    Singleton<UserOption>;
    intf_thread_t *p_intf;

private:
	bool b_load;//python module is Loaded successfull
	bool b_login;//user is login
	QList<QString> fileList;

	int ruid;
	int luid;

	bool b_localShared;
	bool b_netShared;

	QString serverIp;
	int  serverPort;
	QString serverUrl;

	PyObject *pArgs;
	PyObject *pRetValue;

	PyObject *pModule;
	PyObject *regist;
	PyObject *login;
	PyObject *logout;
	PyObject *keeponline;//not used
	PyObject *keepalive;
	PyObject *fileupload;
	PyObject *listmyfile;
	PyObject *filedelete;
	PyObject *filedownload;

};

#endif
