/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2005           Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005,2007-2009, 2011 Spiro Trikaliotis
 *  Copyright 2009,2011           Arnd Menge <arnd(at)jonnz(dot)de>
 *
*/

/*! ************************************************************** 
** \file lib/cbm.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**
****************************************************************/

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM.DLL"

#include "debug.h"

#include <stdlib.h>
#include <string.h>

#include "libmisc.h"

//! mark: We are building the DLL */
#define DLL
#include "opencbm.h"
#include "archlib.h"

#include "opencbm-plugin.h"

#include "getpluginaddress.h"

#include "configuration.h"

#include "arch.h"

/*! \brief @@@@@ \todo document

 \param Handle

 \param PluginName

 \return
*/
int plugin_is_active(opencbm_configuration_handle Handle, const char PluginName[])
{
    char * active = NULL;
    unsigned int active_number = 1;

    FUNC_ENTER();

    do {
        char * next;

        opencbm_configuration_get_data(Handle, PluginName, "Active", &active);

        if ( ! active ) {
            break;
        }

        active_number = strtoul(active, &next, 10);
        if (next && (next[0] != 0) ) {
            DBG_WARN((DBG_PREFIX "Entry 'Active' for plugin '%' "
                "is set to the non-numerical value '%s'",
                PluginName, active));
            active_number = 1;
            break;
        }

    } while (0);

    cbmlibmisc_strfree(active);

    FUNC_LEAVE_INT(active_number);
}

/*! \brief @@@@@ \todo document

 \param PluginName

 \return
*/
int plugin_set_active(const char PluginName[])
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(0);
}


/*! \brief @@@@@ \todo document

 \param PluginName

 \return
*/
int plugin_set_inactive(const char PluginName[])
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(0);
}


/*! \brief @@@@@ \todo document */
struct plugin_information_s {
    SHARED_OBJECT_HANDLE Library; /*!< \brief @@@@@ \todo document */
    opencbm_plugin_t     Plugin;  /*!< \brief @@@@@ \todo document */
};

/*! \brief @@@@@ \todo document */
typedef struct plugin_information_s plugin_information_t;

static
struct plugin_information_s Plugin_information = { 0 };

struct plugin_read_pointer
{
    UINT_PTR offset;
    const char * name;
};

#define OFFSETOF(_type, _element) \
    ((UINT_PTR) (void *) &((_type *)0)->_element)

#define GETELEMENT(_type, _structpointer, _elementoffset) \
    ((_type *)(((char*)(_structpointer)) + (_elementoffset)))

#define PLUGIN_POINTER_DEF(_xxx) \
    { OFFSETOF(plugin_information_t, Plugin._xxx), #_xxx }

#define PLUGIN_POINTER_GET(_what, _element) \
    GEELEMENT(void, _what, _element)

#define PLUGIN_POINTER_END() \
    { 0, NULL }

static struct plugin_read_pointer plugin_pointer_to_read_mandatory[] =
{
	PLUGIN_POINTER_DEF(opencbm_plugin_get_driver_name),
	PLUGIN_POINTER_DEF(opencbm_plugin_driver_open),
	PLUGIN_POINTER_DEF(opencbm_plugin_driver_close),
	PLUGIN_POINTER_DEF(opencbm_plugin_raw_write),
	PLUGIN_POINTER_DEF(opencbm_plugin_raw_read),
	PLUGIN_POINTER_DEF(opencbm_plugin_open),
	PLUGIN_POINTER_DEF(opencbm_plugin_close),
	PLUGIN_POINTER_DEF(opencbm_plugin_listen),
	PLUGIN_POINTER_DEF(opencbm_plugin_talk),
	PLUGIN_POINTER_DEF(opencbm_plugin_unlisten),
	PLUGIN_POINTER_DEF(opencbm_plugin_untalk),
	PLUGIN_POINTER_DEF(opencbm_plugin_get_eoi),
	PLUGIN_POINTER_DEF(opencbm_plugin_clear_eoi),
	PLUGIN_POINTER_DEF(opencbm_plugin_reset),
	PLUGIN_POINTER_DEF(opencbm_plugin_iec_poll),
	PLUGIN_POINTER_DEF(opencbm_plugin_iec_setrelease),
	PLUGIN_POINTER_DEF(opencbm_plugin_iec_wait),
    PLUGIN_POINTER_END()
}; 

static struct plugin_read_pointer plugin_pointer_to_read_optional[] =
{
    PLUGIN_POINTER_DEF(opencbm_plugin_init),
    PLUGIN_POINTER_DEF(opencbm_plugin_uninit),
	PLUGIN_POINTER_DEF(opencbm_plugin_lock),
	PLUGIN_POINTER_DEF(opencbm_plugin_unlock),
	PLUGIN_POINTER_DEF(opencbm_plugin_iec_set),
	PLUGIN_POINTER_DEF(opencbm_plugin_iec_release),
	PLUGIN_POINTER_DEF(opencbm_plugin_parallel_burst_read),
	PLUGIN_POINTER_DEF(opencbm_plugin_parallel_burst_write),
	PLUGIN_POINTER_DEF(opencbm_plugin_parallel_burst_read_track),
	PLUGIN_POINTER_DEF(opencbm_plugin_parallel_burst_write_track),
	PLUGIN_POINTER_DEF(opencbm_plugin_pp_read),
	PLUGIN_POINTER_DEF(opencbm_plugin_pp_write),
    PLUGIN_POINTER_END()
};

static struct plugin_read_pointer plugin_pointer_to_read_parallel_burst[] =
{
	PLUGIN_POINTER_DEF(opencbm_plugin_parallel_burst_read),
	PLUGIN_POINTER_DEF(opencbm_plugin_parallel_burst_write),
	PLUGIN_POINTER_DEF(opencbm_plugin_parallel_burst_read_track),
	PLUGIN_POINTER_DEF(opencbm_plugin_parallel_burst_write_track),
    PLUGIN_POINTER_END()
};

static struct plugin_read_pointer plugin_pointer_to_read_pp_readwrite[] =
{
	PLUGIN_POINTER_DEF(opencbm_plugin_pp_read),
	PLUGIN_POINTER_DEF(opencbm_plugin_pp_write),
    PLUGIN_POINTER_END()
};

static struct plugin_read_pointer plugin_pointer_to_read_srq_burst[] =
{
	PLUGIN_POINTER_DEF(opencbm_plugin_srq_burst_read ),
	PLUGIN_POINTER_DEF(opencbm_plugin_srq_burst_write),
	PLUGIN_POINTER_DEF(opencbm_plugin_srq_burst_read_track),
	PLUGIN_POINTER_DEF(opencbm_plugin_srq_burst_write_track),
    PLUGIN_POINTER_END()
};

static struct plugin_read_pointer plugin_pointer_to_read_tape[] =
{
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_prepare_capture),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_prepare_write),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_get_sense),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_wait_for_stop_sense),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_wait_for_play_sense),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_motor_on),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_motor_off),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_start_capture),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_start_write),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_get_ver),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_download_config),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_upload_config),
	PLUGIN_POINTER_DEF(opencbm_plugin_tap_break),
    PLUGIN_POINTER_END()
};


struct plugin_read_pointer_group
{
    struct plugin_read_pointer * read_pointer;
    enum {
        PRP_MANDATORY,
        PRP_OPTIONAL,
        PRP_OPTIONAL_ALL_OR_NOTHING
    } type;
};

static struct plugin_read_pointer_group read_pointer_group[] =
{
    { plugin_pointer_to_read_mandatory, PRP_MANDATORY },
    { plugin_pointer_to_read_optional, PRP_OPTIONAL },
    { plugin_pointer_to_read_parallel_burst, PRP_OPTIONAL_ALL_OR_NOTHING },
    { plugin_pointer_to_read_pp_readwrite, PRP_OPTIONAL_ALL_OR_NOTHING },
    { plugin_pointer_to_read_srq_burst, PRP_OPTIONAL_ALL_OR_NOTHING },
    { plugin_pointer_to_read_tape, PRP_OPTIONAL_ALL_OR_NOTHING },
    { NULL, PRP_OPTIONAL }
};

static void
read_plugin_pointer(
        plugin_information_t * Plugin_information,
        struct plugin_read_pointer pointer_to_read[]
)
{
    while (pointer_to_read->name) {
        void * ptr_read = plugin_get_address(Plugin_information->Library, pointer_to_read->name);
        void ** ptr_to_save = GETELEMENT(void *, Plugin_information, pointer_to_read->offset);
        *ptr_to_save = ptr_read;
        DBGDO(
            if (ptr_read == NULL) {
               DBG_PRINT((DBG_PREFIX "Plugin entry point %s not found!", pointer_to_read->name));
            }
        )
        ++pointer_to_read;
    }
}

static int check_plugin_pointer(
        plugin_information_t * Plugin_information,
        struct plugin_read_pointer pointer_to_check[]
)
{
    while (pointer_to_check->name) {
        void ** ptr_to_check = GETELEMENT(void *, Plugin_information, pointer_to_check->offset);
        DBGDO(
            if (ptr_to_check == NULL) {
               DBG_PRINT((DBG_PREFIX "Plugin entry point %s not found!", pointer_to_check->name));
            }
        )
        if (ptr_to_check == NULL) return 1;
        ++pointer_to_check;
    }
    return 0;
}

static int check_plugin_pointer_all_or_nothing(
        plugin_information_t * Plugin_information,
        struct plugin_read_pointer pointer_to_check[]
)
{
    int error;

    // first, check if all pointers are available
    error = check_plugin_pointer(Plugin_information, pointer_to_check);
    if (!error) return 0;

    // if not, check if all pointers are *not* available
    while (pointer_to_check->name) {
        void ** ptr_to_check = GETELEMENT(void *, Plugin_information, pointer_to_check->offset);
        DBGDO(
            if (ptr_to_check != NULL) {
               DBG_PRINT((DBG_PREFIX "Plugin entry point %s found, but it should not be available!", pointer_to_check->name));
            }
        )
        if (ptr_to_check != NULL) return 1;
        ++pointer_to_check;
    }
    return 0;
}

static int
read_plugin_pointer_groups(
        plugin_information_t * Plugin_information,
        struct plugin_read_pointer_group pointer_to_read_group[]
)
{
    int error = 0;

    while (pointer_to_read_group->read_pointer) {
        read_plugin_pointer(Plugin_information, pointer_to_read_group->read_pointer);

        switch (pointer_to_read_group->type) {
        case PRP_MANDATORY:
            error = error || check_plugin_pointer(Plugin_information, pointer_to_read_group->read_pointer);
            break;

        case PRP_OPTIONAL:
            break;
        case PRP_OPTIONAL_ALL_OR_NOTHING:
            error = error || check_plugin_pointer_all_or_nothing(Plugin_information, pointer_to_read_group->read_pointer);
            break;
        };

        ++pointer_to_read_group;
    }

    return error;
}

static int
initialize_plugin_pointer(plugin_information_t *Plugin_information, const char * const Adapter)
{
    int error = 1;

    const char * configurationFilename = configuration_get_default_filename();

    char * plugin_name = NULL;
    char * plugin_location = NULL;

    do {

        opencbm_configuration_handle handle_configuration;

        if (configurationFilename == NULL) {
            DBG_ERROR((DBG_PREFIX "Do not know where the plugin information is stored!\n"));
            break;
        }

        handle_configuration = opencbm_configuration_open(configurationFilename);

        if (handle_configuration)
        {
            int error = 0;

            if (Adapter == NULL)
            {
                //
                // get the name of the default plugin
                //

                error = opencbm_configuration_get_data(handle_configuration, 
                           "plugins", "default", &plugin_name);
            }
            else
            {
                //
                // Use the given plugin name
                //
                plugin_name = cbmlibmisc_strdup(Adapter);
            }

            //
            // check if the plugin has been disabled
            //

            if ( ! plugin_is_active(handle_configuration, plugin_name) ) {
                error = 1;
                break;
            }

            //
            // get the location of the plugin
            //
            if (!error)
            {
                error = opencbm_configuration_get_data(handle_configuration,
                           plugin_name, "location", &plugin_location);
            }

            //
            // if an error occurred, make sure that no plugin will be loaded!
            //
            if (error)
            {
                cbmlibmisc_strfree(plugin_location);
                plugin_location = NULL;
            }

            opencbm_configuration_close(handle_configuration);
        }
        else
        {
            DBG_ERROR((DBG_PREFIX "Cannot open config file '%s'.\n", configurationFilename));
            break;
        }
        DBG_PRINT((DBG_PREFIX "Using plugin at '%s'", plugin_location ? plugin_location : "(none)"));

        memset(&Plugin_information->Plugin, 0, sizeof(Plugin_information->Plugin));

        Plugin_information->Library = plugin_load(plugin_location);

        DBG_PRINT((DBG_PREFIX "plugin_load() returned %p", Plugin_information->Library));

        if (!Plugin_information->Library) {
            DBG_ERROR((DBG_PREFIX "Could not open plugin driver at '%s'.\n", plugin_location));
            break;
        }

        error = read_plugin_pointer_groups(Plugin_information, read_pointer_group);
        if (error) {
            DBG_ERROR((DBG_PREFIX "The entry points of plugin %s do not validate correctly.\n",
                                  plugin_location));
            break;
        }

        if (Plugin_information->Plugin.opencbm_plugin_init) {
            error = Plugin_information->Plugin.opencbm_plugin_init();
            if (error) {
                DBG_ERROR((DBG_PREFIX "Plugin %s fails to initialize itself.\n",
                            plugin_location));
                if (Plugin_information->Plugin.opencbm_plugin_uninit) {
                    Plugin_information->Plugin.opencbm_plugin_uninit();
                }
                break;
            }
        }

    } while (0);

    cbmlibmisc_strfree(plugin_name);
    cbmlibmisc_strfree(plugin_location);
    cbmlibmisc_strfree(configurationFilename);

    return error;
}

static void
uninitialize_plugin(void)
{
    if (Plugin_information.Library != NULL)
    {
        if (Plugin_information.Plugin.opencbm_plugin_uninit) {
            Plugin_information.Plugin.opencbm_plugin_uninit();
        }

        plugin_unload(Plugin_information.Library);

        Plugin_information.Library = NULL;
    }
}

static int
initialize_plugin(const char * const Adapter)
{
    /* if library is already opened and initialized correctly then 
       this function returns OK */
    int error = 0;

    /* init pointers if library was not yet opened */
    if (Plugin_information.Library == NULL)
    {
        /* if pointer init failed then close library and make Library NULL */
        error = initialize_plugin_pointer(&Plugin_information, Adapter);
        if(error != 0)
        {
            uninitialize_plugin();
        }
    }

    return error;
}

// #define DBG_DUMP_RAW_READ
// #define DBG_DUMP_RAW_WRITE

/*! \brief Split adapter in adapter and port

 This function extracts the adapter name and the port
 out of the given adapter specification.

 \param Adapter
   The name of the adapter to be used.
   The format is specified in the remarks section below.

 \param Port
   Pointer to a pointer to char which will get the specified port.
   This data has to be freed with cbmlibmisc_strfree() afterwards!

   It is set to NULL if no port was specified.

 \return
   NULL: an error occurred.
   Else: A pointer to the "stripped" adapter specification.
         This data has to be freed with cbmlibmisc_strfree() afterwards!

 \remark
   The Adapter consists of two parts: The name, a colon, and a port.
   The given pointer can be NULL, in which case the default adapter will be used.
   If specified, the name must be the same name as the section in the INI file.
   You can add a colon and a number, opening a specific bus on this device.
   You can also omit the adapter name, thus, only giving a colon and a number,
   in which case the specified bus will be used on the default adapter
*/

static char *
cbm_split_adapter_in_name_and_port(char * Adapter, char ** Port)
{
    char * adapter_stripped = NULL;

    *Port = NULL;

    if (Adapter) 
    {
        char *p = strchr(Adapter, ':');

        if (p)
        {
            // there was some colon (:), thus, extract portNumber and name

            adapter_stripped = cbmlibmisc_strndup(Adapter, p - Adapter);

            if (adapter_stripped != NULL) {
                if (p[1] != 0)
                {
                    *Port = cbmlibmisc_strdup(p+1);
                }
            }
        }
        else
        {
            adapter_stripped = cbmlibmisc_strdup(Adapter);
        }
    }

    if (adapter_stripped && adapter_stripped[0] == 0) {
        cbmlibmisc_strfree(adapter_stripped);
        adapter_stripped = NULL;
    }

    return adapter_stripped;
}

/*-------------------------------------------------------------------*/
/*--------- DRIVER HANDLING -----------------------------------------*/

/*! \brief Get the name of the driver for a specific parallel port, extended version

 Get the name of the driver for a specific parallel port.

 \param Adapter
   The name of the adapter to be used.
   The format is given in the documentation for cbm_split_adapter_in_name_and_port().

 \return 
   Returns a pointer to a null-terminated string containing the
   driver name, or NULL if an error occurred.
*/

const char * CBMAPIDECL
cbm_get_driver_name_ex(char * Adapter)
{
    const char *ret = NULL;

    static const char * buffer = NULL;
    char *adapter_stripped = NULL;
    char *port = NULL;

    int error;

    FUNC_ENTER();

    if ( buffer ) {
        cbmlibmisc_strfree(buffer);
        buffer = NULL;
    }

    if (Adapter != NULL)
    {
        adapter_stripped = cbm_split_adapter_in_name_and_port(Adapter, &port);

        DBG_PRINT((DBG_PREFIX 
            (adapter_stripped == NULL 
              ? "Using default adapter"
              : "Using adapter '%s', that is, adapter '%s' with port '%s'"),
            Adapter, adapter_stripped, port));
    }

    error = initialize_plugin(adapter_stripped);

    if (error == 0) {
        ret = Plugin_information.Plugin.opencbm_plugin_get_driver_name(port);
    }
    else {
        ret = "NO PLUGIN DRIVER!";
    }

    buffer = cbmlibmisc_strdup(ret);

    cbmlibmisc_strfree(adapter_stripped);
    cbmlibmisc_strfree(port);

    FUNC_LEAVE_STRING(buffer);
}

/*! \brief Get the name of the driver for a specific parallel port

 Get the name of the driver for a specific parallel port.

 \param PortNumber
   The port number for the driver to open. 0 means "default" driver, while
   values != 0 enumerate each driver.

 \return 
   Returns a pointer to a null-terminated string containing the
   driver name, or NULL if an error occurred.

 \bug
   PortNumber is not allowed to exceed 10. 

 \note
   Do not use this function.
   It is only there for compatibility reasons with older applications.
   Use cbm_get_driver_name_ex() instead!
*/

const char * CBMAPIDECL
cbm_get_driver_name(int PortNumber)
{
    char number[] = ":0";

    FUNC_ENTER();

    if (PortNumber >= 0 && PortNumber < 10) {
        number[1] = PortNumber + '0';
    }

    FUNC_LEAVE_STRING(cbm_get_driver_name_ex(number));
}

/*! \brief Opens the driver, extended version

 This function Opens the driver.

 \param HandleDevice  
   Pointer to a CBM_FILE which will contain the file handle of the driver.

 \param Adapter
   The name of the adapter to be used.
   The format is given in the documentation for cbm_split_adapter_in_name_and_port().

 \return
   ==0: This function completed successfully
   !=0: otherwise

 \remark
 cbm_driver_open_ex() should be balanced with cbm_driver_close().
*/

int CBMAPIDECL 
cbm_driver_open_ex(CBM_FILE *HandleDevice, char * Adapter)
{
    int error;
    char * port = NULL;
    char * adapter_stripped = NULL;

    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "cbm_driver_open_ex() called"));

    if (Adapter != NULL)
    {
        adapter_stripped = cbm_split_adapter_in_name_and_port(Adapter, &port);

        DBG_PRINT((DBG_PREFIX "Using adapter '%s', that is, adapter '%s' with port '%s'",
            Adapter, adapter_stripped, port));
    }

    error = initialize_plugin(adapter_stripped);

    cbmlibmisc_strfree(adapter_stripped);

    if (error == 0) {
        error = Plugin_information.Plugin.opencbm_plugin_driver_open(HandleDevice, port);
    }

    cbmlibmisc_strfree(port);

    FUNC_LEAVE_INT(error);
}

/*! \brief Opens the driver

 This function Opens the driver.

 \param HandleDevice  
   Pointer to a CBM_FILE which will contain the file handle of the driver.

 \param PortNumber
   The port number of the driver to open. 0 means "default" driver, while
   values != 0 enumerate each driver.

 \return
   ==0: This function completed successfully
   !=0: otherwise

 PortNumber is not allowed to exceed 10. 

 \note
   Do not use this function.
   It is only available for compatibility reasons with older applications.
   Use cbm_driver_open_ex() instead!

 cbm_driver_open() should be balanced with cbm_driver_close().
*/

int CBMAPIDECL 
cbm_driver_open(CBM_FILE *HandleDevice, int PortNumber)
{
    char number[] = ":0";

    FUNC_ENTER();

    if (PortNumber >= 0 && PortNumber < 10) {
        number[1] = PortNumber + '0';
    }

    FUNC_LEAVE_INT(cbm_driver_open_ex(HandleDevice, number));
}

/*! \brief Closes the driver

 Closes the driver, which has be opened with cbm_driver_open() before.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 cbm_driver_close() should be called to balance a previous call to
 cbm_driver_open(). 
 
 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().
*/

void CBMAPIDECL
cbm_driver_close(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    Plugin_information.Plugin.opencbm_plugin_driver_close(HandleDevice);

    uninitialize_plugin();

    FUNC_LEAVE();
}

/*! \brief Lock the parallel port for the driver

 This function locks the driver onto the parallel port. This way,
 no other program or driver can allocate the parallel port and
 interfere with the communication.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().

 \remark
 A call to cbm_lock() is undone with a call to cbm_unlock().

 Note that it is *not* necessary to call this function
 (or cbm_unlock()) when all communication is done with
 the handle to opencbm open (that is, between 
 cbm_driver_open() and cbm_driver_close(). You only
 need this function to pin the driver to the port even
 when cbm_driver_close() is to be executed (for example,
 because the program terminates).
*/

void CBMAPIDECL
cbm_lock(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_lock)
        Plugin_information.Plugin.opencbm_plugin_lock(HandleDevice);

    FUNC_LEAVE();
}

/*! \brief Unlock the parallel port for the driver

 This function unlocks the driver from the parallel port.
 This way, other programs and drivers can allocate the
 parallel port and do their own communication with
 whatever device they use.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().

 \remark
 Look at cbm_lock() for an explanation of this function.
*/

void CBMAPIDECL
cbm_unlock(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_unlock)
        Plugin_information.Plugin.opencbm_plugin_unlock(HandleDevice);

    FUNC_LEAVE();
}

/*-------------------------------------------------------------------*/
/*--------- BASIC I/O -----------------------------------------------*/

/*! \brief Write data to the IEC serial bus

 This function sends data after a cbm_listen().

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which hold the bytes to write to the bus.

 \param Count
   Number of bytes to be written.

 \return
   >= 0: The actual number of bytes written. 
   <0  indicates an error.

 This function tries to write Count bytes. Anyway, if an error
 occurs, this function can stop prematurely.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_raw_write(CBM_FILE HandleDevice, const void *Buffer, size_t Count)
{
    FUNC_ENTER();

#ifdef DBG_DUMP_RAW_WRITE
    DBG_MEMDUMP("cbm_raw_write", Buffer, Count);
#endif

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_raw_write(HandleDevice,Buffer, Count));
}


/*! \brief Read data from the IEC serial bus

 This function retrieves data after a cbm_talk().

 \param HandleDevice 
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which will hold the bytes read.

 \param Count
   Number of bytes to be read at most.

 \return
   >= 0: The actual number of bytes read. 
   <0  indicates an error.

 At most Count bytes are read.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_raw_read(CBM_FILE HandleDevice, void *Buffer, size_t Count)
{
    int bytesRead = 0;

    FUNC_ENTER();

    bytesRead = Plugin_information.Plugin.opencbm_plugin_raw_read(HandleDevice, Buffer, Count);

#ifdef DBG_DUMP_RAW_READ
    DBG_MEMDUMP("cbm_raw_read", Buffer, bytesRead);
#endif

    FUNC_LEAVE_INT(bytesRead);
}

/*! \brief Send a LISTEN on the IEC serial bus

 This function sends a LISTEN on the IEC serial bus.
 This prepares a LISTENer, so that it will wait for our
 bytes we will write in the future.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param SecondaryAddress
   The secondary address for the device on the IEC serial bus.

 \return
   0 means success, else failure

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_listen(CBM_FILE HandleDevice, unsigned char DeviceAddress, unsigned char SecondaryAddress)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_listen(HandleDevice, DeviceAddress, SecondaryAddress));
}

/*! \brief Send a TALK on the IEC serial bus

 This function sends a TALK on the IEC serial bus.
 This prepares a TALKer, so that it will prepare to send
 us some bytes in the future.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param SecondaryAddress
   The secondary address for the device on the IEC serial bus.

 \return
   0 means success, else failure

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_talk(CBM_FILE HandleDevice, unsigned char DeviceAddress, unsigned char SecondaryAddress)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_talk(HandleDevice, DeviceAddress, SecondaryAddress));
}

/*! \brief Open a file on the IEC serial bus

 This function opens a file on the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param SecondaryAddress
   The secondary address for the device on the IEC serial bus.

 \param Filename
   The filename of the file to be opened

 \param FilenameLength
   The size of the Filename. If zero, the Filename has to be
   a null-terminated string.

 \return
   0 means success, else failure

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_open(CBM_FILE HandleDevice, unsigned char DeviceAddress, unsigned char SecondaryAddress, 
         const void *Filename, size_t FilenameLength)
{
    int returnValue;

    FUNC_ENTER();

    returnValue = Plugin_information.Plugin.opencbm_plugin_open(HandleDevice, DeviceAddress, SecondaryAddress);

    if (returnValue == 0)
    {
        returnValue = 0;

        if(Filename != NULL)
        {
            if (FilenameLength == 0)
            {
                DBG_WARN((DBG_PREFIX "*** FilenameLength of 0 encountered!"));
                FilenameLength = strlen(Filename);
            }

            if (FilenameLength > 0)
            {
                returnValue = 
                    (size_t) (cbm_raw_write(HandleDevice, Filename, FilenameLength))
                    != FilenameLength;
            }
            cbm_unlisten(HandleDevice);
        }
    }
    else
    {
        returnValue = -1;
    }

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief Close a file on the IEC serial bus

 This function closes a file on the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param SecondaryAddress
   The secondary address for the device on the IEC serial bus.

 \return
   0 on success, else failure

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
cbm_close(CBM_FILE HandleDevice, unsigned char DeviceAddress, unsigned char SecondaryAddress)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_close(HandleDevice, DeviceAddress, SecondaryAddress));
}

/*! \brief Send an UNLISTEN on the IEC serial bus

 This function sends an UNLISTEN on the IEC serial bus.
 Other than LISTEN and TALK, an UNLISTEN is not directed
 to just one device, but to all devices on that IEC
 serial bus. 

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   0 on success, else failure

 At least on a 1541 floppy drive, an UNLISTEN also undoes
 a previous TALK.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
cbm_unlisten(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_unlisten(HandleDevice));
}

/*! \brief Send an UNTALK on the IEC serial bus

 This function sends an UNTALK on the IEC serial bus.
 Other than LISTEN and TALK, an UNTALK is not directed
 to just one device, but to all devices on that IEC
 serial bus. 

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   0 on success, else failure

 At least on a 1541 floppy drive, an UNTALK also undoes
 a previous LISTEN.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
cbm_untalk(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_untalk(HandleDevice));
}


/*! \brief Get EOI flag after bus read

 This function gets the EOI ("End of Information") flag 
 after reading the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 if EOI was signalled, else 0.

 If a previous read returned less than the specified number
 of bytes, there are two possible reasons: Either an error
 occurred on the IEC serial bus, or an EOI was signalled.
 To find out the cause, check with this function.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_get_eoi(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_get_eoi(HandleDevice));
}

/*! \brief Reset the EOI flag

 This function resets the EOI ("End of Information") flag
 which might be still set after reading the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   0 on success, != 0 means an error has occured.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_clear_eoi(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_clear_eoi(HandleDevice));
}

/*! \brief RESET all devices

 This function performs a hardware RESET of all devices on
 the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   0 on success, else failure

 Don't overuse this function! Normally, an initial RESET
 should be enough.

 Control is returned after a delay which ensures that all
 devices are ready again.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
cbm_reset(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_reset(HandleDevice));
}


/*-------------------------------------------------------------------*/
/*--------- LOW-LEVEL PORT ACCESS -----------------------------------*/

/*! \brief Read a byte from a XP1541/XP1571 cable

 This function reads a single byte from the parallel portion of 
 an XP1541/1571 cable.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   the byte which was received on the parallel port

 This function reads the current state of the port. No handshaking
 is performed at all.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

unsigned char CBMAPIDECL 
cbm_pp_read(CBM_FILE HandleDevice)
{
    unsigned char ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_pp_read)
        ret = Plugin_information.Plugin.opencbm_plugin_pp_read(HandleDevice);

    FUNC_LEAVE_UCHAR(ret);
}

/*! \brief Write a byte to a XP1541/XP1571 cable

 This function writes a single byte to the parallel portion of 
 a XP1541/1571 cable.

 \param HandleDevice

   A CBM_FILE which contains the file handle of the driver.

 \param Byte

   the byte to be output on the parallel port

 This function just writes on the port. No handshaking
 is performed at all.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

void CBMAPIDECL 
cbm_pp_write(CBM_FILE HandleDevice, unsigned char Byte)
{
    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_pp_write)
        Plugin_information.Plugin.opencbm_plugin_pp_write(HandleDevice, Byte);

    FUNC_LEAVE();
}

/*! \brief Read status of all bus lines.

 This function reads the state of all lines on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   The state of the lines. The result is an OR between
   the bit flags IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET.

 This function just reads the port. No handshaking
 is performed at all.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

int CBMAPIDECL
cbm_iec_poll(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_iec_poll(HandleDevice));
}


/*! \brief Activate a line on the IEC serial bus

 This function activates (sets to 0V) a line on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Line
   The line to be activated. This must be exactly one of
   IEC_DATA, IEC_CLOCK, IEC_ATN, or IEC_RESET.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

void CBMAPIDECL
cbm_iec_set(CBM_FILE HandleDevice, int Line)
{
    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_iec_set)
        Plugin_information.Plugin.opencbm_plugin_iec_set(HandleDevice, Line);
    else
        Plugin_information.Plugin.opencbm_plugin_iec_setrelease(HandleDevice, Line, 0);

    FUNC_LEAVE();
}

/*! \brief Deactivate a line on the IEC serial bus

 This function deactivates (sets to 5V) a line on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Line
   The line to be deactivated. This must be exactly one of
   IEC_DATA, IEC_CLOCK, IEC_ATN, or IEC_RESET.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

void CBMAPIDECL
cbm_iec_release(CBM_FILE HandleDevice, int Line)
{
    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_iec_release)
        Plugin_information.Plugin.opencbm_plugin_iec_release(HandleDevice, Line);
    else
        Plugin_information.Plugin.opencbm_plugin_iec_setrelease(HandleDevice, 0, Line);

    FUNC_LEAVE();
}

/*! \brief Activate and deactive a line on the IEC serial bus

 This function activates (sets to 0V, L) and deactivates 
 (set to 5V, H) lines on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Set
   The mask of which lines should be set. This has to be a bitwise OR
   between the constants IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET

 \param Release
   The mask of which lines should be released. This has to be a bitwise
   OR between the constants IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!

 \remark
   If a bit is specified in the Set as well as in the Release mask, the
   effect is undefined.
*/

void CBMAPIDECL
cbm_iec_setrelease(CBM_FILE HandleDevice, int Set, int Release)
{
    FUNC_ENTER();

    Plugin_information.Plugin.opencbm_plugin_iec_setrelease(HandleDevice, Set, Release);

    FUNC_LEAVE();
}

/*! \brief Wait for a line to have a specific state

 This function waits for a line to enter a specific state
 on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Line
   The line to be deactivated. This must be exactly one of
   IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET.

 \param State
   If zero, then wait for this line to be deactivated. \n
   If not zero, then wait for this line to be activated.

 \return
   The state of the IEC bus on return (like cbm_iec_poll).

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

int CBMAPIDECL
cbm_iec_wait(CBM_FILE HandleDevice, int Line, int State)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT(Plugin_information.Plugin.opencbm_plugin_iec_wait(HandleDevice, Line, State));
}

/*! \brief Get the (logical) state of a line on the IEC serial bus

 This function gets the (logical) state of a line on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Line
   The line to be tested. This must be exactly one of
   IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET.

 \return
   1 if the line is set, 0 if it is not

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

int CBMAPIDECL
cbm_iec_get(CBM_FILE HandleDevice, int Line)
{
    FUNC_ENTER();

    FUNC_LEAVE_INT((Plugin_information.Plugin.opencbm_plugin_iec_poll(HandleDevice)&Line) != 0 ? 1 : 0);
}


/*-------------------------------------------------------------------*/
/*--------- HELPER FUNCTIONS ----------------------------------------*/


/*! \brief Read the drive status from a floppy

 This function reads the drive status of a connected
 floppy drive.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param Buffer
   Pointer to a buffer which will hold the drive's status after
   successfull calling,

 \param BufferLength
   The length of the buffer pointed to by Buffer in bytes.

 \return
   Returns the int representation of the drive status,
   that is, the numerical value of the first return
   value from the drive. This is the error number.

 This function will never write more than BufferLength bytes.
 Nevertheless, the buffer will always be terminated with
 a trailing zero.

 If an error occurs, this function returns a
 "99, DRIVER ERROR,00,00\r" and the value 99.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
cbm_device_status(CBM_FILE HandleDevice, unsigned char DeviceAddress, 
                  void *Buffer, size_t BufferLength)
{
    int retValue;

    FUNC_ENTER();

    DBG_ASSERT(Buffer && (BufferLength > 0));

    // Pre-occupy return value

    retValue = 99;

    if (Buffer && (BufferLength > 0))
    {
        char *bufferToWrite = Buffer;

        // make sure we have a trailing zero at the end of the buffer:

        bufferToWrite[--BufferLength] = '\0';

        // pre-occupy buffer with the error value

        strncpy(bufferToWrite, "99, DRIVER ERROR,00,00\r", BufferLength);

        // Now, ask the drive for its error status:

        if (cbm_talk(HandleDevice, DeviceAddress, 15) == 0)
        {
            unsigned int bytesRead;
            
            bytesRead = cbm_raw_read(HandleDevice, bufferToWrite, BufferLength - 1);

            DBG_ASSERT(bytesRead <= BufferLength);

            // make sure we have a trailing zero at the end of the status:

            bufferToWrite[bytesRead] = '\0';

            cbm_untalk(HandleDevice);
        }

        retValue = atoi(bufferToWrite);
    }

    FUNC_LEAVE_INT(retValue);
}

/*! \brief Executes a command in the floppy drive.

 This function Executes a command in the connected floppy drive.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param Command
   Pointer to a string which holds the command to be executed.

 \param Size
   The length of the command in bytes. If zero, the Command
   has to be a null-terminated string.

 \return
   0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
cbm_exec_command(CBM_FILE HandleDevice, unsigned char DeviceAddress, 
                 const void *Command, size_t Size)
{
    int rv;

    FUNC_ENTER();
    rv = cbm_listen(HandleDevice, DeviceAddress, 15);
    if(rv == 0) {
        if(Size == 0) {
            Size = (size_t) strlen(Command);
        }
        rv = (size_t) cbm_raw_write(HandleDevice, Command, Size) != Size;
        cbm_unlisten(HandleDevice);
    }

    FUNC_LEAVE_INT(rv);
}

/*! \brief PARBURST: Read from the parallel port

 This function is a helper function for parallel burst:
 It reads from the parallel port.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   The value read from the parallel port

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

unsigned char CBMAPIDECL
cbm_parallel_burst_read(CBM_FILE HandleDevice)
{
    unsigned char ret = 0;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_parallel_burst_read)
        ret = Plugin_information.Plugin.opencbm_plugin_parallel_burst_read(HandleDevice);

    FUNC_LEAVE_UCHAR(ret);
}

/*! \brief PARBURST: Write to the parallel port

 This function is a helper function for parallel burst:
 It writes to the parallel port.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Value
   The value to be written to the parallel port

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

void CBMAPIDECL
cbm_parallel_burst_write(CBM_FILE HandleDevice, unsigned char Value)
{
    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_parallel_burst_write)
        Plugin_information.Plugin.opencbm_plugin_parallel_burst_write(HandleDevice, Value);

    FUNC_LEAVE();
}

int CBMAPIDECL
cbm_parallel_burst_read_n(CBM_FILE HandleDevice, unsigned char *Buffer,
    unsigned int Length)
{
    unsigned int i;
    int rv;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_parallel_burst_read_n) {
        rv = Plugin_information.Plugin.opencbm_plugin_parallel_burst_read_n(
            HandleDevice, Buffer, Length);
    } else {
        for (i = 0; i < Length; i++) {
            Buffer[i] = Plugin_information.Plugin
                .opencbm_plugin_parallel_burst_read(HandleDevice);
        }
        rv = Length;
    }

    FUNC_LEAVE_INT(rv);
}

int CBMAPIDECL
cbm_parallel_burst_write_n(CBM_FILE HandleDevice, unsigned char *Buffer,
    unsigned int Length)
{
    unsigned int i;
    int rv;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_parallel_burst_write_n) {
        rv = Plugin_information.Plugin.opencbm_plugin_parallel_burst_write_n(
            HandleDevice, Buffer, Length);
    } else {
        for (i = 0; i < Length; i++) {
            Plugin_information.Plugin.opencbm_plugin_parallel_burst_write(
                HandleDevice, Buffer[i]);
        }
        rv = Length;
    }

    FUNC_LEAVE_INT(rv);
}

/*! \brief PARBURST: Read a complete track

 This function is a helper function for parallel burst:
 It reads a complete track from the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which will hold the bytes read.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
 If this function is not implemented, it will return -1.
*/

int CBMAPIDECL
cbm_parallel_burst_read_track(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_parallel_burst_read_track)
        ret = Plugin_information.Plugin.opencbm_plugin_parallel_burst_read_track(HandleDevice, Buffer, Length);

    FUNC_LEAVE_INT(ret);
}

/*! \brief PARBURST: Read a variable length track

 This function is a helper function for parallel burst:
 It reads a variable length track from the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which will hold the bytes read.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
cbm_parallel_burst_read_track_var(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_parallel_burst_read_track)
        ret = Plugin_information.Plugin.opencbm_plugin_parallel_burst_read_track_var(HandleDevice, Buffer, Length);

    FUNC_LEAVE_INT(ret);
}

/*! \brief PARBURST: Write a complete track

 This function is a helper function for parallel burst:
 It writes a complete track to the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which hold the bytes to be written.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
 If this function is not implemented, it will return -1.
*/

int CBMAPIDECL
cbm_parallel_burst_write_track(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_parallel_burst_write_track)
        ret = Plugin_information.Plugin.opencbm_plugin_parallel_burst_write_track(HandleDevice, Buffer, Length);

    FUNC_LEAVE_INT(ret);
}

/*! \brief PARBURST: Read from the parallel port

 This function is a helper function for parallel burst:
 It reads from the parallel port.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   The value read from the parallel port

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

unsigned char CBMAPIDECL
cbm_srq_burst_read(CBM_FILE HandleDevice)
{
    unsigned char ret = 0;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_srq_burst_read)
        ret = Plugin_information.Plugin.opencbm_plugin_srq_burst_read(HandleDevice);

    FUNC_LEAVE_UCHAR(ret);
}

/*! \brief PARBURST: Write to the parallel port

 This function is a helper function for parallel burst:
 It writes to the parallel port.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Value
   The value to be written to the parallel port

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

void CBMAPIDECL
cbm_srq_burst_write(CBM_FILE HandleDevice, unsigned char Value)
{
    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_srq_burst_write)
        Plugin_information.Plugin.opencbm_plugin_srq_burst_write(HandleDevice, Value);

    FUNC_LEAVE();
}

int CBMAPIDECL
cbm_srq_burst_read_n(CBM_FILE HandleDevice, unsigned char *Buffer,
    unsigned int Length)
{
    unsigned int i;
    int rv;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_srq_burst_read_n) {
        rv = Plugin_information.Plugin.opencbm_plugin_srq_burst_read_n(
            HandleDevice, Buffer, Length);
    } else {
        for (i = 0; i < Length; i++) {
            Buffer[i] = Plugin_information.Plugin
                .opencbm_plugin_srq_burst_read(HandleDevice);
        }
        rv = Length;
    }

    FUNC_LEAVE_INT(rv);
}

int CBMAPIDECL
cbm_srq_burst_write_n(CBM_FILE HandleDevice, unsigned char *Buffer,
    unsigned int Length)
{
    unsigned int i;
    int rv;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_srq_burst_write_n) {
        rv = Plugin_information.Plugin.opencbm_plugin_srq_burst_write_n(
            HandleDevice, Buffer, Length);
    } else {
        for (i = 0; i < Length; i++) {
            Plugin_information.Plugin.opencbm_plugin_srq_burst_write(
                HandleDevice, Buffer[i]);
        }
        rv = Length;
    }

    FUNC_LEAVE_INT(rv);
}

/*! \brief PARBURST: Read a complete track

 This function is a helper function for parallel burst:
 It reads a complete track from the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which will hold the bytes read.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
 If this function is not implemented, it will return -1.
*/

int CBMAPIDECL
cbm_srq_burst_read_track(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_srq_burst_read_track)
        ret = Plugin_information.Plugin.opencbm_plugin_srq_burst_read_track(HandleDevice, Buffer, Length);

    FUNC_LEAVE_INT(ret);
}

/*! \brief PARBURST: Write a complete track

 This function is a helper function for parallel burst:
 It writes a complete track to the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which hold the bytes to be written.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
 If this function is not implemented, it will return -1.
*/

int CBMAPIDECL
cbm_srq_burst_write_track(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_srq_burst_write_track)
        ret = Plugin_information.Plugin.opencbm_plugin_srq_burst_write_track(HandleDevice, Buffer, Length);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Prepare capture

 This function is a helper function for tape:
 It prepares the ZoomFloppy hardware for tape capture.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_prepare_capture(CBM_FILE HandleDevice, int *Status)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_prepare_capture)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_prepare_capture(HandleDevice, Status);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Prepare write

 This function is a helper function for tape:
 It prepares the ZoomFloppy hardware for tape write.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_prepare_write(CBM_FILE HandleDevice, int *Status)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_prepare_write)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_prepare_write(HandleDevice, Status);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Get tape sense

 This function is a helper function for tape:
 It returns the current tape sense state.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   The tape sense state

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_get_sense(CBM_FILE HandleDevice, int *Status)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_get_sense)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_get_sense(HandleDevice, Status);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Wait for <STOP> sense

 This function is a helper function for tape:
 It waits until the user stops the tape.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_wait_for_stop_sense(CBM_FILE HandleDevice, int *Status)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_wait_for_stop_sense)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_wait_for_stop_sense(HandleDevice, Status);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Wait for <PLAY> sense

 This function is a helper function for tape:
 It waits until the user presses play on tape.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_wait_for_play_sense(CBM_FILE HandleDevice, int *Status)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_wait_for_play_sense)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_wait_for_play_sense(HandleDevice, Status);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Motor on

 This function is a helper function for tape:
 It turns the tape drive motor on.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_motor_on(CBM_FILE HandleDevice, int *Status)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_motor_on)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_motor_on(HandleDevice, Status);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Motor off

 This function is a helper function for tape:
 It turns the tape drive motor off.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_motor_off(CBM_FILE HandleDevice, int *Status)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_motor_off)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_motor_off(HandleDevice, Status);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Start capture

 This function is a helper function for tape:
 It starts the actual tape capture.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which holds the bytes that are read.

 \param Buffer_Length
   The length of the Buffer.

 \param Status
   The return status.

 \param BytesRead
   The number of bytes read.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_start_capture(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Buffer_Length, int *Status, int *BytesRead)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_start_capture)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_start_capture(HandleDevice, Buffer, Buffer_Length, Status, BytesRead);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Start write

 This function is a helper function for tape:
 It starts the actual tape write.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which holds the bytes to be written.

 \param Length
   The number of bytes to write.

 \param Status
   The return status.

 \param BytesWritten
   The number of bytes written.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_start_write(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length, int *Status, int *BytesWritten)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_start_write)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_start_write(HandleDevice, Buffer, Length, Status, BytesWritten);

    FUNC_LEAVE_INT(ret);
}


/*! \brief TAPE: Return tape firmware version

 This function is a helper function for tape:
 It returns the tape firmware version.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_get_ver(CBM_FILE HandleDevice, int *Status)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_get_ver)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_get_ver(HandleDevice, Status);

    FUNC_LEAVE_INT(ret);
}


int CBMAPIDECL
cbm_tap_break(CBM_FILE HandleDevice)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_break)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_break(HandleDevice);

    FUNC_LEAVE_INT(ret);
}


/*! \brief TAPE: Download configuration

 This function is a helper function for tape:
 It reads the active configuration.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which holds the bytes that are read.

 \param Buffer_Length
   The length of the Buffer.

 \param Status
   The return status.

 \param BytesRead
   The number of bytes read.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_download_config(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Buffer_Length, int *Status, int *BytesRead)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_download_config)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_download_config(HandleDevice, Buffer, Buffer_Length, Status, BytesRead);

    FUNC_LEAVE_INT(ret);
}

/*! \brief TAPE: Upload configuration

 This function is a helper function for tape:
 It writes the active configuration.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which holds the bytes to be written.

 \param Length
   The number of bytes to write.

 \param Status
   The return status.

 \param BytesWritten
   The number of bytes written.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
cbm_tap_upload_config(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length, int *Status, int *BytesWritten)
{
    int ret = -1;

    FUNC_ENTER();

    if (Plugin_information.Plugin.opencbm_plugin_tap_upload_config)
        ret = Plugin_information.Plugin.opencbm_plugin_tap_upload_config(HandleDevice, Buffer, Length, Status, BytesWritten);

    FUNC_LEAVE_INT(ret);
}


/*! \brief Get the function pointer for a function in a plugin

 This function gets the function pointer for a function which 
 resides in the plugin.

 \param Functionname
   The name of the function of which to get the address

 \return
   Pointer to the function if successfull; 0 if not.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/


void * CBMAPIDECL
cbm_get_plugin_function_address(const char * Functionname)
{
    void * pointer = NULL;

    FUNC_ENTER();

    if (Plugin_information.Library)
        pointer = plugin_get_address(Plugin_information.Library, Functionname);

    FUNC_LEAVE_PTR(pointer, void*);
}

/*! \brief Read a byte from the parallel port input register

 This function reads a byte from the parallel port input register.
 (STATUS_PORT). It is a helper function for debugging the cable
 (i.e., for the XCDETECT tool) only!

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   If the routine succeeds, it returns a non-negative value
   which corresponds to the data in the parallel port input
   register (status port).

   If the routine fails, the return value is -1.

 \remark
   Do not use this function in anything but a debugging aid tool
   like XCDETECT!

   This functions masks some bits off. The bits that are not masked
   off are defined in PARALLEL_STATUS_PORT_MASK_VALUES.
*/
int CBMAPIDECL
cbm_iec_dbg_read(CBM_FILE HandleDevice)
{
    int returnValue = -1;

    FUNC_ENTER();

    if ( Plugin_information.Plugin.opencbm_plugin_iec_dbg_read ) {
        returnValue = Plugin_information.Plugin.opencbm_plugin_iec_dbg_read(HandleDevice);
    }

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief Write a byte to the parallel port output register

 This function writes a byte to the parallel port output register.
 (CONTROL_PORT). It is a helper function for debugging the cable
 (i.e., for the XCDETECT tool) only!

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Value
   The value to set the control port to

 \return 
   If the routine succeeds, it returns 0.
   
   If the routine fails, it returns -1.

 \remark
   Do not use this function in anything but a debugging aid tool
   like XCDETECT!

   After this function has been called, it is NOT safe to use the
   parallel port again unless you close the driver (cbm_driver_close())
   and open it again (cbm_driver_open())!

   This functions masks some bits off. That is, the bits not in the
   mask are not changed at all. The bits that are not masked
   off are defined in PARALLEL_CONTROL_PORT_MASK_VALUES.
*/
int CBMAPIDECL
cbm_iec_dbg_write(CBM_FILE HandleDevice, unsigned char Value)
{
    int returnValue = -1;

    FUNC_ENTER();

    if ( Plugin_information.Plugin.opencbm_plugin_iec_dbg_write ) {
        returnValue = Plugin_information.Plugin.opencbm_plugin_iec_dbg_write(HandleDevice, Value);
    }

    FUNC_LEAVE_INT(returnValue);
}
