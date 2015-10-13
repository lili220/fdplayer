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
#include "dialogs/user/task.hpp"
#include "dialogs/user/useroption.hpp"
#include "util/qt_dirs.hpp"
#include <QProgressDialog>
#include <QHBoxLayout>

#include <python2.7/Python.h>
TaskDialog::TaskDialog( intf_thread_t *_p_intf ) : QVLCFrame( _p_intf )
{
	QProgressDialog *process = new QProgressDialog( this );
	process->setLabelText( qtr("Processing...") );
	process->setRange( 0, 50000);
	process->setModal( true );
	process->setCancelButtonText( qtr("cancel") );
	//process->show();

	QHBoxLayout *layout = new QHBoxLayout( this );
	layout->addWidget( process );
	setLayout(layout);
}

TaskDialog::~TaskDialog()
{
    //saveWidgetPosition( "Login" );
}
