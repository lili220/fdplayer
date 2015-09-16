/*****************************************************************************
 * localshare.c :  localshare module 
 *****************************************************************************
 * Author: wangpei
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdarg.h>
#include <xcb/xcb.h>
typedef xcb_atom_t Atom;
#include <X11/Xatom.h> 
#include <vlc_common.h>
#include <vlc_services_discovery.h>
#include <vlc_dialog.h>
#include <vlc_charset.h>
#include <vlc_plugin.h>
#ifdef HAVE_SEARCH_H
# include <search.h>
#endif
#include <poll.h>

#include "user.hpp"

#include <QDebug>
#include <QDir>

/*
 * VLC callback prototypes
 */
static int Open( vlc_object_t* );
static void Close( vlc_object_t* );
static int vlc_sd_probe_Open( vlc_object_t * );
static void AddDesktop(services_discovery_t *);
static int onNewFileAdded( vlc_object_t*, char const *, vlc_value_t, vlc_value_t, void *);
VLC_SD_PROBE_HELPER( "localshare", "Local Share", SD_CAT_SHARE )

/*
 * Module descriptor
 */
vlc_module_begin();
    set_shortname( "localshare" );
    set_description( N_( "Local Share" ) );
	add_shortcut("localshare");
    set_category( CAT_PLAYLIST );
    set_subcategory( SUBCAT_PLAYLIST_SD );
    set_capability( "services_discovery", 0 );
    set_callbacks( Open, Close );

    VLC_SD_PROBE_SUBMODULE
vlc_module_end();

struct services_discovery_sys_t
{
	vlc_thread_t thread;
};

/*****************************************************************************
 * Open: 初始化接口
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
	printf( "----------------------------func:%s------------------------------------\n", __func__) ;
    services_discovery_t *sd = (services_discovery_t *)p_this;
	services_discovery_sys_t *p_sys = molloc( sizeof(*p_sys) );
	if( p_sys == NULL )
		return VLC_ENOMEM;
	sd->p_sys = p_sys;

	if( vlc_clone(&p_sys->thread, Run, sd, VLC_THREAD_PRIORITY_LOW ) )
		goto error;
	return VLC_SUCCESS;

	var_AddCallback( sd->p_libvlc, "localshare", onNewFileAdded, sd );

    AddDesktop (sd);

	printf( "------------------------%s return -----------------------------------\n", __func__ );
error:
	free( p_sys );
	return VLC_EGENERIC;

    return 0;
}

/*****************************************************************************
 * Close: 销毁接口
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    printf("Close local\n");
}

/*** Whole desktop ***/
static void AddDesktop(services_discovery_t *sd)
{
    printf("AddDesktop\n");
	User user;
	printf( "local sharepath == [%s]\n", user.getSharePath().toStdString().c_str() );

	QString path = user.getSharePath();
	QDir *dir = new QDir( path );

	QFileInfoList entries = dir->entryInfoList();
	foreach(const QFileInfo &file, entries )
	{
		if( file.isDir() )
		{
			qDebug() << "dir :" << file.fileName();
		}
		else
		{
			input_item_t *item;
			QString url = "file://";
			item = input_item_NewWithType ( url.append(file.absoluteFilePath()).toStdString().c_str(), _(file.fileName().toStdString().c_str() ),
					0, NULL, 0, -1, ITEM_TYPE_CARD);
			qDebug() << "url:" << url;
			if (item == NULL)
				continue;
			services_discovery_AddItem (sd, item, NULL);
		}
	}
}
static int onNewFileAdded( vlc_object_t *p_this, char const *psz_var, vlc_value_t oldval, vlc_value_t newval, void *p_data )
{
	printf( "-----------------------%s-------------------------\n", __func__ );

	return 0;
}

static void *Run( void *data )
{
	printf( "------------------%s-------------------\n", __func__ );
	services_discovery_t *sd = data;
	services_discovery_sys_t *p_sys = sd->p_sys;

	AddDesktop( sd );
}
