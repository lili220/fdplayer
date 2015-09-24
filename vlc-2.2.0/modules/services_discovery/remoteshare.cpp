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
#include <python2.7/Python.h>
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
static void AddDesktop(services_discovery_t *);
static void AddChildItem(services_discovery_t *);
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
    printf("%d\n", 1);
    services_discovery_t *sd = (services_discovery_t *)p_this;
    
     printf("%d\n", 2);
     // printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    // 
    // PyRun_SimpleString( "import sys" );
    // printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    
    // PyRun_SimpleString( "sys.path.append('.')" );
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    

    
    AddDesktop (sd);
    //AddChildItem (sd);

    return 0;
}

/*****************************************************************************
 * Close: 销毁接口
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    printf("Close local\n");
    Py_Finalize();
}

/*** Whole desktop ***/
static void AddDesktop(services_discovery_t *sd)
{
    PyObject *pName,*pModule,*msg,*pRetValue,*pArgs;
    // printf("AddDesktop\n");
    Py_Initialize();

    if( !Py_IsInitialized() )
    {
        printf( "Python initialize failed! \n" );
        return ;
    }
    PyRun_SimpleString( "import sys" );
    PyRun_SimpleString( "sys.path.append('./modules/services_discovery')" );
    pName = PyString_FromString("remotemsg");
    pModule = PyImport_Import(pName);

    msg = PyObject_GetAttrString(pModule,"nfschina_msg");
    if(msg == NULL)
    {
        printf("msg is NULL\n");
        return;
    }

    pArgs = PyTuple_New( 2 );
    PyTuple_SetItem( pArgs, 0, Py_BuildValue( "i", 34) );
    PyTuple_SetItem( pArgs, 1, Py_BuildValue( "i", 3 ) );
    pRetValue = PyObject_CallObject( msg, pArgs );
    int s = PyList_Size( pRetValue );

    // list<string> msgList;
    LISTSTRING msgList;
    msgList.clear();
    LISTSTRING::iterator ii; 
    for( int i = 0; i < s; i++ )
    {
        msgList.push_back(PyString_AsString( PyList_GetItem( pRetValue, i ) ) );
    }

    printf("msgList.size = %d\n", msgList.size());
    for (ii = msgList.begin(); ii != msgList.end(); ++ii)
    {
        //printf(*ii);
        printf("%s\n", (*ii).c_str());
        input_item_t *item;
        item = input_item_NewWithType ("", _((*ii).c_str()),
                                  0, NULL, 0, -1, ITEM_TYPE_DIRECTORY);
        if (item == NULL)
        return;
        services_discovery_AddItem (sd, item, NULL);
    }   


}

static void AddChildItem(services_discovery_t *sd)
{
    PyObject *pName1,*pModule1,*msg1,*pRetValue1,*pArgs1;
    printf("AddChildItem\n");
    // Py_Initialize();

    // if( !Py_IsInitialized() )
    // {
    //     printf( "Python initialize failed! \n" );
    //     return ;
    // }
    // PyRun_SimpleString( "import sys" );
    // PyRun_SimpleString( "sys.path.append('./modules/services_discovery')" );
    pName1 = PyString_FromString("sharefile");
    printf("%d\n",__LINE__);
    pModule1 = PyImport_Import(pName1);
    printf("%d\n",__LINE__);

    msg1 = PyObject_GetAttrString(pModule1,"getfile");
    printf("%d\n",__LINE__);
    if(msg1 == NULL)
    {
        printf("msg is NULL\n");
        return;
    }

    pArgs1 = PyTuple_New( 3 );
    printf("%d\n",__LINE__);
    PyTuple_SetItem( pArgs1, 0, Py_BuildValue( "s", "192.168.7.88:8090") );
    PyTuple_SetItem( pArgs1, 1, Py_BuildValue( "s", "1001" ) );
    PyTuple_SetItem( pArgs1, 2, Py_BuildValue( "s", "1102" ) );
    printf("%d\n",__LINE__);

    pRetValue1 = PyObject_CallObject( msg1, pArgs1 );
    if(pRetValue1 == NULL)
    {
       printf("%d:pRetValue1 is NULL\n",__LINE__); 
    }
    int s = PyList_Size( pRetValue1 );

    // list<string> msgList;
    LISTSTRING msgList1;
    msgList1.clear();
    LISTSTRING::iterator ii1; 
    printf("%d\n",__LINE__);
    for( int i = 0; i < s; i++ )
    {
        msgList1.push_back(PyString_AsString( PyList_GetItem( pRetValue1, i ) ) );
        printf("%d\n",__LINE__);
    }

    // printf("msgList.size = %d\n", msgList.size());
    for (ii1 = msgList1.begin(); ii1 != msgList1.end(); ++ii1)
    {
        //printf(*ii);
        printf("%s\n", (*ii1).c_str());
        // input_item_t *item;
        // item = input_item_NewWithType ("", _((*ii).c_str()),
        //                           0, NULL, 0, -1, ITEM_TYPE_DIRECTORY);
        // if (item == NULL)
        // return;
        // services_discovery_AddItem (sd, item, NULL);
    }   
}
