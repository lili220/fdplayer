/*****************************************************************************
 * cloudshare.c :  cloudshare module 
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

#include <python2.7/Python.h>
#include <QString>

/*
 * VLC callback prototypes
 */
static int Open( vlc_object_t* );
static void Close( vlc_object_t* );
static int vlc_sd_probe_Open( vlc_object_t * );
static void AddDesktop(services_discovery_t *);
//VLC_SD_PROBE_HELPER( "cloudshare", "Cloud Share", SD_CAT_SHARE )
VLC_SD_PROBE_HELPER( "cloudshare", "云端共享", SD_CAT_SHARE )

/*
 * Module descriptor
 */
vlc_module_begin();
    set_shortname( "cloudshare" );
    set_description( N_( "Cloud Share" ) );
    set_category( CAT_PLAYLIST );
    set_subcategory( SUBCAT_PLAYLIST_SD );
    set_capability( "services_discovery", 0 );
    set_callbacks( Open, Close );

    VLC_SD_PROBE_SUBMODULE
vlc_module_end();


/*****************************************************************************
 * Open: 初始化接口
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    services_discovery_t *sd = (services_discovery_t *)p_this;
	printf( "global uid == %d\n", p_this->p_libvlc->uid );
    
    //AddDesktop (sd);

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
	int uid = sd->p_libvlc->uid;
    input_item_t *item,*item1;

	Py_Initialize();
	if( !Py_IsInitialized() )
	{
		printf( "Python initialize failed! \n" );
		return;
	}
	printf( "------------------%s:%d----------------\n", __func__, __LINE__ );
	PyRun_SimpleString( "import sys" );
	printf( "------------------%s:%d----------------\n", __func__, __LINE__ );
	PyRun_SimpleString( "sys.path.append('./share/python')" );
	printf( "------------------%s:%d----------------\n", __func__, __LINE__ );
	PyObject *pModule = PyImport_ImportModule( "clientrg" );
	if( pModule  == NULL )
	{
		printf( "Can't Import clientrg! \n" );
		return ;
	}
	PyObject *listmyfile = PyObject_GetAttrString(pModule,"nfschina_listmyfile");
	if(listmyfile == NULL)
	{
		printf("listmyfile is NULL\n");
		return ;
	}

	PyObject *pArgs = PyTuple_New ( 1 );
	PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", uid ) );

	PyObject *pRetValue = PyObject_CallObject( listmyfile, pArgs );
	if( pRetValue == NULL )
	{
		printf( "pRetValue for nfschina_GetFileList is NULL\n" );
		return ;
	}

	int s = PyList_Size( pRetValue );
	for( int i = 0; i < s; i++ )
	{
		QString file = PyString_AsString( PyList_GetItem( pRetValue, i ) );
		printf( "file:%s\n", file.toStdString().c_str() );
		QString url = "http://192.168.7.97/download/";
		QString uidstr; uidstr.setNum( uid );
		url.append( uidstr );
		url.append( "/" );
		url.append( file.toUtf8().constData() );
		printf( "url:%s\n", url.toStdString().c_str() );

		item = input_item_NewWithType (url.toStdString().c_str(), _(file.toStdString().c_str()),
				0, NULL, 0, -1, ITEM_TYPE_CARD);
		if (item == NULL)
			continue;
		services_discovery_AddItem (sd, item, NULL);
	}

#if 0
	//printf( "global uid == %d\n", uid );
	item1 = input_item_NewWithType ("file:///home/wp/下载/女儿情.mp3", _("wo"),
			0, NULL, 0, -1, ITEM_TYPE_CARD);
	item = input_item_NewWithType ("http://192.168.7.82/static/paomo.mp3", _("wodezhuomian"),
			0, NULL, 0, -1, ITEM_TYPE_CARD);
	if (item == NULL)
		return;

	services_discovery_AddItem (sd, item, NULL);
	services_discovery_AddItem (sd, item1, NULL);
#endif
}
