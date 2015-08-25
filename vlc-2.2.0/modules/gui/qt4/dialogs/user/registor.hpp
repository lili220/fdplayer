/*****************************************************************************
 * user.hpp : User dialogs
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

#ifndef QVLC_REGISTOR_DIALOG_H_
#define QVLC_REGISTOR_DIALOG_H_ 1

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt4.hpp"

#include "util/qvlcframe.hpp"
#include "util/singleton.hpp"

#include "ui/update.h"
#include "useroption.hpp"


class QEvent;
class QPushButton;
class QLineEdit;

class RegistorDialog : public QVLCFrame, public Singleton<RegistorDialog>
{
    Q_OBJECT
public:
    RegistorDialog( intf_thread_t * );
    virtual ~RegistorDialog();
	bool checkPassword();

public slots:
    virtual void close() { toggleVisible(); }
	void registor();
	void login();

    friend class Singleton<RegistorDialog>;

private:
	UserOption *userOption;
	QString getUsername();
	QString getPassword();
	QString getEmail();

	QLineEdit* nameEdit;
	QLineEdit*   passEdit;
	QLineEdit*   repassEdit;
	QLineEdit*   emailEdit;
	QPushButton* registorBtn;
	QPushButton* loginBtn;
};
#endif
