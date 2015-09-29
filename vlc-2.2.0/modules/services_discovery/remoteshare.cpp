/*****************************************************************************
 * remoteshare.cpp :  remoteshare module 
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
#include <iostream>
#include <string>
#include <list>

using namespace std;
typedef list<string> LISTSTRING;  

/*
 * VLC callback prototypes
 */
static int Open( vlc_object_t* );
static void Close( vlc_object_t* );
static int vlc_sd_probe_Open( vlc_object_t * );
VLC_SD_PROBE_HELPER( "remoteshare", "Remote Share", SD_CAT_SHARE )

/*
 * Module descriptor
 */
vlc_module_begin();
    set_shortname( "remoteshare" );
    set_description( N_( "Remote Share" ) );
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
    return 0;
}

/*****************************************************************************
 * Close: 销毁接口
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    printf("Close local\n");
    return;
}

