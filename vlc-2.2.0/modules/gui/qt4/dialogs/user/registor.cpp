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
#include "dialogs/user/registor.hpp"
#include "dialogs/user/login.hpp"
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

RegistorDialog::RegistorDialog( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	setWindowTitle( qtr( "用户注册" ) );
	setWindowRole( "vlc-registor" );
	setFixedSize( 350, 300 );

	/*Username password and email*/
	QFormLayout *formlayout = new QFormLayout;
	nameEdit = new QLineEdit( this );
	formlayout->addRow( qtr( "用户名:" ), nameEdit );
	passEdit = new QLineEdit( this );
	passEdit->setEchoMode( QLineEdit::Password );
	formlayout->addRow( qtr( "密  码:" ), passEdit );
	repassEdit = new QLineEdit( this );
	repassEdit->setEchoMode( QLineEdit::Password );
	formlayout->addRow( qtr( "重复密码:" ), repassEdit );
	emailEdit = new QLineEdit( this );
	formlayout->addRow( qtr( "注册邮箱:" ), emailEdit );
	
	/*login and registor buttons*/
	QHBoxLayout *bottomlayout = new QHBoxLayout;
	loginBtn = new QPushButton( this );
	loginBtn->setText( qtr( "直接登陆" ) );
	bottomlayout->addStretch();
	bottomlayout->addWidget( loginBtn );

	registorBtn = new QPushButton( this );
	registorBtn->setText( qtr( "提交注册" ) );
	bottomlayout->addWidget( registorBtn );

	QVBoxLayout *mainlayout = new QVBoxLayout;
	mainlayout->addLayout( formlayout );
	mainlayout->addLayout( bottomlayout );
	setLayout( mainlayout );

	restoreWidgetPosition( "Registor", QSize( 350, 300 ) );

	connect( registorBtn, SIGNAL( clicked() ), this, SLOT( registor() ) );
	connect( loginBtn, SIGNAL( clicked() ), SLOT( login() ) );
}

RegistorDialog::~RegistorDialog()
{
    saveWidgetPosition( "Registor" );
}

bool RegistorDialog::checkPassword()
{
	return QString::compare( passEdit->text(), repassEdit->text() ) == 0 ;
}


void RegistorDialog::registor()
{
	qDebug() << __func__;
	if( !checkPassword() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "用户注册" ),
				qtr( "两次输入的密码不一致，请重新输入！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();
	}

	userOption = UserOption::getInstance( p_intf );
	int uid = userOption->nfschina_registor( nameEdit->text(), passEdit->text() );
	qDebug() << "registor result: " << uid;
	if( uid > 0 )
	{
		toggleVisible();
		userOption->setRUid( uid );// save registor uid;
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "用户注册" ),
				qtr( "注册成功，请从登陆窗口登陆！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();
	}
	else
	{
		userOption->setRUid( uid );
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "用户注册" ),
				qtr( "注册失败！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();
	}
}

void RegistorDialog::login()
{
	toggleVisible();
	LoginDialog::getInstance( p_intf )->toggleVisible();
}

QString RegistorDialog::getUsername()
{
	return nameEdit->text();
}

QString RegistorDialog::getPassword()
{
	return passEdit->text();
}

QString RegistorDialog::getEmail()
{
	return emailEdit->text();
}
