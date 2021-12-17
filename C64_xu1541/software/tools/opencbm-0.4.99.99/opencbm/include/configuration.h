/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2007      Spiro Trikaliotis
 *
*/

/*! **************************************************************
** \file include/configuration.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**        Read configuration file
**
****************************************************************/

struct opencbm_configuration_s;

/*! \brief Handle to a configuration file */
typedef struct opencbm_configuration_s * opencbm_configuration_handle;

/*! \brief Callback type for section enumeration

 This function type is used as the callback function
 in opencbm_configuration_enum_sections()

 \param Handle
   Handle to the configuration file

 \param Section
   Name of the enumerated section

 \param Context
   Context data as given in the call to
   opencbm_configuration_enum_sections().
   This data is not interpreted, but forwarded
   "as is".

 \return
   0 on success; everything else indicates an error.
*/
typedef int opencbm_configuration_enum_sections_callback_t(opencbm_configuration_handle Handle,
                                    const char Section[],
                                    void * Context);

/*! \brief Callback type for entry enumeration

 This function type is used as the callback function
 in opencbm_configuration_enum_data()

 \param Handle
   Handle to the configuration file

 \param Section
   Name of the section

 \param Entry
   Name of the enumerated entry

 \param Context
   Context data as given in the call to
   opencbm_configuration_enum_data().
   This data is not interpreted, but forwarded
   "as is".

 \return
   0 on success; everything else indicates an error.
*/
typedef int  opencbm_configuration_enum_data_callback_t(opencbm_configuration_handle Handle,
                                    const char Section[],
                                    const char Entry[],
                                    void * Context);

extern const char *configuration_get_default_filename(void);
extern const char *configuration_get_default_filename_for_install(unsigned int local_install);

extern opencbm_configuration_handle opencbm_configuration_open(const char * Filename);
extern opencbm_configuration_handle opencbm_configuration_create(const char * Filename);
extern int                          opencbm_configuration_close(opencbm_configuration_handle Handle);
extern int                          opencbm_configuration_flush(opencbm_configuration_handle Handle);
extern int                          opencbm_configuration_get_data(opencbm_configuration_handle Handle, const char Section[], const char Entry[], char ** ReturnBuffer);
extern int                          opencbm_configuration_set_data(opencbm_configuration_handle Handle, const char Section[], const char Entry[], const char Value[]);
extern int                          opencbm_configuration_enum_sections(opencbm_configuration_handle Handle, opencbm_configuration_enum_sections_callback_t Callback, void * Context);
extern int                          opencbm_configuration_enum_data(opencbm_configuration_handle Handle, const char Section[], opencbm_configuration_enum_data_callback_t Callback, void * Context);
