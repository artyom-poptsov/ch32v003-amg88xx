/* amg88xx.h -- AMG88xx implementation header for CH32V003.
 *
 * Copyright (C) 2026 Artyom V. Poptsov <poptsov.artyom@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __AMG88XX_H__
#define __AMG88XX_H__

#include <stdint.h>
#include <lib_i2c.h>

/**
 * AMG88XX register adresses.
 */
typedef enum {
  AMG88XX_PCTL         = 0x00,
  AMG88XX_RST          = 0x01,
  AMG88XX_FPSC         = 0x02,
  AMG88XX_INTC         = 0x03,
  AMG88XX_STAT         = 0x04,
  AMG88XX_SCLR         = 0x05,
  // 0x06 reserved
  AMG88XX_AVE          = 0x07,
  AMG88XX_INTHL        = 0x08,
  AMG88XX_INTHH        = 0x09,
  AMG88XX_INTLL        = 0x0A,
  AMG88XX_INTLH        = 0x0B,
  AMG88XX_IHYSL        = 0x0C,
  AMG88XX_IHYSH        = 0x0D,
  AMG88XX_TTHL         = 0x0E,
  AMG88XX_TTHH         = 0x0F,
  AMG88XX_INT_OFFSET   = 0x010,
  AMG88XX_PIXEL_OFFSET = 0x80
} amg88xx_register_t;

/**
 * AMG88XX power modes.
 */
typedef enum {
    AMG88XX_MODE_NORMAL      = 0x00,
    AMG88XX_MODE_SLEEP       = 0x01,
    AMG88XX_MODE_STAND_BY_60 = 0x20,
    AMG88XX_MODE_STAND_BY_10 = 0x21
} amg88xx_power_mode_t;

typedef enum {
    AMG88XX_FLAG_RESET    = 0x30,
    AMG88XX_INITIAL_RESET = 0x3f
} amg88xx_sw_reset_t;

typedef enum {
    AMG88XX_FPS_10 = 0x00,
    AMG88XX_FPS_1  = 0x01
} amg88xx_frame_rate_t;

typedef enum {
    AMG88XX_INT_DISABLED = 0x00,
    AMG88XX_INT_ENABLED  = 0x01
} amg88xx_interrupt_state_t;

typedef enum {
    AMG88XX_INT_MODE_DIFFERENCE     = 0x00,
    AMG88XX_INT_MODE_ABSOLUTE_VALUE = 0x01
} amg88xx_interrupt_mode_t;

extern const uint8_t AMG88XX_ADDRESS;;
extern const uint8_t AMG88XX_PIXEL_ARRAY_SIZE;
extern const float AMG88XX_PIXEL_TEMP_CONVERSION;
extern const float AMG88XX_THERMISTOR_CONVERSION;

typedef struct {
    /**
     * I2C device.
     */
    i2c_device_t device;

    /**
     * Power control register.  Possible values:
     *   0x00 -- Normal mode.
     *   0x01 -- Sleep mode.
     *   0x20 -- Stand-by mode (60s intermittence)
     *   0x21 -- Stand-by mode (10s intermittence)
     */
    uint8_t pctl;

    /**
     * Reset register.
     */
    uint8_t rst;

    /**
     * Frames per second.
     *   0 -- 10 FPS
     *   1 -- 1 FPS
     */
    uint8_t fps;

    /**
     * Interrupt control register.
     */
    uint8_t intc;

    /**
     * Status register.
     */
    uint8_t status;

    /**
     * Status clear register.
     */
    uint8_t sclr;

    /**
     * Average register.
     */
    uint8_t ave;

    /**
     * Interrupt level register.
     */
    uint8_t inthl;
    uint8_t inthh;

    /**
     * Interrupt lower limit.
     */
    uint8_t intll;
    uint8_t intlh;

    /**
     * Interrupt hysteresis level.
     */
    uint8_t ihysl;
    uint8_t ihysh;

    /**
     * Thermistor register.
     */
    uint8_t tthl;
    uint8_t tthh;

} amg88xx_t;

i2c_err_t amg88xx_init(amg88xx_t* this, uint16_t address);
i2c_err_t amg88xx_write(amg88xx_t* this, uint8_t reg, uint8_t* buf, uint8_t num);
i2c_err_t amg88xx_write8(amg88xx_t* this, uint8_t reg, uint8_t input_value);
i2c_err_t amg88xx_read(amg88xx_t* this, uint8_t reg, uint8_t* buf, uint8_t num);
i2c_err_t amg88xx_read8(amg88xx_t* this, uint8_t reg, uint8_t* output_value);
i2c_err_t amg88xx_disable_interrupt(amg88xx_t* this);
i2c_err_t amg88xx_enable_interrupt(amg88xx_t* this);
i2c_err_t amg88xx_interrupt_mode_set(amg88xx_t* this, 
                                     amg88xx_interrupt_mode_t mode);
i2c_err_t amg88xx_clear_interrupt(amg88xx_t* this);
i2c_err_t amg88xx_read_thermistor(amg88xx_t* this, float* output_value);
i2c_err_t amg88xx_read_pixels_raw(amg88xx_t* this, 
                                  uint8_t* buf, 
                                  uint8_t count);
i2c_err_t amg88xx_read_pixels(amg88xx_t* this, float* buf, uint8_t count);           
i2c_err_t amg88xx_interrupt_levels_set(amg88xx_t* this,
                                        float high,
                                        float low,
                                        float hysteresis);                
uint8_t amg88xx_interrupt_read(amg88xx_t* this, uint8_t* buf, uint8_t size);  
i2c_err_t amg88xx_moving_average_mode_set(amg88xx_t* this, uint8_t value);

#endif /* ifndef __AMG88XX_H__ */