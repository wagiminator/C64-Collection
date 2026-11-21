// ===================================================================================
// Settings in the EEPROM                                                     * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include "system.h"
#include "settings.h"
#include "dos.h"
#include "iec.h"

#define SET_MAGIC       0x5D            // marks valid settings

typedef struct {
  uint8_t magic;
  uint8_t address;                      // device address (0: DIP switches)
  uint8_t ext_mode;
  uint8_t ext_hiding;
  uint8_t jiffy;
} settings_t;

void settings_load(void) {
  settings_t s;
  EEPROM_readBlock(&s, 0, sizeof(s));
  ext_mode   = DEF_EXT_MODE;
  ext_hiding = DEF_EXT_HIDING;
  if(s.magic != SET_MAGIC) return;
  if(s.address >= 4 && s.address <= 30) device_address = s.address;
  if(s.ext_mode <= 4) ext_mode = s.ext_mode;
  ext_hiding    = s.ext_hiding & 1;
#if FL_JIFFYDOS
  jiffy_enabled = s.jiffy & 1;
#endif
}

void settings_save(void) {
  settings_t s;
  s.magic      = SET_MAGIC;
  s.address    = (device_address == device_dip_address()) ? 0 : device_address;
  s.ext_mode   = ext_mode;
  s.ext_hiding = ext_hiding;
  s.jiffy      = jiffy_enabled;
  EEPROM_writeBlock(&s, 0, sizeof(s));
}
