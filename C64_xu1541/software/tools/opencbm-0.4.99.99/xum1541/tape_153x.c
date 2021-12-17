/*
 * CBM 1530/1531 tape routines.
 * Copyright (c) 2012 Arnd Menge, arnd(at)jonnz(dot)de
 */

#include "xum1541.h"

#ifdef TAPE_SUPPORT

// Tape firmware version (check tape.h)
#define TapeFirmwareVersion 0x0001

// Tape State Register: Current state of tape operations.
volatile uint8_t TSR = 0;

// Enable definition when tape device hot-plug is allowed.
//#define DeviceHotPlugAllowed

// Global variables (capture)
static volatile uint32_t Tape_Timer1Ovf   = 0; // Timer1 overflow counter.
static volatile uint16_t Tape_Timer1Stamp = 0; // Timer1-ICR1 timestamp.
static volatile uint16_t Tape_Timer1Stamp_last = 0; // Last Timer1-ICR1 timestamp.

// Global variables (write)
static volatile uint32_t HiDelta;
static volatile uint16_t LoDelta;
static volatile uint32_t DeltaCount;

// Global variables (misc)
volatile bool            StopWaitForSense = false;
static volatile uint16_t TapeStatus = Tape_Status_OK;

// OC1A output compare mode/level
#define OC1A_KEEP   0
#define OC1A_CHANGE 1
static volatile uint8_t  myOC1AMode  = 0;
static volatile uint8_t  myOC1ALevel = 0;

static struct ProtocolFunctions TapeFunctions = {
    .cbm_reset = Tape_Reset,
    .cbm_raw_write = NULL,
    .cbm_raw_read = NULL,
    .cbm_wait = NULL,
    .cbm_poll = NULL,
    .cbm_setrelease = NULL,
};

// Forward declarations
uint16_t    Tape_GetTapeFirmwareVersion(void);          // READ/WRITE
void        Tape_ClearDeviceConfigFlags(void);          // READ/WRITE
void        Tape_ResetPorts(void);                      // READ/WRITE
void        Tape_ResetTimerControlRegisters(void);      // READ/WRITE
void        Tape_DisableAndClearTapeInterrupts(void);   // READ/WRITE
void        Tape_ConfigureDisconnectDetection(void);    // READ/WRITE
void        Tape_SetBasicConfig(uint8_t CONFIG_OPTION); // READ/WRITE
void        Tape_Reset(bool whatever);                  // READ/WRITE
uint16_t    Probe4TapeDevice(void);                     // READ/WRITE
void        Enter_Tape_Mode(struct ProtocolFunctions ** protoFn); // READ/WRITE
uint16_t    Tape_isDeviceConnected(void);               // READ/WRITE
uint16_t    Tape_isDeviceConfigured(void);              // READ/WRITE
void        Tape_RememberDisconnectStatus(void);        // READ/WRITE
uint16_t    Tape_MotorOn(void);                         // READ/WRITE
uint16_t    Tape_MotorOff(void);                        // READ/WRITE
INLINE void Tape_Set_OC1A_to_High_Mode(void);           // WRITE
INLINE void Tape_Set_OC1A_to_Low_Mode(void);            // WRITE
INLINE void Tape_ToggleOC1AonCompareMatch(void);        // WRITE
INLINE void Tape_KeepOC1AonCompareMatch(void);          // WRITE
uint16_t    Tape_UploadConfig(void);                    // READ/WRITE
uint16_t    Tape_DownloadConfig(void);                  // READ/WRITE
uint16_t    Tape_PrepareCapture(void);                  // READ
uint16_t    Tape_PrepareWrite(void);                    // WRITE
uint16_t    Tape_GetSense(void);                        // READ/WRITE
uint16_t    Tape_WaitForStopSense(void);                // READ/WRITE
uint16_t    Tape_WaitForPlaySense(void);                // READ/WRITE
uint16_t    Tape_StartCapture(void);                    // READ
uint16_t    Tape_StartWrite(void);                      // WRITE
void        Tape_StopCapture(void);                     // READ
uint16_t    Tape_StartWrite(void);                      // WRITE
void        Tape_StopWrite(void);                       // WRITE
void        Tape_usbSendTimeStamp(void);                // READ
void        Tape_usbReceiveDelta(void);                 // WRITE
uint16_t    Tape_Capture(void);                         // READ
uint16_t    Tape_Write(void);                           // WRITE


// Return tape firmware version for compatibility check.
uint16_t Tape_GetTapeFirmwareVersion(void)
{
	return TapeFirmwareVersion;
}


// Set tape device disconnect flag.
// Disconnect and hot-plug of tape device is not allowed.
// Flag is only cleared by Probe4TapeDevice() after ZoomFloppy restart.
void Tape_RememberDisconnectStatus(void)
{
	TSR |= XUM1541_TAP_DISCONNECTED;
}


// Tape device disconnect check.
//   Return values:
//   - Tape_Status_OK
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_isDeviceConnected(void)
{
#ifdef DeviceHotPlugAllowed
	return ((PIN_DETECT & IO_DETECT_IN) ? Tape_Status_ERROR_Device_Disconnected : Tape_Status_OK);
#else
	if ((PIN_DETECT & IO_DETECT_IN) || (TSR & XUM1541_TAP_DISCONNECTED))
	{
		Tape_RememberDisconnectStatus(); // Remember tape device was disconnected.
		return Tape_Status_ERROR_Device_Disconnected;
	}
	return Tape_Status_OK;
#endif
}


// Returns if device is configured for read or write operations.
//   Return values:
//   - Tape_Status_OK_Device_Configured_for_Read
//   - Tape_Status_OK_Device_Configured_for_Write
//   - Tape_Status_ERROR_Device_Not_Configured
uint16_t Tape_isDeviceConfigured(void)
{
	if (TSR & XUM1541_TAP_DEVICE_CONFIG_READ)
		return Tape_Status_OK_Device_Configured_for_Read;

	if (TSR & XUM1541_TAP_DEVICE_CONFIG_WRITE)
		return Tape_Status_OK_Device_Configured_for_Write;

	return Tape_Status_ERROR_Device_Not_Configured;
}


// Clear device read/write configuration flags.
void Tape_ClearDeviceConfigFlags(void)
{
	TSR &= (uint8_t)~XUM1541_TAP_DEVICE_CONFIG_READ;
	TSR &= (uint8_t)~XUM1541_TAP_DEVICE_CONFIG_WRITE;
}


// Set all port pins to defined level (input/pull-up), but turn motor on port B off.
void Tape_ResetPorts(void)
{
	// Set MOTOR CONTROL flag.
	TSR |= XUM1541_TAP_MOTOR;

	DDRB  = (uint8_t)IO_MOTOR;
	PORTB = (uint8_t)~IO_MOTOR;

	DDRC  = 0;
	PORTC = 0xff;
	DDRD  = 0;
	PORTD = 0xff;
}


void Tape_ResetTimerControlRegisters(void)
{
	// Timer/Counter1 Control Register A/B.
	// Normal port operation (pins OCnA/B/C disconnected).
	// Timer1 Mode of Operation: Normal.
	TCCR1A = 0b00000000;

	// Timer/Counter1 Control Register B.
	// Deactivate Input Capture Noise Canceler (1=activate).
	// Restore ZF settings (board_init()).
	OCR1A = (F_CPU / 1024) / 10;
	TCCR1B = (1 << WGM12) | (1 << CS02) | (1 << CS00);
}


void Tape_DisableAndClearTapeInterrupts(void)
{
	// Disable tape interrupts.
	PCICR  &= (uint8_t)~(1 << PCIE0);  // Disable PCINT7..0 interrupts, PCIE0="Pin Change Interrupt Enable 0", PCICR="Pin Change Interrupt Control Register".
	TIMSK1 &= (uint8_t)~( (1 << ICIE1)|(1 << TOIE1)|(1 << OCIE1A) );
	PCMSK0 &= (uint8_t)~(1 << PCINT0); // Input pin change: SENSE
	EIMSK  &= (uint8_t)~IN_EIMSK;      // External Interrupt Mask Register: disable disconnect interrupt.
	// Clear pending interrupt flags.
	TIFR1 = 0xff;            // Timer1 Interrupt Flag Register.
	PCIFR = 0xff;            // Pin Change Interrupt Flag Register.
	EIFR = (uint8_t)IN_EIFR; // External Interrupt Flag Register: Clear pending disconnect interrupt flag.
}


// Configure device disconnect detection.
void Tape_ConfigureDisconnectDetection(void)
{
	// MCUCR PUD is default 0 on powerup and not changed by ZF, hence pull-up resistor usage allowed.
	MCUCR &= (uint8_t)~(1 << PUD); // Allow pull-up resistor usage, PUD="Pull-up Disable".
	DDR_DETECT &= (uint8_t)~IO_DETECT_IN;   // Switch PD0/INT0 to input.
	DDR_DETECT |= (uint8_t)IO_DETECT_OUT;   // Switch PD1 to output.
	PORT_DETECT |= (uint8_t)IO_DETECT_IN;   // Enable pull-up resistor on PD0.
	PORT_DETECT &= (uint8_t)~IO_DETECT_OUT; // Output low level on PD1.
	//DELAY_US(1);
}


// Set basic configuration. Keep MOTOR CONTROL setting if desired.
// Original ZF board configuration is not restored in order to save SRAM, disk operations might no longer work.
void Tape_SetBasicConfig(uint8_t CONFIG_OPTION)
{
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts.

	// Clear device config state flags.
	Tape_ClearDeviceConfigFlags();

	// Keep MOTOR CONTROL setting if desired.
	if (CONFIG_OPTION != TAPE_CONFIG_OPTION_KEEP_MOTOR)
	{
		// Set MOTOR CONTROL flag.
		TSR |= XUM1541_TAP_MOTOR;
		// Turn off motor.
		DDR_MOTOR |= (uint8_t)IO_MOTOR;
		PORT_MOTOR &= (uint8_t)~IO_MOTOR;
	}

	// Reset PC6/OC1A to input.
	DDR_WRITE &= (uint8_t)~IO_WRITE;

	// Timer/Counter1 Control Register A/B.
	// Normal port operation (pins OCnA/B/C disconnected).
	// Timer1 Mode of Operation: Normal.
	// Deactivate Input Capture Noise Canceler (1=activate).
	Tape_ResetTimerControlRegisters();

	// Disable tape interrupts & clear pending interrupt flags.
	Tape_DisableAndClearTapeInterrupts();

	SREG = oldSREG; // Restore Global Interrupt Enable state.
}


// External tape reset. Dummy argument.
void Tape_Reset(bool whatever)
{
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts.

	StopWaitForSense = true; // Stop WaitForSense loops.

	// Flag tape capture/write stop.
	if (TSR & XUM1541_TAP_CAPTURING) Tape_StopCapture();
	if (TSR & XUM1541_TAP_WRITING)   Tape_StopWrite();

	// Set all port pins to defined level (input/pull-up), but turn motor on port B off.
	Tape_ResetPorts();

	// Clear config flags, set basic configuration, motor off.
	Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC);

	// Configure for disconnect detection.
	Tape_ConfigureDisconnectDetection();

	TapeStatus = Tape_Status_ERROR_External_Break;
	SREG = oldSREG; // Restore Global Interrupt Enable state.
}


// Probe for tape device presence.
// Checks if PD0 gets low level from PD1.
// PD0 input, pull-up enabled.
// PD1 output low.
// PD0 has internal 20K-50K pull-up and 100K pull-down when disconnected.
//   Return values:
//   - Tape_Status_OK_Tape_Device_Present
//   - Tape_Status_OK_Tape_Device_Not_Present
uint16_t Probe4TapeDevice(void)
{
	uint8_t  scSaved_DDR_DETECT, scSaved_PORT_DETECT, scSaved_MCUCR;
	uint8_t  oldSREG = SREG; // Unknown Global Interrupt Enable state.
	uint16_t res;

	cli(); // Disable interrupts while probing.

	// Save previous configuration.
	scSaved_DDR_DETECT = DDR_DETECT;
	scSaved_PORT_DETECT = PORT_DETECT;
	scSaved_MCUCR = MCUCR;

	TSR = 0; // Clear Tape State Register.

	// Configure device disconnect detection.
	Tape_ConfigureDisconnectDetection();

	//   Return values:
	//   - Tape_Status_OK
	//   - Tape_Status_ERROR_Device_Disconnected
	res = Tape_isDeviceConnected();

	// Restore previous configuration.
	PORT_DETECT = scSaved_PORT_DETECT;
	DDR_DETECT = scSaved_DDR_DETECT;
	MCUCR = scSaved_MCUCR;

	// Restore Global Interrupt Enable state.
	SREG = oldSREG;

	return ( (res == Tape_Status_OK) ? Tape_Status_OK_Tape_Device_Present : Tape_Status_OK_Tape_Device_Not_Present);
}


// Enter XUM1541 tape mode. May be called multiple times.
// Must be called before any CBM 153x tape operations can be used.
// Disk operations can't be called afterwards.
void Enter_Tape_Mode(struct ProtocolFunctions ** protoFn)
{
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts.

	// Set default configuration.
	TSR |= (uint8_t)XUM1541_TAP_READ_STARTFALLEDGE;   // Start reading with falling edge.
	TSR &= (uint8_t)~XUM1541_TAP_WRITE_STARTFALLEDGE; // Start writing with rising edge.

	// Set all port pins to defined level (input/pull-up), but turn motor on port B off.
	Tape_ResetPorts();

	// Configure for disconnect detection.
	Tape_ConfigureDisconnectDetection();

	// Timer/Counter1 Control Register A/B.
	// Normal port operation (pins OCnA/B/C disconnected).
	// Timer1 Mode of Operation: Normal.
	// Deactivate Input Capture Noise Canceler (1=activate).
	Tape_ResetTimerControlRegisters();

	// Disable tape interrupts & clear pending interrupt flags.
	Tape_DisableAndClearTapeInterrupts();

	// Configure ProtocolFunctions interface for tape mode.
	if (protoFn != NULL)
		*protoFn = &TapeFunctions;

	SREG = oldSREG; // Restore Global Interrupt Enable state.
}


// Motor control.
//   Return values:
//   - Tape_Status_OK_Motor_On
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_MotorOn(void)
{
	if (Tape_isDeviceConnected() == Tape_Status_ERROR_Device_Disconnected)
	{
		// Device disconnected: Clear config flags, set basic configuration, motor off.
		Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC);
		return Tape_Status_ERROR_Device_Disconnected;
	}

	TSR |= XUM1541_TAP_MOTOR; // Set MOTOR CONTROL flag.

	DDR_MOTOR |= (uint8_t)IO_MOTOR;
	PORT_MOTOR |= (uint8_t)IO_MOTOR;

	return Tape_Status_OK_Motor_On;
}


// Motor control.
//   Return values:
//   - Tape_Status_OK_Motor_On
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_MotorOff(void)
{
	if (Tape_isDeviceConnected() == Tape_Status_ERROR_Device_Disconnected)
	{
		// Device disconnected: Clear config flags, set basic configuration, motor off.
		Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC);
		return Tape_Status_ERROR_Device_Disconnected;
	}

	TSR |= XUM1541_TAP_MOTOR; // Set MOTOR CONTROL flag.

	DDR_MOTOR |= (uint8_t)IO_MOTOR;
	PORT_MOTOR &= (uint8_t)~IO_MOTOR;

	return Tape_Status_OK_Motor_Off;
}


// Set WRITE pin to high on next OCR1A compare match.
INLINE void Tape_Set_OC1A_to_High_Mode(void)
{
	// Timer/Counter1 Control Register A.
	// Set OC1A on OCR1A compare match (set output to high level).
	TCCR1A |= (uint8_t)( (1 << COM1A1)|(1 << COM1A0) );

	myOC1ALevel = 1; // Update level tracker.
}


// Set WRITE pin to low on next OCR1A compare match.
INLINE void Tape_Set_OC1A_to_Low_Mode(void)
{
	// Timer/Counter1 Control Register A:
	// Clear OC1A on OCR1A compare match (set output to low level).
	TCCR1A |= (uint8_t)(1 << COM1A1);
	TCCR1A &= (uint8_t)~(1 << COM1A0);

	myOC1ALevel = 0; // Update level tracker.
}


// Toggle WRITE pin on next OCR1A compare match.
INLINE void Tape_ToggleOC1AonCompareMatch(void)
{
	myOC1AMode = OC1A_CHANGE; // Update mode tracker.
	if (myOC1ALevel == 0)
		Tape_Set_OC1A_to_High_Mode(); // Set WRITE pin to high on next OCR1A compare match.
	else
		Tape_Set_OC1A_to_Low_Mode(); // Set WRITE pin to low on next OCR1A compare match.
}


// Keep WRITE pin on next OC1A compare match.
INLINE void Tape_KeepOC1AonCompareMatch(void)
{
	myOC1AMode = OC1A_KEEP; // Update mode tracker.
}


// Receive tape read/write configuration from host PC.
//   Return values:
//   - Tape_Status_OK_Config_Uploaded
//   - Tape_Status_ERROR_usbRecvByte
uint16_t Tape_UploadConfig(void)
{
	uint8_t data;
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts.

	wdt_reset(); // Feed the watchdog.
	usbInitIo(-1, ENDPOINT_DIR_OUT);
	DELAY_MS(10);

	// Get 1 configuration byte.
	if (usbRecvByte(&data) != 0)
	{
		usbIoDone();
		SREG = oldSREG; // Restore Global Interrupt Enable state.
		return Tape_Status_ERROR_usbRecvByte;
	}

	// Store configuration to TSR.
	TSR &= (uint8_t)~(XUM1541_TAP_READ_STARTFALLEDGE | XUM1541_TAP_WRITE_STARTFALLEDGE);
	TSR |= (uint8_t)(data & (XUM1541_TAP_READ_STARTFALLEDGE | XUM1541_TAP_WRITE_STARTFALLEDGE));

	usbIoDone();
	SREG = oldSREG; // Restore Global Interrupt Enable state.
	return Tape_Status_OK_Config_Uploaded;
}


// Send tape read/write configuration to host PC.
//   Return values:
//   - Tape_Status_OK_Config_Downloaded
//   - Tape_Status_ERROR_usbSendByte
uint16_t Tape_DownloadConfig(void)
{
	uint8_t data = TSR;
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts.

	wdt_reset(); // Feed the watchdog.
	usbInitIo(-1, ENDPOINT_DIR_IN);
	DELAY_MS(10);

	// Send TSR byte.
	if (usbSendByte(data) != 0)
	{
		usbIoDone();
		SREG = oldSREG; // Restore Global Interrupt Enable state.
		return Tape_Status_ERROR_usbSendByte;
	}

	usbIoDone();
	SREG = oldSREG; // Restore Global Interrupt Enable state.
	return Tape_Status_OK_Config_Downloaded;
}


// Configure for tape capture.
//   Return values:
//   - Tape_Status_OK_Device_Configured_for_Read
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_PrepareCapture(void)
{
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts while configuring.

	wdt_reset(); // Feed the watchdog.

	StopWaitForSense = false; // Clear WaitForSense stop flag.

	// When configured for tape write:
	// Set basic configuration to disable write config.
	if (Tape_isDeviceConfigured() == Tape_Status_OK_Device_Configured_for_Write)
		Tape_SetBasicConfig(TAPE_CONFIG_OPTION_KEEP_MOTOR); // Clear config flags, set basic configuration, keep MOTOR CONTROL setting.

	// Init TapeStatus flag.
	TapeStatus = Tape_Status_OK;

	// Set all port pins to defined level (input/pull-up), but turn motor on port B off.
	Tape_ResetPorts();

	// Power Reduction Register 0, enable Timer1.
	power_timer1_enable(); // Enable Timer 1 module (->avr/power.h).
	//PRR0 &= (uint8_t)~(1 << PRTIM1);

	// Timer/Counter1 Control Register A/B.
	// Normal port operation (pins OCnA/B/C disconnected).
	// Timer1 Mode of Operation: Normal.
	TCCR1A = 0b00000000;
	TCCR1B &= (uint8_t)~( (1 << WGM13)|(1 << WGM12) );

	// Timer/Counter1 Control Register B.
	// Activate Input Capture Noise Canceler (1=activate).
	// Delays input capture by 4 system clock cycles at 16MHz.
	TCCR1B |= (uint8_t)(1 << ICNC1);

	// Timer/Counter1 Control Register B.
	if (TSR & XUM1541_TAP_READ_STARTFALLEDGE)
		TCCR1B &= (uint8_t)~(1 << ICES1); // Input Capture Edge Select: use falling edge as first trigger (0=falling edge).
	else
		TCCR1B |= (uint8_t)(1 << ICES1);  // Input Capture Edge Select: use rising edge as first trigger (1=rising edge).

	// Timer/Counter1 Control Register B.
	// Low Power Crystal Oscillator (XTAL1/2) -> TCCR1B (Clock Select CSn2:0).
	// Start Timer1 at Clock Select = clk/1 (16 MHz timer precision).
	TCCR1B |= (uint8_t)(1 << CS10);
	TCCR1B &= (uint8_t)~( (1 << CS12)|(1 << CS11) );

	// Timer1 Interrupt Mask Register.
	// Enable Timer1 Overflow Interrupt.
	// Disable all other interrupts here.
	// Timer1 feeds the watchdog until tape capturing starts.
	TIMSK1 = (uint8_t)(1 << TOIE1);

	// Prepare PC7/ICP1 for input capture: READ
	DDR_READ &= (uint8_t)~IO_READ; // Switch PC7 to input.
	PORT_READ |= (uint8_t)IO_READ; // Enable pull-up resistor on PC7.

	// Prepare PB0 for input pin change: SENSE
	DDR_SENSE &= (uint8_t)~IO_SENSE; // Switch PB0 to input.
	PORT_SENSE |= (uint8_t)IO_SENSE; // Enable pull-up resistor on PB0/PCINT0.
	MCUCR &= (uint8_t)~(1 << PUD); // Allow pull-up resistor usage, PUD="Pull-up Disable".
	PCICR = (uint8_t)(1 << PCIE0); // Enable PCINT7..0 interrupts, PCIE0="Pin Change Interrupt Enable 0", PCICR="Pin Change Interrupt Control Register".
	PCMSK0 = 0; // Disable pin change interrupts until capture starts, PCMSK0="Pin Change Mask Register 0".

	// Configure disconnect detection.
	Tape_ConfigureDisconnectDetection();

	// Prepare disconnect interrupt.
	EIMSK &= (uint8_t)~IN_EIMSK; // External Interrupt Mask Register: disable disconnect interrupt.
	EICRA |= (uint8_t)IN_EICRA;  // External Interrupt Sense Control: Rising edge generates interrupt request.

	// Clear pending interrupt flags.
	TIFR1 = 0xff;            // Timer1 Interrupt Flag Register.  
	PCIFR = 0xff;            // Pin Change Interrupt Flag Register.
	EIFR = (uint8_t)IN_EIFR; // External Interrupt Flag Register: Clear pending disconnect interrupt flag.

	// Prepare PB1 for motor control and turn motor on.
	if (Tape_MotorOn() == Tape_Status_ERROR_Device_Disconnected)
	{
		SREG = oldSREG; // Restore Global Interrupt Enable state.
		return Tape_Status_ERROR_Device_Disconnected; // Device disconnected: Routine cleared config flags, set basic configuration, motor off.
	}

	// Feed the watchdog until tape capturing starts.
	TSR &= (uint8_t)~XUM1541_TAP_CAPTURING; // Clear tape capture in progress flag.

	// Device is now configured for tape read operations.
	TSR |= (uint8_t)XUM1541_TAP_DEVICE_CONFIG_READ;

	SREG = oldSREG; // Restore Global Interrupt Enable state.
	// TIMER1_OVF IRQ must be active now.

	return Tape_Status_OK_Device_Configured_for_Read;
}


// Configure for tape write.
//   Return values:
//   - Tape_Status_OK_Device_Configured_for_Write
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_PrepareWrite(void)
{
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts while configuring.

	wdt_reset(); // Feed the watchdog.

	StopWaitForSense = false; // Clear WaitForSense stop flag.

	// When configured for tape read:
	// Set basic configuration to disable read config.
	if (Tape_isDeviceConfigured() == Tape_Status_OK_Device_Configured_for_Read)
		Tape_SetBasicConfig(TAPE_CONFIG_OPTION_KEEP_MOTOR); // Clear config flags, set basic configuration, keep MOTOR CONTROL setting.

	// Init TapeStatus flag.
	TapeStatus = Tape_Status_OK;

	// Set all port pins to defined level (input/pull-up), but turn motor on port B off.
	Tape_ResetPorts();

	// Power Reduction Register 0, enable Timer1.
	power_timer1_enable(); // Enable Timer 1 module (->avr/power.h).
	//PRR0 &= (uint8_t)~(1 << PRTIM1);

	// Note: WRITE signal is usually inverted READ signal.
	// Timer/Counter1 Control Register C: Force Output Compare for Channel A.
	if (TSR & XUM1541_TAP_WRITE_STARTFALLEDGE)
		Tape_Set_OC1A_to_High_Mode(); // We start with OC1A=1 (first WRITE signal is falling/negative edge).
	else
		Tape_Set_OC1A_to_Low_Mode();  // We start with OC1A=0 (first WRITE signal is rising/positive edge).
	TCCR1C |= (uint8_t)(1 << FOC1A);
	Tape_KeepOC1AonCompareMatch(); // Keep OC1A until WRITE actually starts.

	// Timer/Counter1 Control Register A/B.
	// Timer1 Mode of Operation: CTC (0-1-0-0).
	TCCR1A &= (uint8_t)~( (1 << WGM11)|(1 << WGM10) );
	TCCR1B = (uint8_t)(1 << WGM12);
	TCCR1B &= (uint8_t)~(1 << WGM13);

	// Timer/Counter1 Control Register B.
	// Low Power Crystal Oscillator (XTAL1/2) -> TCCR1B (Clock Select CSn2:0).
	// Start Timer1 at Clock Select = clk/1 (16 MHz timer precision).
	TCCR1B |= (uint8_t)(1 << CS10);
	TCCR1B &= (uint8_t)~( (1 << CS12)|(1 << CS11) );

	// Timer1 Interrupt Mask Register.
	// Enable Timer1 Output Compare A Match Interrupt.
	// Disable all other Timer1 interrupts here.
	// Timer1 feeds the watchdog until tape write starts.
	TIMSK1 = (uint8_t)(1 << OCIE1A);

	// Prepare PC6/OC1A for output: WRITE
	DDR_WRITE |= (uint8_t)IO_WRITE;  // Switch PC6/OC1A to output.

	// Prepare PB0/PCINT0 for input pin change: SENSE
	DDR_SENSE &= (uint8_t)~IO_SENSE; // Switch PB0/PCINT0 to input.
	PORT_SENSE |= (uint8_t)IO_SENSE; // Enable pull-up resistor on PB0/PCINT0.
	MCUCR &= (uint8_t)~(1 << PUD);   // Allow pull-up resistor usage, PUD="Pull-up Disable".
	PCICR = (uint8_t)(1 << PCIE0);   // Enable PCINT7..0 interrupts, PCIE0="Pin Change Interrupt Enable 0", PCICR="Pin Change Interrupt Control Register".
	PCMSK0 = 0; // Disable pin change interrupts until capture starts, PCMSK0="Pin Change Mask Register 0".

	// Configure disconnect detection.
	Tape_ConfigureDisconnectDetection();

	// Prepare disconnect interrupt.
	EIMSK &= (uint8_t)~IN_EIMSK; // External Interrupt Mask Register: disable disconnect interrupt.
	EICRA |= (uint8_t)IN_EICRA;  // External Interrupt Sense Control: Rising edge generates interrupt request.

	// Clear pending interrupt flags.
	TIFR1 = 0xff; // Timer1 Interrupt Flag Register.  
	PCIFR = 0xff; // Pin Change Interrupt Flag Register.
	EIFR = (uint8_t)IN_EIFR; // External Interrupt Flag Register: Clear pending disconnect interrupt flag.

	// Prepare PB1 for motor control and turn motor on.
	if (Tape_MotorOn() == Tape_Status_ERROR_Device_Disconnected)
	{
		SREG = oldSREG; // Restore Global Interrupt Enable state.
		return Tape_Status_ERROR_Device_Disconnected; // Device disconnected: Routine cleared config flags, set basic configuration, motor off.
	}

	// Feed the watchdog until tape writing starts.
	TSR &= (uint8_t)~XUM1541_TAP_WRITING; // Clear tape write in progress flag.
	OCR1A = 0xBB80; // =3ms (16MHz timer).

	// Device is now configured for tape write operations.
	TSR |= (uint8_t)XUM1541_TAP_DEVICE_CONFIG_WRITE;

	SREG = oldSREG; // Restore Global Interrupt Enable state.
	// TIMER1_COMPA IRQ must be active now.

	return Tape_Status_OK_Device_Configured_for_Write;
}


// Return tape SENSE state.
//   Return values:
//   - Tape_Status_OK_Sense_On_Play
//   - Tape_Status_OK_Sense_On_Stop
//   - Tape_Status_ERROR_Device_Not_Configured
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_GetSense(void)
{
	// Must be configured for tape operations.
	if (Tape_isDeviceConfigured() == Tape_Status_ERROR_Device_Not_Configured)
		return Tape_Status_ERROR_Device_Not_Configured;

	if (Tape_isDeviceConnected() == Tape_Status_ERROR_Device_Disconnected)
	{
		// Device disconnected: Clear config flags, set basic configuration, motor off.
		Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC);
		return Tape_Status_ERROR_Device_Disconnected;
	}

	return ((PIN_SENSE & IO_SENSE) ? Tape_Status_OK_Sense_On_Stop : Tape_Status_OK_Sense_On_Play);
}


// Wait for tape <STOP> if running. Feed watchdog while waiting.
//   Return values:
//   - Tape_Status_OK_Sense_On_Stop
//   - Tape_Status_ERROR_Device_Not_Configured
//   - Tape_Status_ERROR_Device_Disconnected
//   - Tape_Status_ERROR_External_Break
uint16_t Tape_WaitForStopSense(void)
{
	bool StopSenseDetected = false;
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.

	// Must be configured for tape operations.
	if (Tape_isDeviceConfigured() == Tape_Status_ERROR_Device_Not_Configured)
		return Tape_Status_ERROR_Device_Not_Configured;

	sei(); // Enable interrupts while waiting.

	// Assume tape device is currently connected. Tape_GetSense() checked this.

	while ((Tape_isDeviceConnected() != Tape_Status_ERROR_Device_Disconnected) && (StopSenseDetected == false) && (StopWaitForSense == false))
	{
		StopSenseDetected = (PIN_SENSE & IO_SENSE);
		wdt_reset(); // Feed the watchdog.
	}

	SREG = oldSREG; // Restore Global Interrupt Enable state.

	if (StopWaitForSense)
		return Tape_Status_ERROR_External_Break; // StopWaitForSense flag cleared on next PrepareRead/PrepareWrite.

	if (StopSenseDetected)
		return Tape_Status_OK_Sense_On_Stop;

	// Device disconnected: Clear config flags, set basic configuration, motor off.
	Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC);

	return Tape_Status_ERROR_Device_Disconnected;
}


// Wait for tape <PLAY/RECORD> if stopped. Feed watchdog while waiting.
//   Return values:
//   - Tape_Status_OK_Sense_On_Play
//   - Tape_Status_ERROR_Device_Not_Configured
//   - Tape_Status_ERROR_Device_Disconnected
//   - Tape_Status_ERROR_External_Break
uint16_t Tape_WaitForPlaySense(void)
{
	bool StopSenseDetected = true;
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.

	// Must be configured for tape operations.
	if (Tape_isDeviceConfigured() == Tape_Status_ERROR_Device_Not_Configured)
		return Tape_Status_ERROR_Device_Not_Configured;

	sei(); // Enable interrupts while waiting.

	// Assume tape device is currently connected. Tape_GetSense() checked this.

	while ((Tape_isDeviceConnected() != Tape_Status_ERROR_Device_Disconnected) && (StopSenseDetected == true) && (StopWaitForSense == false))
	{
		StopSenseDetected = (PIN_SENSE & IO_SENSE);
		wdt_reset(); // Feed the watchdog.
	}

	SREG = oldSREG; // Restore Global Interrupt Enable state.

	if (StopWaitForSense)
		return Tape_Status_ERROR_External_Break; // StopWaitForSense flag cleared on next PrepareRead/PrepareWrite.

	if (!StopSenseDetected)
		return Tape_Status_OK_Sense_On_Play;
		
	// Device disconnected: Clear config flags, set basic configuration, motor off.
	Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC);

	return Tape_Status_ERROR_Device_Disconnected;
}


// Start actual tape capture.
// Tape SENSE is already on <PLAY> when entering this function.
//   Return values:
//   - Tape_Status_OK
//   - Tape_Status_ERROR_Sense_Not_On_Play
//   - Tape_Status_ERROR_Device_Not_Configured
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_StartCapture(void)
{
	// Must be configured for tape read operations.
	if (Tape_isDeviceConfigured() != Tape_Status_OK_Device_Configured_for_Read)
		return Tape_Status_ERROR_Device_Not_Configured;

	if (Tape_isDeviceConnected() == Tape_Status_ERROR_Device_Disconnected)
	{
		// Device disconnected: Clear config flags, set basic configuration, motor off.
		Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC);
		return Tape_Status_ERROR_Device_Disconnected;
	}

	// Enable device disconnect interrupt.
	EIFR = (uint8_t)IN_EIFR;    // External Interrupt Flag Register: Clear pending disconnect interrupt flag.
	EIMSK |= (uint8_t)IN_EIMSK; // External Interrupt Mask Register: enable disconnect interrupt.

	// Reset Timer1 and its counters.
	TCNT1 = 0;
	Tape_Timer1Ovf = 0;
	Tape_Timer1Stamp_last = 0;

	// Timer1 Interrupt Flag Register. Clear pending interrupt flags.
	TIFR1 = 0xff; // TIFR1 |= (1<<ICF1)|(1<<TOV1); // ICF1 = Timer1 Input Capture Flag, TOV1 = Timer1 Overflow Flag.

	// Set tape capture flag.
	TSR |= (uint8_t)XUM1541_TAP_CAPTURING;

	// Pin Change Mask Register 0. Enable pin change interrupt on PB0: SENSE
	PCMSK0 = (uint8_t)(1 << PCINT0);

	// Timer1 Interrupt Mask Register. Enable Input Capture Interrupt: READ
	// Tape SENSE is already on <PLAY> so we have a valid READ signal when enabling (->ZF 100K pull-down).
	TIMSK1 |= (uint8_t)(1 << ICIE1);

	// Pin Change Interrupt Flag Register. Clear pending interrupt flags.
 	PCIFR = 0xff;

	// Timer1 Interrupt Flag Register. Clear pending input capture interrupt flag.
	TIFR1 |= (1<<ICF1);

	// Make sure the tape is still on <PLAY>.
	if (Tape_GetSense() != Tape_Status_OK_Sense_On_Play)
	{
		Tape_StopCapture();
		return Tape_Status_ERROR_Sense_Not_On_Play;
	}

	// TapeStatus was initialized to Tape_Status_OK by Tape_PrepareCapture().
	return TapeStatus;
}


// Start actual tape write.
// Tape SENSE is already on <RECORD> when entering this function.
//   Return values:
//   - Tape_Status_OK
//   - Tape_Status_ERROR_usbRecvByte
//   - Tape_Status_ERROR_Sense_Not_On_Record
//   - Tape_Status_ERROR_Device_Not_Configured
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_StartWrite(void)
{
	// Must be configured for tape write operations.
	if (Tape_isDeviceConfigured() != Tape_Status_OK_Device_Configured_for_Write)
		return Tape_Status_ERROR_Device_Not_Configured;

	if (Tape_isDeviceConnected() == Tape_Status_ERROR_Device_Disconnected)
	{
		// Device disconnected: Clear config flags, set basic configuration, motor off.
		Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC);
		return Tape_Status_ERROR_Device_Disconnected;
	}

	// Flag tape write.
	TSR |= (uint8_t)XUM1541_TAP_WRITING;

	// Enable device disconnect interrupt.
	EIFR = (uint8_t)IN_EIFR;    // External Interrupt Flag Register: Clear pending disconnect interrupt flag.
	EIMSK |= (uint8_t)IN_EIMSK; // External Interrupt Mask Register: enable disconnect interrupt.

	// Pin Change Mask Register 0. Enable pin change interrupt on PB0: SENSE
	PCMSK0 = (uint8_t)(1 << PCINT0);

	// Receive first delta (+ avoid SENSE signal noise).
	Tape_usbReceiveDelta();

	// Reset Timer1.
	TCNT1 = 0;

	// Handle first delta.
	if (HiDelta == 0)
	{
		// Set OC1A to low in this timer1 cycle.
		OCR1A = LoDelta;
		Tape_ToggleOC1AonCompareMatch();
	}
	else
	{
		// Keep OC1A high in this timer1 cycle.
		Tape_KeepOC1AonCompareMatch();
		if (LoDelta < 0x8000)
		{
			LoDelta += 0x8000; // Shift LoDelta into upper timer1 cycle half.
			OCR1A = 0x8000; // Pull 0x8000 from Delta.
		}
		else
			OCR1A = 0xffff;
	}

	// Clear pending interrupt flags.
	TIFR1 = 0xff; // Timer1 Interrupt Flag Register.  
	PCIFR = 0xff; // Pin Change Interrupt Flag Register.

	// Make sure the tape is still on <RECORD>.
	if (Tape_GetSense() != Tape_Status_OK_Sense_On_Play)
	{
		Tape_StopWrite();
		return Tape_Status_ERROR_Sense_Not_On_Record;
	}

	// TapeStatus initialized to Tape_Status_OK by Tape_PrepareWrite().
	// Tape_usbReceiveDelta() above ("Receive first delta") may have flagged USB transfer failure.
	return TapeStatus;
}


// Stops tape capture in progress.
// Executed while interrupts disabled.
void Tape_StopCapture(void)
{
	// Turn off motor.
	Tape_MotorOff();

	// Disable our interrupts.
	TIMSK1 &= (uint8_t)~( (1 << ICIE1)|(1 << TOIE1) );
	PCMSK0 &= (uint8_t)~(1 << PCINT0); // Pin Change Mask Register 0. Disable pin change interrupt on PB0: SENSE
	EIMSK  &= (uint8_t)~IN_EIMSK;      // External Interrupt Mask Register: disable disconnect interrupt.

	// Clear pending interrupt flags.
	TIFR1 = 0xff; // Timer1 Interrupt Flag Register.
	PCIFR = 0xff; // Pin Change Interrupt Flag Register.
	EIFR = (uint8_t)IN_EIFR; // External Interrupt Flag Register: Clear pending disconnect interrupt flag.

	// Flag tape capture stop.
	TSR &= (uint8_t)~XUM1541_TAP_CAPTURING;
}


// Stops tape write in progress.
// Executed while interrupts disabled.
void Tape_StopWrite(void)
{
	// Turn off motor.
	Tape_MotorOff();

	// Keep last OC1A signal.
	Tape_KeepOC1AonCompareMatch();

	// Disable our interrupts.
	PCMSK0 &= (uint8_t)~(1 << PCINT0); // Pin Change Mask Register 0. Disable pin change interrupt on PB0: SENSE
	EIMSK  &= (uint8_t)~IN_EIMSK;      // External Interrupt Mask Register: disable disconnect interrupt.

	// Clear pending interrupt flags.
	TIFR1 = 0xff; // Timer1 Interrupt Flag Register.
	PCIFR = 0xff; // Pin Change Interrupt Flag Register.
	EIFR = (uint8_t)IN_EIFR; // External Interrupt Flag Register: Clear pending disconnect interrupt flag.

	// Feed the watchdog.
	OCR1A = 0x7D00; // =2ms (16MHz timer).

	// Flag tape write stop.
	TSR &= (uint8_t)~XUM1541_TAP_WRITING;
}


// Send timestamp to host. Stop tape capture on error.
// Executed from ISR while interrupts disabled.
// Flags "Tape_Status_ERROR_usbSendByte" on USB transfer error.
void Tape_usbSendTimeStamp(void)
{
	// Calculate delta
	HiDelta = Tape_Timer1Ovf;
	LoDelta = Tape_Timer1Stamp - Tape_Timer1Stamp_last;
	if (Tape_Timer1Stamp < Tape_Timer1Stamp_last)
		HiDelta--;
	Tape_Timer1Ovf = 0;
	Tape_Timer1Stamp_last = Tape_Timer1Stamp;

	if ((HiDelta != 0) || (LoDelta >= 0x8000))
	{
		// Long signal (>=2ms)
		// MSB of 5-byte timestamp must be 1 (restricts deltas to max 9.5 hours).
		if (usbSendByte(((HiDelta >> 16) & 0xff) | 0x80) != 0)
		{
			Tape_StopCapture();
			TapeStatus = Tape_Status_ERROR_usbSendByte;
			return;
		}

		if (usbSendByte((HiDelta >> 8) & 0xff) != 0)
		{
			Tape_StopCapture();
			TapeStatus = Tape_Status_ERROR_usbSendByte;
			return;
		}

		if (usbSendByte(HiDelta & 0xff) != 0)
		{
			Tape_StopCapture();
			TapeStatus = Tape_Status_ERROR_usbSendByte;
			return;
		}
	}

	if (usbSendByte(LoDelta >> 8) != 0)
	{
		Tape_StopCapture();
		TapeStatus = Tape_Status_ERROR_usbSendByte;
		return;
	}

	if (usbSendByte(LoDelta & 0xff) != 0)
	{
		Tape_StopCapture();
		TapeStatus = Tape_Status_ERROR_usbSendByte;
		return;
	}
}


// Receive delta from host. Stop tape write on error.
// Executed from ISR while interrupts disabled.
// Flags "Tape_Status_ERROR_usbRecvByte" on USB transfer error.
void Tape_usbReceiveDelta(void)
{
	uint8_t data, data2;

	if (usbRecvByte(&data) != 0)
	{
		Tape_StopWrite();
		TapeStatus = Tape_Status_ERROR_usbRecvByte;
		return;
	}

	if (usbRecvByte(&data2) != 0)
	{
		Tape_StopWrite();
		TapeStatus = Tape_Status_ERROR_usbRecvByte;
		return;
	}

	if ((data & 0x80) == 0)
	{
		// Short signal (<2ms)
		HiDelta = 0;
		LoDelta = data;
		LoDelta = (LoDelta << 8) + data2;

		// Update delta counter.
		DeltaCount -= 2;
	}
	else
	{
		// Long signal (>=2ms)
		HiDelta = data & 0x7F;
		HiDelta = (HiDelta << 8) + data2;

		if (usbRecvByte(&data) != 0)
		{
			Tape_StopWrite();
			TapeStatus = Tape_Status_ERROR_usbRecvByte;
			return;
		}
		HiDelta = (HiDelta << 8) + data;

		if (usbRecvByte(&data) != 0)
		{
			Tape_StopWrite();
			TapeStatus = Tape_Status_ERROR_usbRecvByte;
			return;
		}
		LoDelta = data;

		if (usbRecvByte(&data) != 0)
		{
			Tape_StopWrite();
			TapeStatus = Tape_Status_ERROR_usbRecvByte;
			return;
		}
		LoDelta = (LoDelta << 8) + data;

		// Update delta counter.
		DeltaCount -= 5;
	}
}


// Pin change interrupt: SENSE
// Stop tape capture/write if user presses <STOP>.
// Only active while actually reading or writing.
ISR(PCINT0_vect)
{
	if (TSR & XUM1541_TAP_CAPTURING) Tape_StopCapture();
	if (TSR & XUM1541_TAP_WRITING)
	{
		Tape_StopWrite();
		TapeStatus = Tape_Status_ERROR_Write_Interrupted_By_Stop;
	}
}


// INT0: device autodetect
// Stop tape operations if user disconnects tape hardware.
ISR(INT0_vect)
{
	EIMSK &= (uint8_t)~IN_EIMSK; // External Interrupt Mask Register: disable disconnect interrupt.
	EIFR = (uint8_t)IN_EIFR;     // External Interrupt Flag Register: Clear pending disconnect interrupt flag.

	if (TSR & XUM1541_TAP_CAPTURING) Tape_StopCapture();
	if (TSR & XUM1541_TAP_WRITING)   Tape_StopWrite();

#ifndef DeviceHotPlugAllowed
	// Remember tape device was disconnected.
	// Disconnect and hot-plug of tape device is not allowed.
	Tape_RememberDisconnectStatus();
#endif

	TapeStatus = Tape_Status_ERROR_Device_Disconnected;
}


// Timer1 capture event: PC7/ICP1.
ISR(TIMER1_CAPT_vect)
{
	Tape_Timer1Stamp = ICR1;         // Save ICR1 timestamp.
	TCCR1B ^= (uint8_t)(1 << ICES1); // Timer/Counter1 Control Register B. Change edge sensing.

	if ( (Tape_Timer1Stamp < 0xFE00) && (TIFR1 & (1 << TOV1)) )
	{
		// Handle race condition. (Todo: handle duty cycles of ~0% and ~100%).
		// 0xFFFF minus time for instructions executed so far for this ISR.
		// This works with small ICR1 values.
		// A high ICR1>0xC35 (50us) value here might indicate a problem with
		// preceding USB transfer and lost signal edges due to overridden ICR1.
		Tape_Timer1Ovf++;
		TIFR1 |= (uint8_t)(1 << TOV1);
	}

	Tape_usbSendTimeStamp(); // Send 5-byte timestamp to host.
}


// Timer1 overflow counter for tape capture timestamp generation.
// Feed watchdog when no tape capture in progress.
ISR(TIMER1_OVF_vect)
{
	if (TSR & XUM1541_TAP_CAPTURING)
		Tape_Timer1Ovf++; // Timer1 overflow counter.
	else
		wdt_reset(); // Feed the watchdog when not capturing.
}


// Timer1 OCR1A compare match: handle WRITE pin PC6/OC1A.
// Feed watchdog when no tape writing in progress.
ISR(TIMER1_COMPA_vect)
{
	if ((TSR & XUM1541_TAP_WRITING) == 0)
	{
		wdt_reset(); // Feed the watchdog when not writing.
	}
	else // (TSR & XUM1541_TAP_WRITING)
	{
		if (myOC1AMode == OC1A_KEEP)
		{
			// No toggle on OC1A pin occurred.

			HiDelta--; // Update HiDelta.

			if (HiDelta == 0)
			{
				// Toggle OC1A in this timer1 cycle.
				OCR1A = LoDelta;
				Tape_ToggleOC1AonCompareMatch();
			}
			else
				OCR1A = 0xffff;
		}
		else // (myOC1AMode == OC1A_CHANGE)
		{
			// A toggle on OC1A pin occurred.

			if (DeltaCount < 1)
			{
				Tape_StopWrite();
			}
			else
			{
				Tape_usbReceiveDelta(); // Get next delta. LoDelta < 10 is endless CTC.

				if (HiDelta == 0)
				{
					// Toggle OC1A in this timer1 cycle.
					OCR1A = LoDelta;
					Tape_ToggleOC1AonCompareMatch();
				}
				else
				{
					// Keep OC1A in this timer1 cycle.
					Tape_KeepOC1AonCompareMatch();
					if (LoDelta < 0x8000)
					{
						LoDelta += 0x8000; // Shift LoDelta into upper timer1 cycle half.
						OCR1A = 0x8000; // Pull 0x8000 from Delta.
					}
					else
						OCR1A = 0xffff;
				}
			} // else (DeltaCount >= 1)
		}
	} // else (TSR & XUM1541_TAP_WRITING)
}


// Tape capture loop. Starts the actual tape capturing.
//   Return values:
//   - Tape_Status_OK_Capture_Finished
//   - Tape_Status_ERROR_External_Break
//   - Tape_Status_ERROR_usbSendByte
//   - Tape_Status_ERROR_Sense_Not_On_Play
//   - Tape_Status_ERROR_Device_Not_Configured
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_Capture(void)
{
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts.

	wdt_reset(); // Feed the watchdog.
	usbInitIo(-1, ENDPOINT_DIR_IN);
	DELAY_MS(10);
	DELAY_MS(30); // Avoid SENSE signal noise.

	//   Return values:
	//   - Tape_Status_OK
	//   - Tape_Status_ERROR_Sense_Not_On_Play
	//   - Tape_Status_ERROR_Device_Not_Configured
	//   - Tape_Status_ERROR_Device_Disconnected
	TapeStatus = Tape_StartCapture(); // Start actual tape capture.

	sei(); // Enable interrupts for tape capture.

	while (TSR & XUM1541_TAP_CAPTURING)
		wdt_reset(); // Feed the watchdog while capturing.

	Set_usbDataLen(0);
	usbIoDone();

	Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC); // Clear config flags, set basic configuration, motor off.

	SREG = oldSREG; // Restore Global Interrupt Enable state.

	// Everything ok if Tape_Status_OK, return Tape_Status_OK_Capture_Finished.
	// If error occurred return specific error reason.
	return ((TapeStatus == Tape_Status_OK) ? Tape_Status_OK_Capture_Finished : TapeStatus);
}


// Tape write loop. Starts the actual tape writing.
//   Return values:
//   - Tape_Status_OK_Write_Finished
//   - Tape_Status_ERROR_External_Break
//   - Tape_Status_ERROR_usbRecvByte
//   - Tape_Status_ERROR_Write_Interrupted_By_Stop
//   - Tape_Status_ERROR_Sense_Not_On_Record
//   - Tape_Status_ERROR_Device_Not_Configured
//   - Tape_Status_ERROR_Device_Disconnected
uint16_t Tape_Write(void)
{
	uint8_t oldSREG = SREG; // Unknown Global Interrupt Enable state.
	cli(); // Disable interrupts.

	wdt_reset(); // Feed the watchdog.
	usbInitIo(-1, ENDPOINT_DIR_OUT);
	DELAY_MS(10);

	// Get number of delta bytes.
	// Sets TapeStatus to "Tape_Status_ERROR_usbRecvByte" if USB transfer fails.
	// TapeStatus was initialized to Tape_Status_OK by initial Tape_PrepareWrite().
	Tape_usbReceiveDelta();
	DeltaCount = HiDelta;
	DeltaCount = (DeltaCount << 16) | LoDelta; // Only lower 2 bytes of HiDelta used here.

	//   Return values:
	//   - Tape_Status_OK
	//   - Tape_Status_ERROR_usbRecvByte
	//   - Tape_Status_ERROR_Sense_Not_On_Record
	//   - Tape_Status_ERROR_Device_Not_Configured
	//   - Tape_Status_ERROR_Device_Disconnected
	TapeStatus = Tape_StartWrite(); // Start actual tape write.

	sei(); // Enable interrupts for tape write.

	while (TSR & XUM1541_TAP_WRITING)
		wdt_reset(); // Feed the watchdog while writing.

	Set_usbDataLen(0);

	// Stall endpoint if tape write stopped early. Feeds watchdog.
	if (DeltaCount > 0)
		Endpoint_StallTransaction();
	else
		usbIoDone();

	Tape_SetBasicConfig(TAPE_CONFIG_OPTION_BASIC); // Clear config flags, set basic configuration, motor off.

	SREG = oldSREG; // Restore Global Interrupt Enable state.

	// Everything ok if Tape_Status_OK, return Tape_Status_OK_Write_Finished.
	// If error occurred return specific error reason.
	return ((TapeStatus == Tape_Status_OK) ? Tape_Status_OK_Write_Finished : TapeStatus);
}

#endif // TAPE_SUPPORT
