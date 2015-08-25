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

class UserOption : public QObject, /*public QVLCFrame,*/ public Singleton<UserOption>
{
    Q_OBJECT
public:
    UserOption( intf_thread_t * _p_intf );
    virtual ~UserOption();
	void init();
	int nfschina_registor( QString username, QString password );
	int nfschina_login( QString username, QString password, bool b_share, QString ip, QString port );
	int nfschina_login( QString username, QString password, bool b_share );
	int nfschina_keeponline( int userid, bool b_share );
	int nfschina_upLoad( int userid, QString filename, QString filepath );
	int nfschina_listMyFile( int userid );
	int nfschina_delete( int userid, QString filename );
	int nfschina_download( int userid, QString filename );

	int getRUid(){ return ruid; }
	int getLUid() {return luid; }
	void setRUid( int uid ) { ruid = uid; }
	void setLUid( int uid ) { luid = uid; }

	void setServerIp( QString ip ){ serverIp = ip; }
	void setServerPort( QString port ){ serverPort = port; }
	
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
	int ruid;
	int luid;

	bool b_localShared;
	bool b_netShared;

	QString serverIp;
	QString serverPort;

	PyObject *pArgs;
	PyObject *pRetValue;

	PyObject *pModule;
	PyObject *regist;
	PyObject *login;
	PyObject *keeponline;
	PyObject *fileupload;
	PyObject *listmyfile;
	PyObject *filedelete;
	PyObject *filedownload;
};

#endif
