{*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004-2005 Spiro Trikaliotis
 *
}

unit opencbm;

interface

{ I'm not sure if this is the way to define constants }
const IEC_DATA = 1;
const IEC_CLOCK = 2;
const IEC_ATN = 4;
const IEC_RESET = 8;

type cbm_device_type = ( { one of }
   cbm_dt_unknown = -1,
   cbm_dt_cbm1541 = 0,
   cbm_dt_cbm1570 = 1,
   cbm_dt_cbm1571 = 2,
   cbm_dt_cbm1581 = 3
   );

type CBM_FILE = INTEGER; { HANDLE }
type PCBM_FILE = ^CBM_FILE;
const CBM_FILE_INVALID :CBM_FILE = -1; { this denotes an invalid CBM_FILE }

function cbm_driver_open(const HFile :PCBM_FILE; const Port :Integer) :Integer; cdecl; external 'opencbm.dll';
procedure cbm_driver_close(const HFile :CBM_FILE); cdecl; external 'opencbm.dll';

function cbm_get_driver_name(const port:integer) : PChar; cdecl; external 'opencbm.dll';

function cbm_listen(const HFile :CBM_FILE; const Device :ShortInt;
	const SecAddr :ShortInt) : Integer; cdecl; external 'opencbm.dll';
function cbm_talk(const HFile :CBM_FILE; const Device :ShortInt;
	const SecAddr :ShortInt) : Integer; cdecl; external 'opencbm.dll';

function cbm_open(HFile :CBM_FILE; const Device :ShortInt; const SecAddr :ShortInt;
	const Filename :PChar; const FilenameSize :Integer) : Integer; cdecl; external 'opencbm.dll';
function cbm_close(const HFile :CBM_FILE; const Device :ShortInt;
	const SecAddr :ShortInt) : Integer; cdecl; external 'opencbm.dll';

function cbm_raw_read(const HFile :CBM_FILE; Buffer :PChar; const BufferSize :Integer) : Integer; cdecl; external 'opencbm.dll';
function cbm_raw_write(const HFile :CBM_FILE; const Buffer :PChar; const BufferSize :Integer) : Integer; cdecl; external 'opencbm.dll';

function cbm_unlisten(const HFile :CBM_FILE) : Integer; cdecl; external 'opencbm.dll';
function cbm_untalk(const HFile :CBM_FILE) : Integer; cdecl; external 'opencbm.dll';

function cbm_get_eoi(const HFile :CBM_FILE) : Integer; cdecl; external 'opencbm.dll';
function cbm_clear_eoi(const HFile :CBM_FILE) : Integer; cdecl; external 'opencbm.dll';

function cbm_reset(const HFile :CBM_FILE) : Integer; cdecl; external 'opencbm.dll';

function cbm_pp_read(const HFile :CBM_FILE) : ShortInt; cdecl; external 'opencbm.dll';
procedure cbm_pp_write(const HFile :CBM_FILE; const Value :ShortInt); cdecl; external 'opencbm.dll';

function cbm_iec_poll(const HFile :CBM_FILE) : Integer; cdecl; external 'opencbm.dll';
function cbm_iec_get(const HFile :CBM_FILE; const Line :Integer) : Integer; cdecl; external 'opencbm.dll';
procedure cbm_iec_set(const HFile :CBM_FILE; const Line :Integer); cdecl; external 'opencbm.dll';
procedure cbm_iec_release(const HFile :CBM_FILE; const Line :Integer); cdecl; external 'opencbm.dll';
procedure cbm_iec_setrelease(const HFile :CBM_FILE; const Set :Integer; const Release :Integer); cdecl; external 'opencbm.dll';
function cbm_iec_wait(const HFile :CBM_FILE; const Line :Integer; const State :Integer) : Integer; cdecl; external 'opencbm.dll';

function cbm_upload(const HFile :CBM_FILE; const Device :ShortInt; const AddressInFloppy :Integer;
	const Prog :PChar; const ProgLength :Integer) : Integer; cdecl; external 'opencbm.dll';
function cbm_device_status(const HFile :CBM_FILE; const Device :ShortInt; Buffer :PChar;
	const BufferSize :Integer) : Integer; cdecl; external 'opencbm.dll';
function cbm_exec_command(const HFile :CBM_FILE; const Device :ShortInt; const Command :PChar;
	const BufferSize :Integer) : Integer; cdecl; external 'opencbm.dll';

function cbm_identify(const HFile :CBM_FILE; const Device :ShortInt; DeviceType :CBM_DEVICE_TYPE;
	TypeString :PPChar) : Integer; cdecl; external 'opencbm.dll';

function cbm_petscii2ascii_c(const c: Char) : Char; cdecl; external 'opencbm.dll';
function cbm_ascii2petscii_c(const c: Char) : Char; cdecl; external 'opencbm.dll';
function cbm_petscii2ascii(const c: PChar) : PChar; cdecl; external 'opencbm.dll';
function cbm_ascii2petscii(const c: PChar) : PChar; cdecl; external 'opencbm.dll';

implementation

end.
