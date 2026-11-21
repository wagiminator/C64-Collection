// ===================================================================================
// Basic I2C Master Functions for ATmega48/88/168/328(P)                      * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include "i2c.h"

static uint8_t I2C_err = 0;                     // error flag

// Wait for the end of a bus operation (with timeout of about 1ms)
static void I2C_wait(void) {
  uint16_t timeout = F_CPU / 10000;
  while(!(TWCR & (1<<TWINT))) {
    if(!--timeout) {
      I2C_err = 1;
      TWCR = 0;                                 // reset TWI module
      TWCR = (1<<TWEN);
      return;
    }
  }
}

// I2C init function
void I2C_init(void) {
  PORTC |= (1<<4) | (1<<5);                     // internal pull-ups on SDA and SCL
  TWSR  = 0;                                    // prescaler 1
  TWBR  = ((F_CPU / I2C_CLOCK) - 16) / 2;       // set bit rate
  TWCR  = (1<<TWEN);                            // enable TWI
  I2C_err = 0;
}

// I2C start transmission
void I2C_start(uint8_t addr) {
  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);   // send START condition
  I2C_wait();
  TWDR = addr;                                  // send slave address
  TWCR = (1<<TWINT) | (1<<TWEN);
  I2C_wait();
  if((TWSR & 0xF8) != 0x18 && (TWSR & 0xF8) != 0x40) I2C_err = 1;   // no ACK
}

// I2C transmit one data byte to the slave
void I2C_write(uint8_t data) {
  TWDR = data;
  TWCR = (1<<TWINT) | (1<<TWEN);
  I2C_wait();
}

// I2C stop transmission
void I2C_stop(void) {
  TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);   // send STOP condition
  uint16_t timeout = F_CPU / 10000;
  while((TWCR & (1<<TWSTO)) && --timeout);      // wait until STOP is sent
}

// Check for bus errors since init
uint8_t I2C_error(void) {
  return I2C_err;
}
