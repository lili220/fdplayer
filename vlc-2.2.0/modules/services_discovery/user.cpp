/*****************************************************************************
 * User.cpp : User Options
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

#include "user.hpp"

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

#include <python2.7/Python.h>


#include <assert.h>

User::User()
{
	qDebug() << "-------------------"<< __func__ << "----------------------------";
	if( access("../sbin/minidlna.conf", F_OK ) >= 0 )
		setConfigPath( "../sbin/minidlna.conf" );
	else if( access("./minidlna-1.1.4/minidlna.conf", F_OK ) >= 0 )
		setConfigPath( "./minidlna-1.1.4/minidlna.conf" );
	else
		qDebug() << " minidlna.conf not found!";
	//qDebug() << getConfigPath();

	setSharePath( getConfigPath() );
	//qDebug() << getSharePath();
}

User::~User()
{
	printf("-----------------------%s---------------------------\n", __func__ );
}

QString User::getSharePath()
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

