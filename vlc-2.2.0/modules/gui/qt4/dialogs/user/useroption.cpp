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

#include <vlc_network.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "recents.hpp"
#include <python2.7/Python.h>


#include <assert.h>

UserOption::UserOption( intf_thread_t *_p_intf ) : p_intf( _p_intf )
{
	pArgs = NULL;
	pRetValue = NULL;

	pModule = NULL;
	regist = NULL;
	login = NULL;
	keeponline = NULL;
	fileupload = NULL;
	listmyfile = NULL;
	filedelete = NULL;
	filedownload = NULL;

	b_localShared = false;
	b_netShared = false;

	setServerIp( "192.168.7.97" );
	setServerPort( "80" );

	//readSettings();//read localshare state and netshare state
	init();
}

UserOption::~UserOption()
{
}

void UserOption::init()
{
	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return;
	}

	PyRun_SimpleString( "import sys" );
	PyRun_SimpleString( "sys.path.append('./modules/gui/qt4/dialogs/user')" );
	PyRun_SimpleString( "sys.path.append('.')" );

	pModule = PyImport_ImportModule( "registor" );
	if( pModule  == NULL )
	{
		printf( "Can't Import registor! \n" );
		return ;
	}

	regist = PyObject_GetAttrString(pModule,"nfschina_register");
	if(regist == NULL)
	{
		printf("regist is NULL\n");
		return ;
	}

	login = PyObject_GetAttrString(pModule,"nfschina_login");
	if(login == NULL)
	{
		printf("login is NULL\n");
		return ;
	}

	keeponline = PyObject_GetAttrString(pModule,"nfschina_keeponline");
	if(keeponline == NULL)
	{
		printf("keeponline is NULL\n");
		return ;
	}

	fileupload = PyObject_GetAttrString(pModule,"nfschina_upload");
	if(fileupload == NULL)
	{
		printf("fileupload is NULL\n");
		return ;
	}

	listmyfile = PyObject_GetAttrString(pModule,"nfschina_listmyfile");
	if(listmyfile == NULL)
	{
		printf("listmyfile is NULL\n");
		return ;
	}

	filedelete = PyObject_GetAttrString(pModule,"nfschina_delete");
	if(filedelete == NULL)
	{
		printf("delete is NULL\n");
		return ;
	}

	filedownload = PyObject_GetAttrString(pModule,"nfschina_download");
	if(filedownload ==NULL)
	{
		printf("download is NULL\n");
		return ;
	}
}

int UserOption::nfschina_registor( QString username, QString password )
{
	pArgs = PyTuple_New( 2 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "s", username.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", password.toStdString().c_str()) );
	pRetValue = PyObject_CallObject( regist, pArgs );
	int uid = _PyInt_AsInt( pRetValue );
	printf("register :: uid = %d\n",uid);

	return uid;
}

int UserOption::nfschina_login( QString username, QString password, bool b_share, QString ip, QString port )
{
	printf("//service.nfschina_login\n");

	pArgs = PyTuple_New( 5 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "s", username.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", password.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "i", b_share ) );
	PyTuple_SetItem( pArgs, 3, Py_BuildValue( "s", ip.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 4, Py_BuildValue( "s", port.toStdString().c_str()) );

	pRetValue = PyObject_CallObject( login, pArgs );
	int uid = _PyInt_AsInt(pRetValue);
	printf("login :: luid = %d\n",uid);

	return uid;
}

int UserOption::nfschina_login( QString username, QString password, bool b_share )
{
	printf("//service.nfschina_login2\n");

	pArgs = PyTuple_New( 5 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "s", username.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", password.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "i", b_share ) );
	PyTuple_SetItem( pArgs, 3, Py_BuildValue( "s", serverIp.toStdString().c_str()) );
	PyTuple_SetItem( pArgs, 4, Py_BuildValue( "s", serverPort.toStdString().c_str()) );

	pRetValue = PyObject_CallObject( login, pArgs );
	int uid = _PyInt_AsInt(pRetValue);
	printf("login :: luid = %d\n",uid);

	return uid;
}

int UserOption::nfschina_keeponline( int userid, bool b_share )
{
	printf( "------------------------%s-------------------------\n", __func__ );
	printf( "userid = %d, b_share = %d\n", userid, b_share );

	pArgs = PyTuple_New( 2 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue("i", luid) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue("i", b_share) );

	pRetValue = PyObject_CallObject( keeponline, pArgs );
	int err = _PyInt_AsInt(pRetValue);

	printf("1:success -1:fail err = %d\n",err);

	return err;
}
int UserOption::nfschina_upLoad( int userid, QString filename, QString filepath )
{
	pArgs = PyTuple_New( 3 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", filename.toStdString().c_str() ) );
	PyTuple_SetItem( pArgs, 2, Py_BuildValue( "s", filepath.toStdString().c_str() ) );

	pRetValue = PyObject_CallObject( fileupload, pArgs );
	int err = _PyInt_AsInt( pRetValue );

	printf( "upload retvalue: %d \n", err );
	
	return err;
}
int UserOption::nfschina_listMyFile( int userid )
{
	pArgs = PyTuple_New ( 1 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );

	pRetValue = PyObject_CallObject( listmyfile, pArgs );
	int err = _PyInt_AsInt( pRetValue );
	printf( "listmyfile retvalue:%d\n", err );

	return err;
}
int UserOption::nfschina_delete( int userid, QString filename )
{
	pArgs = PyTuple_New( 2 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", filename.toStdString().c_str() ) );

	pRetValue = PyObject_CallObject( filedelete, pArgs );
	int err = _PyInt_AsInt( pRetValue );
	printf( "filedelete retvalue: %d\n", err );

	return err;

}
int UserOption::nfschina_download( int userid, QString filename )
{
	pArgs = PyTuple_New( 2 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", userid ) );
	PyTuple_SetItem( pArgs, 1, Py_BuildValue( "s", filename.toStdString().c_str() ) );

	pRetValue = PyObject_CallObject( filedownload, pArgs );
	int err = _PyInt_AsInt( pRetValue );
	printf( "filedownload retvalue:%d\n", err );

	return err;
}

void UserOption::toggleLocalShared( bool state )
{
	printf( "----------------------%s-----------------------\n", __func__ );
	if( b_localShared == state )
		return;
	b_localShared = state;

	printf("state = %d\n", state );
	printf("b_localShared = %d\n", b_localShared );
}

void UserOption::toggleNetShared( bool state )
{
	printf( "----------------------%s-----------------------\n", __func__ );
	if( b_netShared == state )
		return;
	b_netShared = state;

	printf("state = %d\n", state );
	printf("b_netShared = %d\n", b_netShared );
	nfschina_keeponline( getLUid(), getNetShared() );
}
