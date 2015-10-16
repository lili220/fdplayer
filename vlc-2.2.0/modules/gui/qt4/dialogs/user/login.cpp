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

LoginDialog::LoginDialog( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	setWindowTitle( qtr( "用户登陆" ) );
	setWindowRole( "vlc-login" );
	setFixedSize( 350, 300 );

	userName = "";
	passWord = "";
	b_login = false;
	//readLoginConfig();

	p_intf->p_sys->p_playlist->uid = -1;
	init();

#if 0
	userOption = UserOption::getInstance( _p_intf );
	printf( "Login:userOption = %p\n", userOption );
#endif

	connect( loginBtn, SIGNAL( clicked() ), this, SLOT( login() ) );
}

LoginDialog::~LoginDialog()
{
    saveWidgetPosition( "Login" );
}

void LoginDialog::init()
{
	/* Username and password */
	QFormLayout *formlayout = new QFormLayout;
	nameEdit = new QLineEdit( this );
	formlayout->addRow( qtr( "用户名:" ), nameEdit );
	passEdit = new QLineEdit( this );
	passEdit->setEchoMode( QLineEdit::Password );
	formlayout->addRow( qtr( "密  码:" ), passEdit );

	/*remember login info selection */
	QHBoxLayout *bottomlayout = new QHBoxLayout;
	rememberBtn = new QCheckBox( this );
	rememberBtn->setText( qtr( "记住密码" ) );
	bottomlayout->addWidget( rememberBtn );
	bottomlayout->addStretch();

	/*login button*/
	loginBtn = new QPushButton( this );
	loginBtn->setText( qtr("登陆") );
	bottomlayout->addWidget( loginBtn );

	/* main layout */
	QVBoxLayout *mainlayout = new QVBoxLayout;
	mainlayout->addLayout( formlayout );
	mainlayout->addLayout( bottomlayout );
	setLayout( mainlayout );

	restoreWidgetPosition( "User", QSize( 350, 300 ) );

}

void LoginDialog::prompt_dialog_box(char* tital, char *msg)
{
	QMessageBox msgBox( QMessageBox::Information,
			qtr(tital),
			qtr(msg),
			QMessageBox::Ok,
			NULL );
	msgBox.exec();
}

bool LoginDialog::input_check(char *tital, QString name, QString pass)
{
	QString valid_char = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_";
	int name_len = name.count();
	int pass_len = pass.count();
	if (name_len < 6 || name_len > 30 || pass_len < 6 || pass_len > 128) {
		printf( "用户名、密码的长度错误!\n" );
		prompt_dialog_box(tital, "用户名或密码无效！\n"\
			"用户名：大于等于6位且小于等于30位的大小写字母、数字和下划线!\n"\
			"密码：大于等于6位且小于等于128位的大小写字母、数字和下划线!\n");
		return false;
	}
	for (int i = 0; i < name_len; i++) {
		QChar c = name.at(i);
		if (valid_char.indexOf(c) == -1) {
			printf( "用户名含有无效字符!\n" );
			prompt_dialog_box(tital, "用户名或密码无效！\n"\
				"用户名：大于等于6位且小于等于30位的大小写字母、数字和下划线!\n"\
				"密码：大于等于6位且小于等于128位的大小写字母、数字和下划线!\n");
			return false;
		}
	}
	for (int i = 0; i < pass_len; i++) {
		QChar c = pass.at(i);
		if (valid_char.indexOf(c) == -1) {
			printf( "密码含有无效字符!\n" );
			prompt_dialog_box(tital, "用户名或密码无效！\n"\
				"用户名：大于等于6位且小于等于30位的大小写字母、数字和下划线!\n"\
				"密码：大于等于6位且小于等于128位的大小写字母、数字和下划线!\n");
			return false;
		}
	}
	return true;
}

bool LoginDialog::login()
{
	qDebug() << __func__;

	qDebug() << "username:" << nameEdit->text();
	qDebug() << "password:" << passEdit->text();
	if (!input_check("用户登录", nameEdit->text(), passEdit->text())) {
		return false;
	}

	userOption = UserOption::getInstance( p_intf );
	if( !userOption->isLoaded() )
	{
		printf( "python module is not loaded, can'n connect to server!\n" );
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "用户登录" ),
				qtr( "Python 模块没有加载成功，无法链接到服务器！" ),
				QMessageBox::Ok,
				NULL );
		msgBox.exec();
		return false;
	}

	//int uid = userOption->nfschina_login( nameEdit->text(), passEdit->text(), false, "192.168.7.97", "80" );
	//int uid = userOption->nfschina_login( nameEdit->text(), passEdit->text(), userOption->getNetShared() );
	int uid = userOption->nfschina_login( nameEdit->text(), passEdit->text() );
	printf( "Login result: %d\n", uid );
	if( uid > 0 )
	{
		userOption->setLUid( uid );// save login uid
		setLogState( true );//save login state
		p_intf->p_sys->p_playlist->p_libvlc->uid = uid;//save the uid information
	}
	else
	{
		userOption->setLUid( uid );// save login uid
		setLogState( false );//save login state
		p_intf->p_sys->p_playlist->p_libvlc->uid = -1;//reset the uid information
	}

	/* record the log information if login successfull and show log result to user */
	if( isLogin() )
	{
		userName = nameEdit->text();
		passWord = passEdit->text();

		toggleVisible();
		message = new QMessageBox( QMessageBox::Information,
				qtr( "用户登录" ),
				qtr( "登陆成功" ),
				QMessageBox::Ok,
				NULL );
		message->exec();
	}
	else
	{
		message = new QMessageBox( QMessageBox::Information,
				qtr( "用户登录" ),
				qtr( "用户名或密码错误！" ),
				QMessageBox::Ok,
				NULL );
		message->exec();
	}

	return true;
}

void LoginDialog::logout()
{
	qDebug() << __func__;
	UserOption *user = UserOption::getInstance( p_intf );
	if( !user->isLogin() )
	{
		QMessageBox msgBox( QMessageBox::Information,
				qtr( "用户退出" ),
				qtr( "当前没有用户登陆，不需要退出!" ),
				QMessageBox::Ok ,
				NULL );
		msgBox.exec();

		return ;
	}
	QMessageBox msgBox( QMessageBox::Information,
			qtr( "用户退出" ),
			qtr( "您确定要退出吗？" ),
			QMessageBox::Yes | QMessageBox::No,
			NULL );
	int ret =  msgBox.exec();
	if( ret == QMessageBox::Yes )
	{
		//saveLogInfo();
		//user->nfschina_logout( user->getLUid() );
		setLogState( false );
		UserOption::getInstance( p_intf )->setLogin( false );
		UserOption::getInstance( p_intf )->setLUid( -1 );
		p_intf->p_sys->p_playlist->p_libvlc->uid = -1;//reset the uid information
	}
	else if( ret == QMessageBox::No )
	{
		qDebug() << "Keep login states";
		return;
	}
}

