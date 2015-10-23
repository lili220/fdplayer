/*****************************************************************************
 * useroption.cpp : User Options
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
#include "dialogs/user/useroption.hpp"
#include "dialogs/user/login.hpp"
#include "dialogs/user/task.hpp"
#include "util/qt_dirs.hpp"
#include "dialogs/user/ini.hpp"
//extern "C" {
#include "dialogs/user/wan_share.hpp"
//}

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

#include <vlc_network.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "recents.hpp"
#include <python2.7/Python.h>


#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#define URLTAIL "/haha/service/?wsdl"
#define HTTPURLTAIL "/haha/service/uploadfile"

UserOption::UserOption( intf_thread_t *_p_intf ) : p_intf( _p_intf )
{
	/*Initialize PyObjects */
	pArgs = NULL;
	pRetValue = NULL;

	pModule = NULL;
	pModule1 = NULL;
	regist = NULL;
	login = NULL;
	logout = NULL;
	keeponline = NULL;
	fileupload = NULL;
	listmyfile = NULL;
	filedelete = NULL;
	filedownload = NULL;

	keepalive = NULL;

	b_localShared = false;
	b_netShared = false;
	b_cloudModeStart = false;
	b_remoteModeStart = false;
	b_lanModeStart = true;

	/*default settings*/
	readWebServerConf();
#if 0
	setServerIp( "192.168.7.96" );
	setServerPort( 80 );
	setServerUrl( "192.168.7.96:8001" );
#endif

	//readSettings();//read localshare state and netshare state
	b_login = false;
	initialConf();
	//b_load = init();
	b_load = initialize();
        initflag = false;
}

UserOption::~UserOption()
{
	printf("-----------------------%s---------------------------\n", __func__ );
}
void UserOption::readWebServerConf()
{
	ini_t *conf = ini_load("./vlc.conf");
	if( conf == NULL )
	{
		printf("ini_load failed for readWebServerConf\n");
		return ;
	}

	char *server_ip = NULL;
	int server_port = -1;
	ini_read_str(conf, "public", "webserver_ip", &server_ip, NULL);
	ini_read_int(conf, "public", "webserver_port", &server_port, 0);
	ini_free(conf);
	setServerIp( server_ip );
	setServerPort( server_port );
}

void UserOption::initialConf()
{
	qDebug() << "-------------------"<< __func__ << "----------------------------";
	if( access("../sbin/minidlna.conf", F_OK ) >= 0 )
		setConfigPath( "../sbin/minidlna.conf" );
	else if( access("./minidlna-1.1.4/minidlna.conf", F_OK ) >= 0 )
		setConfigPath( "./minidlna-1.1.4/minidlna.conf" );
	else
		qDebug() << " minidlna.conf not found!";
	qDebug() << getConfigPath();

	setSharePath( getConfigPath() );
	qDebug() << getSharePath();
}

QString UserOption::getSharePath()
{
	qDebug() << "-------------------"<< __func__ << "----------------------------";

	QString cmd = "sed -n --silent '/^media_dir=/p' ";
	QString configFile = getConfigPath();
	cmd.append( configFile );
	cmd.append( "  | awk -F, '{print $2}'" );

	QString path;
	//qDebug() << "cmd:" << cmd;
	FILE * pathFile= popen( cmd.toStdString().c_str(), "r" );
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


#if 0
bool UserOption::init()
{
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return false;
	}

	PyRun_SimpleString( "import sys" );
	PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	pModule = PyImport_ImportModule( "registor" );
	if( pModule  == NULL )
	{
		printf( "Can't Import registor! \n" );
		return false;
	}

	regist = PyObject_GetAttrString(pModule,"nfschina_register");
	if(regist == NULL)
	{
		printf("regist is NULL\n");
		return false;
	}

	login = PyObject_GetAttrString(pModule,"nfschina_login");
	if(login == NULL)
	{
		printf("login is NULL\n");
		return false;
	}

	logout = PyObject_GetAttrString(pModule,"nfschina_logout");
	if(logout == NULL)
	{
		printf("logout is NULL\n");
		return false;
	}

	keeponline = PyObject_GetAttrString(pModule,"nfschina_keeponline");
	if(keeponline == NULL)
	{
		printf("keeponline is NULL\n");
		return false;
	}

	fileupload = PyObject_GetAttrString(pModule,"nfschina_upload");
	if(fileupload == NULL)
	{
		printf("fileupload is NULL\n");
		return false;
	}

	listmyfile = PyObject_GetAttrString(pModule,"nfschina_listmyfile");
	if(listmyfile == NULL)
	{
		printf("listmyfile is NULL\n");
		return false;
	}

	filedelete = PyObject_GetAttrString(pModule,"nfschina_delete");
	if(filedelete == NULL)
	{
		printf("delete is NULL\n");
		return false;
	}

	filedownload = PyObject_GetAttrString(pModule,"nfschina_download");
	if(filedownload ==NULL)
	{
		printf("download is NULL\n");
		return false;
	}

	keepalive = PyObject_GetAttrString(pModule, "SocketClient");
	if(keepalive == NULL)
	{
		printf( "keepalive is NULL\n" );
		return false;
	}

	return true;
}
#endif
bool UserOption::initialize()
{
	printf( "---------------%s---------------\n", __func__ );
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return false;
	}

	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
		return false;
	}

#if 0
	regist = PyObject_GetAttrString(pModule,"nfschina_register");
	if(regist == NULL)
	{
		printf("regist is NULL\n");
		return false;
	}
#if 0
	pModule1 = PyImport_ImportModule( "clientlg" );
	if( pModule1  == NULL )
	{
		printf( "Can't Import clientlg! \n" );
		return false;
	}
#endif
	login = PyObject_GetAttrString(pModule,"nfschina_login");
	if(login == NULL)
	{
		printf("login is NULL\n");
		return false;
	}

	listmyfile = PyObject_GetAttrString(pModule,"nfschina_listmyfile");
	if(listmyfile == NULL)
	{
		printf("listmyfile is NULL\n");
		return false;
	}

	fileupload = PyObject_GetAttrString(pModule,"nfschina_upload");
	if(fileupload == NULL)
	{
		printf("fileupload is NULL\n");
		return false;
	}

	filedelete = PyObject_GetAttrString(pModule,"nfschina_delete");
	if(filedelete == NULL)
	{
		printf("delete is NULL\n");
		return false;
	}
#endif

	return true;
}

QString UserOption::buildURL( QString ip, QString tail )
{
	QString url = "http://";
	url.append( ip );
	url.append( tail );

	return url;
}

int UserOption::nfschina_registor( QString username, QString password )
{
#if 1
/*
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return -1;
	}
*/

	initpython();
	PyGILState_STATE state = PyGILState_Ensure();
	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		PyGILState_Release( state );
                printf( "Can't Import clientrg! \n" );
		return -1;
	}

	PyObject *regist = PyObject_GetAttrString(pModule,"nfschina_register");
	if(regist == NULL)
	{
		printf("regist is NULL\n");
                PyGILState_Release( state );
		return -1;
	}
#endif
	//QString url = "http://192.168.7.97/haha/service/?wsdl";
	QString url = buildURL( getServerIp(), URLTAIL );
	printf( "before registor url: %s\n", url.toStdString().c_str() );

	PyObject *pArgs = PyTuple_New( 3 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "s", username.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", password.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "s", url.toStdString().c_str()) );
	PyObject *pRetValue = PyObject_CallObject( regist, pArgs );
	int uid = _PyInt_AsInt( pRetValue );
	printf("register :: uid = %d\n",uid);
	PyGILState_Release( state );
	return uid;
}

//int UserOption::nfschina_login( QString username, QString password, bool b_share, QString ip, QString port )
int UserOption::nfschina_login( QString username, QString password )
{
	printf("nfschina_login this->threadid=[%lu]\n",pthread_self());
	printf("//service.nfschina_login\n");

	printf("username=%s :: password = %s\n",username.toStdString().c_str(), password.toStdString().c_str() );
#if 1
        /*
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return -1;
	}
        */
	initpython();
        PyGILState_STATE state = PyGILState_Ensure();
	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
                PyGILState_Release( state );
		return -1;
	}

	PyObject *login = PyObject_GetAttrString(pModule,"nfschina_login");
	if(login == NULL)
	{
		printf("login is NULL\n");
                PyGILState_Release( state );
		return -1;
	}
#endif

	QString url = buildURL( getServerIp(), URLTAIL );
	printf( "before login url: %s\n", url.toStdString().c_str() );

	pArgs = PyTuple_New( 3 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "s", username.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", password.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "s", url.toStdString().c_str() ) );
	//PyTuple_SetItem( pArgs, 3, Py_BuildValue( "s", ip.toStdString().c_str()) );
	//PyTuple_SetItem( pArgs, 4, Py_BuildValue( "s", port.toStdString().c_str()) );

	pRetValue = PyObject_CallObject( login, pArgs );
	int uid = _PyInt_AsInt(pRetValue);
	printf("login :: luid = %d\n",uid);
	b_login = ( uid > 0 ? true : false );
	PyGILState_Release( state );
	return uid;
}

int UserOption::nfschina_logout( int userid )
{
	printf("//service.nfschina_logout\n");

	if( !isLogin() )
	{
		printf( "No user is Login current!\n" );
		return 0;
	}
	printf("userid=%d \n", userid );
	pArgs = PyTuple_New( 1 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );

	pRetValue = PyObject_CallObject( logout, pArgs );
	int ret = _PyInt_AsInt(pRetValue);
	printf("logout :: ret = %d\n",ret );

	return ret;//????
}

int UserOption::nfschina_keeponline( int userid, bool b_share )
{
	printf( "------------------------%s-------------------------\n", __func__ );
	printf( "userid = %d, b_share = %d\n", userid, b_share );
	if( !isLogin() )
	{
		printf( "Plase Login !\n" );
		return -1;// ?????
	}

	pArgs = PyTuple_New( 2 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue("i", luid) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue("i", b_share) );

	pRetValue = PyObject_CallObject( keeponline, pArgs );
	int err = _PyInt_AsInt(pRetValue);

	printf("1:success -1:fail err = %d\n",err);

	return err;
}

static void *thread_upload( void *data )
{
	printf( "-----------%s:%d--------\n", __func__, __LINE__ );

	int ret;
	if( (ret = pthread_detach(pthread_self())) != 0 )
	{
		fprintf( stderr, "pthread_detach failed for thread_upload:%s\n", strerror(ret) );
		return (void*)-1;
	}

	ThreadArg *arg = (ThreadArg*)data;
/*
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return (void*)-1;
	}
*/
	PyGILState_STATE state = PyGILState_Ensure();

	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
                PyGILState_Release( state );
		return (void*)-1;
	}

	PyObject *fileupload = PyObject_GetAttrString(pModule,"nfschina_upload");
	if(fileupload == NULL)
	{
		printf("fileupload is NULL\n");
                PyGILState_Release( state );
		return (void*)-1;
	}

	PyObject *pArgs = PyTuple_New( 5 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", arg->uid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", arg->file.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "s", arg->path.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 3, Py_BuildValue( "s", arg->url.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 4, Py_BuildValue( "s", arg->httpurl.toStdString().c_str() ) );

#if 0
	TaskDialog *task = TaskDialog::getInstance(arg->p_intf);
	task->addUploadItem(arg->file, 0, "Uploading");
#endif
	PyObject *pRetValue = PyObject_CallObject( fileupload, pArgs );
	int err =0;
	err = _PyInt_AsInt( pRetValue );
	PyGILState_Release( state );

#if 0
	Py_DECREF(pModule);
	Py_DECREF(fileupload);
	Py_DECREF(pArgs);
	Py_DECREF(pRetValue);
	Py_Finalize();
#endif

	printf( "upload retvalue: %d \n", err );
	
	return (void*)err;
}

//int UserOption::nfschina_upLoad( int userid, QString filename, QString filepath )
int UserOption::nfschina_upLoad( int userid, const char* filename, const char* filepath )
{
	printf( "-----------%s:%d--------\n", __func__, __LINE__ );
	printf("this->threadid=[%lu]\n",pthread_self());
	if( !isLogin() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "文件上传提示框" ),
				qtr( "您还未登陆，不能上传共享文件！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();

		return -1;// ?????
	}

#if 1
	QString url = buildURL( getServerIp(), URLTAIL );
	printf( "before nfschina_upLoad url: %s\n", url.toStdString().c_str() );

	QString httpurl = buildURL( getServerIp(), HTTPURLTAIL );
	printf( "before nfschina_upLoad httpurl: %s\n", httpurl.toStdString().c_str() );
        initpython();
	//ThreadArg *arg = new ThreadArg( userid, filename, filepath, url, httpurl );
	ThreadArg *arg = new ThreadArg( userid, filename, filepath, url, httpurl, p_intf );
	int ret = 0;
	pthread_t upthread_id;
	if( (ret = pthread_create( &upthread_id, NULL, thread_upload, (void*)arg)) != 0 )
	{
		fprintf( stderr, "pthread_create for upload:%s\n", strerror(ret) );
		return -1;
	}

#if 1
	TaskDialog *task = TaskDialog::getInstance(p_intf);
	task->saveNewTask("upload", userid, filename, 0, qtr("上传中..."), filepath);
	task->addUploadItem(filename, 0, qtr("上传中..."), userid, filepath, upthread_id);
#endif

//	pthread_join(upthread_id, (void**)&ret);
	printf("thread_upload return value: ret = %d\n", ret);

	return ret;
#endif
#if 0
	pArgs = PyTuple_New( 3 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );
	//PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", filename.toStdString().c_str() ) );
	//PyTuple_SetItem( pArgs, 2, Py_BuildValue( "s", filepath.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", filename ) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "s", filepath ) );

	pRetValue = PyObject_CallObject( fileupload, pArgs );
	int err =0;
	err = _PyInt_AsInt( pRetValue );

	printf( "upload retvalue: %d \n", err );
	
	return err;
#endif
}

void UserOption::nfschina_listMyFile( int userid )
{
	if( !isLogin() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "获取共享文件" ),
				qtr( "您还未登陆，不能获取共享文件列表！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();

		return ;
	}

	QString url = buildURL( getServerIp(), URLTAIL );
	printf( "before nfschina_listMyFile url: %s\n", url.toStdString().c_str() );

	PyObject *pArgs = PyTuple_New ( 2 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", url.toStdString().c_str() ) );

	PyObject *pRetValue = PyObject_CallObject( listmyfile, pArgs );
	int s = PyList_Size( pRetValue );

	fileList.clear(); int i = 0;
	for( i = 0; i < s; i++ )
	{
		fileList << PyString_AsString( PyList_GetItem( pRetValue, i ) );
		printf( "file[%d]:%s\n", i,  PyString_AsString( PyList_GetItem( pRetValue, i ) ));
	}
	printf( "s = %d, fileList count = %d\n", s, fileList.count() );

	return ;
}

#if 1
void *thread_getfile( void* data )
{
	printf( "--------%s:%d------------\n", __func__, __LINE__ );
	ThreadListArg *arg = ( ThreadListArg*)data;
	int userid = arg->uid;
	QString url = arg->url;
	//arg->filelist->push_back("aaaaaaa");
/*
	if( !Py_IsInitialized() )
	{
		Py_Initialize();
		printf( "Python initialize failed! \n" );
		//return (void*)false;
	}
*/
        PyGILState_STATE state = PyGILState_Ensure( );
	printf( "--------%s:%d------------\n", __func__, __LINE__ );
	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );
	printf( "--------%s:%d------------\n", __func__, __LINE__ );

	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
                PyGILState_Release( state );
		return (void*)false;
	}
	PyObject *listmyfile = PyObject_GetAttrString(pModule,"nfschina_listmyfile");
	if(listmyfile == NULL)
	{
		printf("listmyfile is NULL\n");
                PyGILState_Release( state );
		return (void*)false;
	}

	PyObject *pArgs = PyTuple_New ( 2 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", url.toStdString().c_str() ) );

	PyObject *pRetValue = PyObject_CallObject( listmyfile, pArgs );
	if( pRetValue == NULL )
	{
		printf( "pRetValue for nfschina_GetFileList is NULL\n" );
                PyGILState_Release( state );
		return (void*)false;
	}

	int s = PyList_Size( pRetValue );

	//filelist.clear(); int i = 0;
	QList<QString> *plist = arg->filelist;
	for( int i = 0; i < s; i++ )
	{
		printf( "file[%d]:%s\n", i,  PyString_AsString( PyList_GetItem( pRetValue, i ) ));
		//filelist << PyString_AsString( PyList_GetItem( pRetValue, i ) );
		(*plist) << PyString_AsString( PyList_GetItem( pRetValue, i ) );
	}
	printf( "s = %d, fileList count = %d\n", s, plist->count() );

	Py_DECREF(pModule);
	Py_DECREF(listmyfile);
	Py_DECREF(pArgs);
	Py_DECREF(pRetValue);
	PyGILState_Release( state );

	//Py_Finalize();
	//arg->filelist->push_back("aaaaaaa");
	return (void*)true;
}
#endif
QList<QString> UserOption::nfschina_GetFileList( int userid )
{
	printf( "--------%s:%d------------\n", __func__, __LINE__ );
	QList<QString> filelist;
	filelist.clear();
	if( !isLogin() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "获取共享文件" ),
				qtr( "您还未登陆，不能获取共享文件列表！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();

		return filelist;
	}

#if 1
	int ret;
	pthread_t listthread_id;
	bool retval;
	QList<QString> *plist = new QList<QString>;
	QString url = buildURL( getServerIp(), URLTAIL );
	printf( "before nfschina_GetFileList url: %s\n", url.toStdString().c_str() );
	ThreadListArg *arg = new ThreadListArg( userid,  plist, url );
	if( (ret = pthread_create(&listthread_id, NULL, thread_getfile, (void*)arg)) != 0 )
	{
		fprintf( stderr, "pthread_create for getfilelist failed:%s\n", strerror(ret) );
		return filelist;
	}

	if( (ret = pthread_join( listthread_id, (void**)&retval)) != 0 )
	{
		fprintf( stderr, "pthread_join failed for getfilelist:%s\n", strerror(ret) );
		return filelist;
	}

	foreach( const QString& file, *plist )
	{
		printf( "get filelist:%s\n", file.toStdString().c_str() );
		filelist.append( file );
	}

	delete plist;
	return filelist;

#endif

#if 0
#if 1//此处和远端模块由冲突，所以重新初始化python模块，能够正常
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return filelist;
	}

	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
		return filelist;
	}
	PyObject *listmyfile = PyObject_GetAttrString(pModule,"nfschina_listmyfile");
	if(listmyfile == NULL)
	{
		printf("listmyfile is NULL\n");
		return filelist;
	}
#endif

	PyObject *pArgs = PyTuple_New ( 1 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );

	PyObject *pRetValue = PyObject_CallObject( listmyfile, pArgs );
	if( pRetValue == NULL )
	{
		printf( "pRetValue for nfschina_GetFileList is NULL\n" );
		return filelist;
	}

	int s = PyList_Size( pRetValue );

	//filelist.clear(); int i = 0;
	for( int i = 0; i < s; i++ )
	{
		printf( "file[%d]:%s\n", i,  PyString_AsString( PyList_GetItem( pRetValue, i ) ));
		filelist << PyString_AsString( PyList_GetItem( pRetValue, i ) );
	}
	printf( "s = %d, fileList count = %d\n", s, filelist.count() );

	Py_DECREF(pModule);
	Py_DECREF(listmyfile);
	Py_DECREF(pArgs);
	Py_DECREF(pRetValue);
	Py_Finalize();
	return filelist;
#endif
}

static void* thread_delete( void *data )
{
	int ret;
	if( (ret = pthread_detach(pthread_self())) != 0 )
	{
		printf( "pthread_detach failed for thread_delete:%s\n", strerror( ret ) );
		return (void*)-1;
	}
	ThreadArg *arg = (ThreadArg*)data;
#if 0
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return (void*)-1;
	}
#endif
	// grab the global interpreter lock
	//PyEval_AcquireLock();
        PyGILState_STATE state = PyGILState_Ensure();
	printf( "----------%s:%s-----------\n", __FILE__, __func__ );

	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
		return (void*)-1;
	}

	PyObject *filedelete = PyObject_GetAttrString(pModule,"nfschina_delete");
	if(filedelete == NULL)
	{
		printf("delete is NULL\n");
		return (void*)-1;
	}

	printf( "--------%s:file:%s:url:%s-----------\n", __func__, arg->file.toStdString().c_str(),  arg->url.toStdString().c_str() );
	PyObject *pArgs = PyTuple_New( 3 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", arg->uid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", arg->file.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "s", arg->url.toStdString().c_str() ) );

	PyObject *pRetValue = PyObject_CallObject( filedelete, pArgs );
	int err = _PyInt_AsInt( pRetValue );
	printf( "filedelete retvalue: %d\n", err );

        // release the lock
	//PyEval_ReleaseLock();
         PyGILState_Release( state );

	return (void*)err;
}

void UserOption::initpython()
{
	if (! initflag)  {
	// initialize Python
	Py_Initialize();
	// initialize thread support
	PyEval_InitThreads();
        //PyEval_ReleaseLock();
        PyEval_ReleaseThread(PyThreadState_Get());
            initflag=true;
        }
}

int UserOption::nfschina_delete( int userid, QString filename )
{
	if( !isLogin() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "删除提示框" ),
				qtr( "您还未登陆，不能进行删除操作！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();

		return -1;// ?????
	}
#if 1

	pthread_t delthread_id;
	int ret;
	QString url = buildURL( getServerIp(), URLTAIL );
	printf( "before thread_delete url: %s\n", url.toStdString().c_str() );

	ThreadArg *arg = new ThreadArg( userid, filename, NULL, url );
	if( (ret = pthread_create(&delthread_id, NULL, thread_delete, (void*)arg)) != 0 )
	{
		fprintf( stderr, "pthread_create for nfschina_delete:%s\n", strerror(ret) );
		return -1;
	}
	return 0;
#endif

#if 0
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return -1;
	}

	PyRun_SimpleString( "import sys" );
	PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
		return -1;
	}

	PyObject *filedelete = PyObject_GetAttrString(pModule,"nfschina_delete");
	if(filedelete == NULL)
	{
		printf("delete is NULL\n");
		return false;
	}

	PyObject *pArgs = PyTuple_New( 2 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", filename.toStdString().c_str() ) );

	PyObject *pRetValue = PyObject_CallObject( filedelete, pArgs );
	int err = _PyInt_AsInt( pRetValue );
	printf( "filedelete retvalue: %d\n", err );

	return err;
#endif

}

QString UserOption::nfschina_download( int userid, QString filename )
{
	if( !isLogin() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "下载提示框" ),
				qtr( "您还未登陆，不能进行下载操作！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();

		return NULL;// ?????
	}
/*
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return NULL;
	}
*/
	initpython();
	PyGILState_STATE state  = PyGILState_Ensure( );

	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
		PyGILState_Release( state );
		return NULL;
	}

	PyObject *filedownload = PyObject_GetAttrString(pModule,"nfschina_download");
	if(filedownload ==NULL)
	{
		printf("download is NULL\n");
		PyGILState_Release( state );
		return NULL;
	}


	QString url = buildURL( getServerIp(), URLTAIL );
	printf( "before nfschina_download url: %s\n", url.toStdString().c_str() );
	PyObject *pArgs = PyTuple_New( 3 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", filename.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "s", url.toStdString().c_str() ) );

	PyObject *pRetValue = PyObject_CallObject( filedownload, pArgs );
	int s = PyList_Size( pRetValue );
	printf( "s = %d\n", s );
	if( s <= 0 )
	{
		PyGILState_Release( state );
		return NULL;
	}

	//int i;
	//for( i = 0; i < s; i++ )
	//printf( "get url:%s\n", PyString_AsString( pRetValue ) );
	PyGILState_Release( state );
	return PyString_AsString( PyList_GetItem( pRetValue, 0) );
}

static void* thread_dwncloud( void *data )
{
	int ret;
	if( (ret = pthread_detach( pthread_self())) != 0 )
	{
		fprintf( stderr, "pthread_detach failed for thread_dwncloud:%s\n", strerror(ret) );
		return (void*)-1;
	}
	ThreadArg *arg = (ThreadArg*)data;
/*
	if( !Py_IsInitialized() )
	{
		Py_Initialize();
		printf( "Python initialize failed! \n" );
		//return (void*)-1;
	}
*/
	PyGILState_STATE state  = PyGILState_Ensure( );
	PyRun_SimpleString( "import sys" );
	//PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	PyRun_SimpleString( "sys.path.append('../share/python')" );
	PyRun_SimpleString( "sys.path.append('.')" );


	PyObject *pName = PyString_FromString( "download" );
	PyObject *pModule = PyImport_Import( pName );
	if( !pModule )
	{
		printf( "can't find download.py\n" );
		PyGILState_Release( state );
		return (void*)-1;
	}

	PyObject *pDict = PyModule_GetDict( pModule );
	if( !pDict )
	{
		printf( "can't get dict from download.py\n" );
		PyGILState_Release( state );
		return (void*)-1;
	}

	PyObject *pFunc = PyDict_GetItemString( pDict, "paxel" );
	if( !pFunc || !PyCallable_Check( pFunc ) )
	{
		printf( "can't find function [paxel]" );
		PyGILState_Release( state );
		return (void*)-1;
	}

	PyObject *pArgs = PyTuple_New( 3 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "s", arg->path.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", arg->file.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "i", 4 ) );

	PyObject_CallObject( pFunc, pArgs );

	Py_DECREF( pName );
	Py_DECREF( pArgs );
	Py_DECREF( pModule );

	//Py_Finalize();
	PyGILState_Release( state );

	return (void*)0;
}
void UserOption::downloadCloudShareFile( const QString url, const QString file )
{
	printf( "----------------------%s-----------------------\n", __func__ );
	pthread_t dwncloud_thread_id;
	ThreadArg *arg = new ThreadArg( file, url );//user the path memeber of TreadArg as url

	int ret;
	initpython();
	if( (ret = pthread_create( &dwncloud_thread_id, NULL, thread_dwncloud, (void*)arg)) != 0 )
	{
		fprintf( stderr, "pthread_create failed for download cloudshare file:%s\n", strerror( ret ));
		return ;
	}

#if 1
	int userid = getLUid();
	TaskDialog *task = TaskDialog::getInstance(p_intf);
	task->saveNewTask("download", userid, file, 0, qtr("下载中..."), url);
	task->addDownloadItem(file, 0, qtr("下载中..."), userid, url, dwncloud_thread_id);
#endif
}

void UserOption::toggleLocalShared( bool state )
{
	printf( "----------------------%s-----------------------\n", __func__ );
	if( b_localShared == state )
		return;
	b_localShared = state;

	printf("state = %d\n", state );
	printf("b_localShared = %d\n", b_localShared );
	QString cmd;

	if( b_localShared )//Open LocalShare
	{
		if( access("../sbin/dlna_tools", F_OK ) >= 0 )
		{
			cmd = "cd ../sbin/; bash dlna_tools -s start" ;
		}
		else if( access( "./minidlna-1.1.4/dlna_tools", F_OK ) >= 0 )
		{
			cmd = " cd ./minidlna-1.1.4/; bash dlna_tools -s start";
		}
		else
		{
			qDebug() << " dlna_tools not found!";
			return ;
		}
		qDebug() << cmd;
		system( cmd.toStdString().c_str() );
	}
	else//close LocalShare
	{
		if( access("../sbin/dlna_tools", F_OK ) >= 0 )
		{
			cmd = "cd ../sbin/;bash dlna_tools -s stop" ;
		}
		else if( access( "./minidlna-1.1.4/dlna_tools", F_OK ) >= 0 )
		{
			cmd = " cd ./minidlna-1.1.4/;bash dlna_tools -s stop";
		}
		else
		{
			qDebug() << " dlna_tools not found!";
			return ;
		}
		qDebug() << cmd;
		system( cmd.toStdString().c_str() );
		//system( "kill -9 `ps -ef | grep minidlna | grep -v grep | awk '{print $2}'`" );
	}

	setLanSharedStart(false);
}

void UserOption::toggleNetShared( bool state )
{
	printf( "----------------------%s-----------------------\n", __func__ );
	if( b_netShared == state )
		return;

	printf("state = %d\n", state );
	printf("b_netShared = %d\n", b_netShared );
	if( !isLogin() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "远端共享提示" ),
				qtr( "用户尚未登录, 无法开启远端共享！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();
	}
	else
	{
		if (state == 1)
		{
			open_wan_share(getLUid());
			qDebug() << "open wan share!";
		} else {
			close_wan_share();
			qDebug() << "close wan share!";
		}
		b_netShared = state;
	}
	//	printf("line:%d\n", __LINE__);
	//	nfschina_keepalive("0.0.0.0", 8000, getLUid(), state, getServerUrl() );
	//	nfschina_keeponline( getLUid(), getNetShared() );
	printf("line:%d\n", __LINE__);
	emit netShareState(b_netShared);
}

void UserOption::nfschina_keepalive(QString localIp, int localPort, int userid, bool is_share, QString ServerUrl)
{
	printf( "----------------------%s-----------------------\n", __func__ );
#if 0
	ThreadKeepAlive *keepalive = new ThreadKeepAlive( p_intf, localIp, localPort, userid, is_share );
	keepalive->run();
#endif
	//printf( "userid = %d, is_share = %d, serverUrl = %s\n", userid, is_share, ServerUrl.toStdString().c_str() );
	pArgs = PyTuple_New( 5 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "s", localIp.toStdString().c_str() ) );
	//PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", localPort.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "i", localPort ) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "i", userid ) );
	PyTuple_SetItem( pArgs, 3, Py_BuildValue( "i", is_share ) );
	PyTuple_SetItem( pArgs, 4, Py_BuildValue( "s", ServerUrl.toStdString().c_str() ) );

	pRetValue = PyObject_CallObject( keepalive, pArgs );
}
