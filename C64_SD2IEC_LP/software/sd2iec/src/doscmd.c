// ===================================================================================
// Disk Operating System: Commands (Channel 15)                               * v1.0 *
// ===================================================================================
//
// Commands available (see README for details):
// --------------------------------------------
// I                        initialize (close all channels)
// V                        validate (nothing to do)
// N:name[,id]              format the mounted disk image
// S:pattern[,pattern..]    scratch files
// R:new=old                rename a file
// C:new=old[,old..]        copy (concatenate) files
// CD:dir / CD:image.d64    change directory / mount disk image
// CD:_ (left arrow), CD//  parent directory, root directory
// MD:dir, RD:dir           make / remove directory
// U1/UA, U2/UB             block read / block write (disk images)
// B-R, B-W, B-P, B-A, B-F  block commands
// U3..U8, M-E              execute drive code (fast loader detection)
// M-R, M-W                 memory read / write (drive detection, fast loaders)
// UI, UJ, U:               reset (UI-/UI+: VIC-20 / C64 timing)
// U0>n, M-W 119            change the device address
// P ch lo hi ..            position in a file
// XE0..4, XE+/-            extension mode, extension hiding
// XJ+/-                    JiffyDOS on/off
// XS:name, XS              set / clear the disk swap list
// XW                       store the settings in the EEPROM
// X?, X                    version, settings
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include <avr/pgmspace.h>
#include "dos.h"
#include "iec.h"
#include "fastloader.h"
#include "settings.h"
#include "led.h"

uint8_t command_buffer[COMMAND_LENGTH + 2];
uint8_t command_length;

// Terminate the command and clear the rest of the buffer
static void clean_cmdbuffer(void) {
  memset(command_buffer + command_length, 0, sizeof(command_buffer) - command_length);
}

// Parse a decimal number, moves the pointer
static uint16_t parse_number(uint8_t** str) {
  uint16_t n = 0;
  while(**str >= '0' && **str <= '9') n = n * 10 + *(*str)++ - '0';
  return n;
}

// Parse up to 4 parameters of a block command ("B-R:2 0 18 0" or binary after U1),
// returns the number of parameters or -1
static int8_t parse_blockparam(uint8_t* values) {
  uint8_t* str = (uint8_t*)strchr((char*)command_buffer, ':');
  uint8_t n = 0;
  if(!str) {
    if(command_length < 3) { set_error(ERROR_SYNTAX_UNABLE); return -1; }
    str = command_buffer + 3;
  }
  else str++;
  while(*str && n < 4) {
    while(*str == ' ' || *str == 0x1D || *str == ',') str++;   // separators
    if(!*str) break;
    values[n++] = parse_number(&str);
    while(*str && *str != ' ' && *str != 0x1D && *str != ',') str++;
  }
  return n;
}

// "+" / "-" parameter: 1, 0 or 255 (error)
static uint8_t parse_bool(void) {
  if(command_buffer[2] == '+') return 1;
  if(command_buffer[2] == '-') return 0;
  set_error(ERROR_SYNTAX_UNKNOWN);
  return 255;
}

// Extended status: "03,En+,J+,dd,00" (extension mode/hiding, JiffyDOS, address)
static void show_status(void) {
  buffer_t* b = &buffers[BUFFER_COUNT];
  uint8_t* p = b->data;
  set_error_ts(3, device_address, 0);           // sets the buffer up
  p[3] = 'E'; p[4] = '0' + ext_mode; p[5] = ext_hiding ? '+' : '-';
  p[6] = 'J'; p[7] = jiffy_enabled ? '+' : '-';
  p[8] = ',';
  p[9]  = '0' + device_address / 10;
  p[10] = '0' + device_address % 10;
  memcpy_P(p + 11, PSTR(",00\r"), 4);
  b->lastused = 14;
}

// ===================================================================================
// Directories and Files
// ===================================================================================

// Change directory or mount an image ("CD" parameter string)
void do_chdir(uint8_t* str) {
  path_t path;
  dirent_t dent;
  uint8_t* name;
  if(parse_path(str, &path, &name, 1)) return;
  if(name[0] == 0x5F || (name[0] == '_' && !name[1])) {      // left arrow: parent
    dent.name[0] = '_'; dent.name[1] = 0;
    if(!fs_chdir(&path, &dent)) current_path = path;
    if(!path.image && image_mounted) image_unmount();
    return;
  }
  if(!*name) {                                  // path only
    if(!path.image && image_mounted) image_unmount();
    current_path = path;
    return;
  }
  if(fs_first_match(&path, name, FLAG_HIDDEN, &dent)) return;
  if(path.image) { set_error(ERROR_FILE_NOT_FOUND_39); return; }
  if(image_mounted) image_unmount();
  if(!fs_chdir(&path, &dent)) current_path = path;
}

// CD: change directory or mount an image
static void parse_chdir(void) {
  clean_cmdbuffer();
  do_chdir(command_buffer + 2);
}

// MD: make directory
static void parse_mkdir(void) {
  path_t path;
  uint8_t* name;
  clean_cmdbuffer();
  if(parse_path(command_buffer + 2, &path, &name, 0)) return;
  if(!*name) { set_error(ERROR_SYNTAX_NONAME); return; }
  fs_mkdir(&path, name);
}

// RD: remove directory
static void parse_rmdir(void) {
  path_t path;
  dirent_t dent;
  uint8_t* name;
  clean_cmdbuffer();
  if(parse_path(command_buffer + 2, &path, &name, 0)) return;
  if(path.image) { set_error(ERROR_SYNTAX_UNABLE); return; }
  if(fs_first_match(&path, name, TYPE_DIR | FLAG_HIDDEN, &dent)) return;
  switch(fs_delete(&path, &dent)) {
    case 0:   set_error(ERROR_FILE_EXISTS); break;   // not empty
    case 1:   set_error_ts(ERROR_SCRATCHED, 1, 0); break;
  }
}

// S: scratch files
static void parse_scratch(void) {
  path_t path;
  dirent_t dent;
  uint8_t *name, *next, count = 0;
  clean_cmdbuffer();
  name = (uint8_t*)strchr((char*)command_buffer, ':');
  if(!name) { set_error(ERROR_SYNTAX_NONAME); return; }
  name = command_buffer + 1;
  while(*name) {
    next = (uint8_t*)strchr((char*)name, ',');
    if(next) *next = 0;
    if(parse_path(name, &path, &name, 0)) return;
    if(fs_opendir(&matchdh, &path)) return;
    while(fs_next_match(&matchdh, name, 0, &dent) == 0) {
      if((dent.type & TYPE_MASK) == TYPE_DIR || (dent.type & FLAG_RO)) continue;
      switch(fs_delete(&path, &dent)) {
        case 1: count++; break;
        case 255: return;
      }
      if(fs_opendir(&matchdh, &path)) return;   // start again (directory changed)
    }
    if(!next) break;
    name = next + 1;
  }
  set_error_ts(ERROR_SCRATCHED, count, 0);
}

// R: rename (R:new=old)
static void parse_rename(void) {
  path_t oldpath, newpath;
  dirent_t dent;
  uint8_t *oldname, *newname, *eq;
  clean_cmdbuffer();
  eq = (uint8_t*)strchr((char*)command_buffer, '=');
  if(!eq || !strchr((char*)command_buffer, ':')) { set_error(ERROR_SYNTAX_NONAME); return; }
  *eq = 0;
  if(parse_path(eq + 1, &oldpath, &oldname, 0) || parse_path(command_buffer + 1, &newpath, &newname, 0)) return;
  if(strchr((char*)newname, '*') || strchr((char*)newname, '?')) { set_error(ERROR_SYNTAX_JOKER); return; }
  if(fs_first_match(&newpath, newname, FLAG_HIDDEN, &dent) == 0) { set_error(ERROR_FILE_EXISTS); return; }
  set_error(ERROR_OK);
  if(fs_first_match(&oldpath, oldname, FLAG_HIDDEN, &dent)) return;
  fs_rename(&oldpath, &dent, newname);
}

// C: copy (C:new=old1,old2,..)
static void parse_copy(void) {
  path_t path;
  dirent_t dent;
  buffer_t *src, *dst;
  uint8_t *name, *next, *eq, type = 0;
  clean_cmdbuffer();
  eq = (uint8_t*)strchr((char*)command_buffer, '=');
  if(!eq) { set_error(ERROR_SYNTAX_NONAME); return; }
  *eq = 0;

  // source files must exist, the destination must not
  if(parse_path(command_buffer + 1, &path, &name, 0)) return;
  if(fs_first_match(&path, name, FLAG_HIDDEN, &dent) == 0) { set_error(ERROR_FILE_EXISTS); return; }
  set_error(ERROR_OK);
  if(!(dst = buffer_alloc())) return;
  if(!(src = buffer_alloc())) { buffer_free(dst); return; }

  name = eq + 1;
  while(1) {
    next = (uint8_t*)strchr((char*)name, ',');
    if(next) *next = 0;
    {
      path_t spath;
      dirent_t sdent;
      uint8_t* sname;
      if(parse_path(name, &spath, &sname, 0) || fs_first_match(&spath, sname, FLAG_HIDDEN, &sdent)) break;
      if(!type) {                               // open the destination with the type
        type = sdent.type & TYPE_MASK;          // of the first source
        parse_path(command_buffer + 1, &path, &name, 0);
        memset(&dent, 0, sizeof(dent));
        strncpy((char*)dent.name, (char*)name, CBM_NAME_LENGTH);
        fs_open_write(&path, &dent, type, dst, 0);
        if(!dst->write) break;
      }
      fs_open_read(&spath, &sdent, src);
      while(src->read) {
        while(src->position <= src->lastused) {
          dst->data[dst->position++] = src->data[src->position];
          dst->dirty = 1;
          if(!dst->position) { dst->mustflush = 1; if(dst->refill(dst)) goto end; }
          if(src->position++ == 255) break;
        }
        if(src->sendeoi) break;
        if(src->refill(src)) goto end;
      }
      src->cleanup(src);
      src->cleanup = buffer_dummy;
    }
    if(!next) break;
    name = next + 1;
  }
end:
  buffer_free(src);
  buffer_free(dst);
}

// N: format (only disk images)
static void parse_new(void) {
  uint8_t *name, *id;
  clean_cmdbuffer();
  name = (uint8_t*)strchr((char*)command_buffer, ':');
  if(!name) { set_error(ERROR_SYNTAX_NONAME); return; }
  name++;
  id = (uint8_t*)strchr((char*)name, ',');
  if(id) *id++ = 0;
  buffers_free(1);
  fs_format(name, id);
}

// P: position (P ch lo hi b3 b4)
static void parse_position(void) {
  buffer_t* buf;
  uint32_t pos;
  if(command_length < 4) { set_error(ERROR_SYNTAX_UNABLE); return; }
  buf = buffer_find(command_buffer[1] & 0x0F);
  if(!buf) { set_error(ERROR_FILE_NOT_OPEN); return; }
  if(buf->refill == directbuffer_refill) { buf->position = command_buffer[2]; return; }
  pos = command_buffer[2] | ((uint32_t)command_buffer[3] << 8);
  if(command_length > 4) pos |= (uint32_t)command_buffer[4] << 16;
  if(command_length > 5) pos |= (uint32_t)command_buffer[5] << 24;
  if(fs_seek(buf, pos)) set_error(ERROR_RECORD_MISSING);
}

// ===================================================================================
// Block and User Commands
// ===================================================================================

// B-R, B-W, B-P, B-A, B-F (also used by U1/U2)
static void parse_block(void) {
  uint8_t* str = (uint8_t*)strchr((char*)command_buffer, '-');
  uint8_t params[4];
  buffer_t* buf;
  clean_cmdbuffer();
  if(!str) { set_error(ERROR_SYNTAX_UNABLE); return; }
  memset(params, 0, sizeof(params));
  if(parse_blockparam(params) < 0) return;

  switch(str[1]) {
    case 'R':
    case 'W':
      if(!(buf = buffer_find(params[0]))) { set_error(ERROR_NO_CHANNEL); return; }
      if(str[1] == 'R') {
        fs_read_sector(buf, params[2], params[3]);
        if(command_buffer[0] == 'B') { buf->position = 1; buf->lastused = buf->data[0]; }
        else { buf->position = 0; buf->lastused = 255; }
      }
      else {
        if(command_buffer[0] == 'B') buf->data[0] = buf->position - 1;
        fs_write_sector(buf, params[2], params[3]);
      }
      break;
    case 'P':
      if(!(buf = buffer_find(params[0]))) { set_error(ERROR_NO_CHANNEL); return; }
      buf->position = params[1];
      break;
    case 'A':
    case 'F':
      if(!image_mounted) { set_error(ERROR_SYNTAX_UNABLE); return; }
      image_block_alloc(params[1], params[2], str[1] == 'A');
      break;
    default:
      set_error(ERROR_SYNTAX_UNABLE);
  }
}

// U commands
static void parse_user(void) {
  switch(command_buffer[1]) {
    case 'A': case '1':                         // U1: block read
      command_buffer[0] = '-'; command_buffer[1] = 'R';
      parse_block();
      break;
    case 'B': case '2':                         // U2: block write
      command_buffer[0] = '-'; command_buffer[1] = 'W';
      parse_block();
      break;
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case '3': case '4': case '5': case '6': case '7': case '8':
      fl_execute(0x500 + 3 * ((command_buffer[1] - '3') & 7));   // jump table at $0500
      break;
    case 'I': case '9':                         // soft reset
      if(command_length > 2 && command_buffer[2] == '+') vc20mode = 0;
      else if(command_length > 2 && command_buffer[2] == '-') vc20mode = 1;
      set_error(ERROR_DOSVERSION);
      break;
    case 'J': case ':':                         // reset
      buffers_free(1);
      set_error(ERROR_DOSVERSION);
      break;
    case '0':                                   // U0>n: device address
      if((command_buffer[2] & 0x1F) == 0x1E && command_buffer[3] >= 4 && command_buffer[3] <= 30) {
        device_address = command_buffer[3];
        break;
      }
      // fall through
    default:
      set_error(ERROR_SYNTAX_UNKNOWN);
  }
}

// M-R, M-W, M-E
static void parse_memory(void) {
  uint16_t address = command_buffer[3] | (command_buffer[4] << 8);
  if(command_length < 5) { set_error(ERROR_SYNTAX_UNKNOWN); return; }
  switch(command_buffer[2]) {
    case 'R': {
      buffer_t* b = &buffers[BUFFER_COUNT];
      uint8_t n = command_length > 5 ? command_buffer[5] : 1;
      if(!n) n = 1;
      if(n > 40) n = 40;
      memset(b->data, 0, n);
      fl_memread(address, b->data, n);
      b->position = 0;
      b->lastused = n - 1;
      b->read = 1;
      b->sendeoi = 1;
      break;
    }
    case 'W':
      if(command_length < 6) return;
      if(address == 119) { device_address = command_buffer[6] & 0x1F; return; }
      fl_memwrite(address, command_buffer + 6, command_buffer[5]);
      break;
    case 'E':
      fl_execute(address);
      break;
    default:
      set_error(ERROR_SYNTAX_UNKNOWN);
  }
}

// X commands
static void parse_xcommand(void) {
  uint8_t num, *str;
  path_t path;
  clean_cmdbuffer();
  switch(command_buffer[1]) {
    case 'E':                                   // extension mode
      str = command_buffer + 2;
      if(*str == '+') ext_hiding = 1;
      else if(*str == '-') ext_hiding = 0;
      else {
        num = parse_number(&str);
        if(num > 4) { set_error(ERROR_SYNTAX_UNKNOWN); return; }
        ext_mode = num;
        if(num >= 3) ext_hiding = 1;
      }
      show_status();
      break;
    case 'J':                                   // JiffyDOS on/off
      num = parse_bool();
      if(num != 255) { jiffy_enabled = num; show_status(); }
      break;
    case 'S':                                   // swap list
      if(command_buffer[2] != ':') { swap_set(0); break; }
      if(parse_path(command_buffer + 2, &path, &str, 0)) return;
      {
        path_t saved = current_path;
        current_path = path;
        swap_set(str);
        current_path = saved;
      }
      break;
    case 'W':                                   // store settings
      settings_save();
      show_status();
      break;
    case '?':
      set_error(ERROR_LONGVERSION);
      break;
    default:
      if(command_length == 1) show_status();
      else set_error(ERROR_SYNTAX_UNKNOWN);
  }
}

// ===================================================================================
// Command Parser
// ===================================================================================
void parse_doscommand(void) {
  set_error(ERROR_OK);

  // Strip CR at the end
  if(command_length && command_buffer[command_length - 1] == 13) command_length--;
  if(!command_length) return;

  // Commands that need no card
  switch(command_buffer[0]) {
    case 'U': parse_user(); return;
    case 'X': parse_xcommand(); return;
    case 'M':
      if(command_buffer[1] == '-') { parse_memory(); return; }
      if(command_buffer[1] == 'D') break;
      set_error(ERROR_SYNTAX_UNKNOWN);
      return;
  }

  if(!disk_ready && command_buffer[0] != 'I') { set_error(ERROR_DRIVE_NOT_READY); return; }

  switch(command_buffer[0]) {
    case 'I':
      buffers_free(1);
      if(disk_changed || !disk_ready) fs_init();
      disk_changed = 0;
      if(!disk_ready) set_error_ts(ERROR_READ_NOSYNC, 18, 0);
      break;
    case 'V':
      break;
    case 'N': parse_new(); break;
    case 'S':
      if(command_buffer[1] == '-') set_error(ERROR_SYNTAX_UNKNOWN);   // S-8 etc.
      else parse_scratch();
      break;
    case 'R':
      if(command_buffer[1] == 'D') parse_rmdir();
      else parse_rename();
      break;
    case 'C':
      if(command_buffer[1] == 'D') parse_chdir();
      else parse_copy();
      break;
    case 'M': parse_mkdir(); break;
    case 'B': parse_block(); break;
    case 'P': parse_position(); break;
    case '/':                                   // CMD style "/dir" = CD
      memmove(command_buffer + 2, command_buffer, command_length);
      command_length += 2;
      command_buffer[0] = 'C'; command_buffer[1] = 'D';
      parse_chdir();
      break;
    default:
      set_error(ERROR_SYNTAX_UNKNOWN);
  }
  fs_sync();
}
