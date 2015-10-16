/*****************************************************************************
 * login.hpp : User Login dialogs
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

#ifndef QVLC_LOGIN_DIALOG_H_
#define QVLC_LOGIN_DIALOG_H_ 1

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt4.hpp"

#include "util/qvlcframe.hpp"
#include "util/singleton.hpp"
#include "useroption.hpp"

class QEvent;
class QPushButton;
class QCheckBox;
class QMessageBox;
class QLineEdit;

class LoginDialog : public QVLCFrame, public Singleton<LoginDialog>
{
    Q_OBJECT
public:
    LoginDialog( intf_thread_t * );
    virtual ~LoginDialog();
	void init();
	static bool input_check(char*, QString, QString);

public slots:
    virtual void close() { toggleVisible(); }
    bool login();
	void logout();

    friend class    Singleton<LoginDialog>;

	QString getUsername(){ return userName; }
	QString getPassword(){ return passWord; }

	bool isLogin(){ return b_login == true; }
	void setLogState(bool state ){ b_login = state; }

private:
	UserOption *userOption;

	QString userName;
	QString passWord;
	bool b_login;//是否已登录
	static void prompt_dialog_box(char *, char*);

	QLineEdit* nameEdit;
	QLineEdit*   passEdit;
	QPushButton* loginBtn;
	QCheckBox *rememberBtn;

	QMessageBox *message;
};

#endif
