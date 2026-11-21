// ===================================================================================
// Disk Operating System: Disk Swap                                           * v1.0 *
// ===================================================================================
//
// Games on several disks ask for the next disk. The NEXT and PREV buttons change
// the mounted disk image without touching the computer:
//
// - With a swap list (set with XS:name, or AUTOSWAP.LST in the current directory)
//   the buttons step through the images named in the list. Lines starting with ";"
//   are comments; a first line "#PETSCII" marks a list written in PETSCII.
// - Without a swap list the buttons step through all disk images in the directory
//   of the mounted image (in directory order).
// - NEXT + PREV together go back to the first image.
//
// The LEDs blink to confirm the change (BUSY: next, ERROR: previous, both: first).
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include <avr/pgmspace.h>
#include "system.h"
#include "dos.h"
#include "led.h"

static FAT_file swap_list;              // the swap list file
static uint8_t  swap_active;            // swap list is open
static uint32_t swap_dir;               // directory of the list (names are relative)
static uint8_t  swap_line;              // current entry
static uint8_t  swap_index;             // folder mode: number of the image

// Blink to confirm a disk change
static void confirm_blink(uint8_t key) {
  uint8_t i;
  for(i = 0; i < 2; i++) {
    if(!i || (key & KEY_NEXT)) LED_busy(1);
    if(!i || (key & KEY_PREV)) LED_dirty(1);
    DLY_ms(100);
    LED_busy(0);
    LED_dirty(0);
    DLY_ms(100);
  }
}

// Read entry n of the list into command_buffer (wraps around; 255: last entry),
// returns 1 if found
static uint8_t read_line(uint8_t* n) {
  uint8_t c, len, count = 0, petscii = 0, first = 1, comment;
  uint8_t* p;
  if(FAT_seek(&swap_list, 0)) return 0;
  while(1) {
    // read one line
    len = 0; comment = 0;
    while(FAT_read(&swap_list, &c, 1) == 1 && c != 13 && c != 10) {
      if(!len && c == ';') comment = 1;
      if(len < COMMAND_LENGTH - 1) command_buffer[1 + len++] = c;
    }
    command_buffer[1 + len] = 0;
    if(first && !memcmp_P(command_buffer + 1, PSTR("#PETSCII"), 8)) { petscii = 1; comment = 1; }
    first = 0;
    if(len && !comment) {
      if(count == *n) break;
      count++;
    }
    if(swap_list.pos >= swap_list.size) {       // end of the list
      if(!count) return 0;
      *n = (*n == 255) ? count - 1 : 0;         // last entry / wrap to the first
      FAT_seek(&swap_list, 0);
      count = 0; first = 1;
    }
  }
  if(!petscii) asc2pet(command_buffer + 1);
  for(p = command_buffer + 1; *p; p++) if(*p == 9) *p = ' ';
  return 1;
}

// Mount the entry swap_line of the list, returns 1 if successful
static uint8_t mount_line(void) {
  uint8_t* name = command_buffer + 1;
  buffers_free(1);
  if(!read_line(&swap_line)) return 0;
  if(image_mounted) image_unmount();
  current_path.image = 0;
  current_path.dir = swap_dir;
  if(!strchr((char*)name, ':') && *name != '/') *--name = ':';
  set_error(ERROR_OK);
  do_chdir(name);
  return current_error == ERROR_OK;
}

// Set a swap list (name in the current directory), 0 removes it
void swap_set(uint8_t* name) {
  dirent_t dent;
  swap_active = 0;
  if(!name || !*name) return;
  if(current_path.image) { set_error(ERROR_SYNTAX_UNABLE); return; }
  if(fs_first_match(&current_path, name, 0, &dent)) return;
  FAT_open(&swap_list, &dent.pvt.fat);
  swap_dir = current_path.dir;
  swap_active = 1;
  swap_line = 0;
  if(mount_line()) confirm_blink(KEY_HOME);
}

// Forget the swap list (card changed)
void swap_reset(void) {
  swap_active = 0;
  swap_index = 0;
}

// Folder mode: mount image number n of the directory (wraps, 255: last one)
static uint8_t mount_folder(uint8_t n) {
  dirhandle_t dh;
  dirent_t dent;
  path_t path;
  uint8_t count = 0;

  path.image = 0;
  path.dir = current_path.dir;
  while(1) {
    if(fs_opendir(&dh, &path)) return 0;
    while(fs_readdir(&dh, &dent) == 0) {
      if((dent.type & TYPE_MASK) == TYPE_DIR || !check_imageext((uint8_t*)dent.pvt.fat.name)) continue;
      if(count++ == n) {
        buffers_free(1);
        image_unmount();
        current_path.image = 0;
        if(fs_chdir(&current_path, &dent)) return 0;
        swap_index = n;
        return 1;
      }
    }
    if(!count) return 0;                        // no images at all
    n = (n == 255) ? count - 1 : 0;             // last one / wrap around
    count = 0;
  }
}

// Button pressed
void swap_change(uint8_t key) {
  if(!disk_ready) return;
  if(!swap_active && !current_path.image) {
    // try AUTOSWAP.LST in the current directory
    dirent_t dent;
    uint8_t name[sizeof(SWAP_LIST_NAME)];
    memcpy_P(name, PSTR(SWAP_LIST_NAME), sizeof(SWAP_LIST_NAME));
    if(!fs_first_match(&current_path, name, 0, &dent)) {
      FAT_open(&swap_list, &dent.pvt.fat);
      swap_dir = current_path.dir;
      swap_active = 1;
      swap_line = (key == KEY_PREV) ? 255 : 0;
      if(mount_line()) confirm_blink(KEY_HOME);
      return;
    }
    set_error(ERROR_OK);
  }

  if(swap_active) {
    if(key == KEY_NEXT) swap_line++;
    else if(key == KEY_PREV) swap_line--;
    else swap_line = 0;
    if(mount_line()) confirm_blink(key);
    return;
  }

  // folder mode: the images in the directory of the mounted image (or the current
  // directory if no image is mounted)
  {
    uint8_t n = swap_index;
    if(!image_mounted) n = (key == KEY_PREV) ? 255 : 0;
    else if(key == KEY_NEXT) n++;
    else if(key == KEY_PREV) n = n ? n - 1 : 255;
    else n = 0;
    if(mount_folder(n)) confirm_blink(key);
    set_error(ERROR_OK);
  }
}
