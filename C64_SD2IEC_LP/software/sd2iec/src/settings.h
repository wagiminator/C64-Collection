// ===================================================================================
// Settings in the EEPROM                                                     * v1.0 *
// ===================================================================================
//
// Functions available:
// --------------------
// settings_load()          Load the stored settings (or the defaults from config.h)
// settings_save()          Store device address, extension mode and JiffyDOS (XW)
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void settings_load(void);
void settings_save(void);

#ifdef __cplusplus
};
#endif
