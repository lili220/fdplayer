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

/*多线程上传入口函数参数结构体*/
class ThreadArg
{
	public:
		ThreadArg(  int _uid, const QString& _file, const QString& _path = NULL, const QString& _url = NULL, const QString& _httpurl = NULL, intf_thread_t* _p_intf = NULL ):
			uid( _uid ), file( _file ), path( _path ), url( _url ), httpurl( _httpurl ), p_intf( _p_intf )
	{
	}
		ThreadArg(  const QString& _file, const QString& _path = NULL, const QString& _url = NULL, const QString& _httpurl = NULL, intf_thread_t* _p_intf = NULL ):
			file( _file ), path( _path ), url(_url ), httpurl(_httpurl), p_intf( _p_intf )
	{
	//	uid = -1;
	}

	int uid;
	QString file;
	QString path;
	QString url;
	QString httpurl;
	intf_thread_t* p_intf;
};

/*用于获取文件列表*/
class ThreadListArg
{
	public:
		/*注意，此处为浅拷贝*/
		ThreadListArg( int _uid, QList<QString>* _filelist, QString _url ):
			uid( _uid ), filelist( _filelist ), url( _url )
	{
	}
		int uid;
		QList<QString> *filelist;
		QString url;
};

class UserOption : public QObject, /*public QVLCFrame,*/ public Singleton<UserOption>
{
    Q_OBJECT
public:
    UserOption( intf_thread_t * _p_intf );
    virtual ~UserOption();
	//bool init();
	bool initialize();//new
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
	//int nfschina_upLoad( int userid, const char* filename, const char* filepath );
	void nfschina_listMyFile( int userid );
	QList<QString> nfschina_GetFileList( int userid );
	int nfschina_delete( int userid, QString filename );
	QString nfschina_download( int userid, QString filename );//return the URL of the selected cloudshare file

	void downloadCloudShareFile( const QString url, const QString filename );
	int getProcess(int index);
    int getUploadProcess(int index);

	/*停止下载任务*/
	void stopDownload(int index);
    void stopUpload(int index);

	QList<QString> getFileList(){ return fileList; }
	int getRUid(){ return ruid; }
	int getLUid() {return luid; }
	void setRUid( int uid ) { ruid = uid; }
	void setLUid( int uid ) { luid = uid; }

	void readWebServerConf();
	void setServerIp( QString ip ){ serverIp = ip; }
	void setServerPort( int port ){ serverPort = port; }
	QString getServerIp(){ return serverIp; }
	int getServerPort(){ return serverPort; }
	void setServerUrl( QString url ){ serverUrl = url; }
	QString getServerUrl(){ return serverUrl; }
	QString buildURL(QString ip, QString tail );
	
	void setLocalShared( bool state ){ b_localShared = state; }
	void setNetShared( bool state ){ b_netShared = state; }
	bool getLocalShared(){ return b_localShared == true; }
	bool getNetShared(){ return b_netShared == true; }

	void setRemoteSharedStart( bool state ){ b_remoteModeStart = state; }
	void setCloudSharedStart( bool state ){ b_cloudModeStart = state; }
	void setLanSharedStart( bool state ){ b_lanModeStart = state; }
	bool getRemoteSharedStart(){ return b_remoteModeStart == true; }
	bool getCloudSharedStart(){ return b_cloudModeStart == true; }
	bool getLanSharedStart(){ return b_lanModeStart == true; }

	
	void initialConf();
	void setConfigPath( QString path = "../sbin/minidlna.conf"){ configPath = path; }
	QString getConfigPath(){ return configPath; }
	void setSharePath( QString path = "~/share" ){ sharePath = path; }
	QString getSharePath();

signals:
	void netShareState(bool);

#if 1
public slots:
	void toggleLocalShared( bool state = false );
	void toggleNetShared( bool state = false );
	void initpython();
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
	bool b_cloudModeStart;
	bool b_remoteModeStart;
	bool b_lanModeStart;
	bool initflag;

	QString serverIp;
	int  serverPort;
	QString serverUrl;

	QString configPath;
	QString sharePath;

	PyObject *pArgs;
	PyObject *pRetValue;

	PyObject *pModule;
	PyObject *pModule1;
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
