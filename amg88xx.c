/* amg88xx.c -- AMG88xx implementation for CH32V003.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "amg88xx.h"

const uint8_t AMG88XX_ADDRESS = 0x69;
const uint8_t AMG88XX_PIXEL_ARRAY_SIZE = 64;
const float AMG88XX_PIXEL_TEMP_CONVERSION = 0.25;
const float AMG88XX_THERMISTOR_CONVERSION = 0.0625;

/**
 * @brief Convert a 12-bit signed magnitude value to a floating point number.
 * @param value The 12-bit signed magnitude value to be converted.
 * @return The converted floating point value.
 */
float signed_magnitude12_to_float(uint16_t value) {
        // take first 11 bits as absolute val
        uint16_t abs_value = (value & 0x7FF);

        return (value & 0x800) ? 0 - (float) abs_value : (float) abs_value;
}

/**
 * @brief Convert a 12-bit integer two's complement value to a
 *   floating point number.
 * @param value A 12-bit two's compliment value to be converted.
 * @return A converted floating point value.
 */
float int12_to_float(uint16_t value) {
        int16_t val = (value << 4);
        return val >> 4;
}

/**
 * @brief Write an one-byte unsigned value to the specified register
 *   of the AMS88XX.
 * @param this An AMG88XX device.
 * @param reg The register to write to.
 * @param input_value The value to write.
 * @return I2C_OK on succes, error code otherwise.
 */
i2c_err_t amg88xx_write8(amg88xx_t* this, uint8_t reg, uint8_t input_value) {
        return i2c_write_reg(&(this->device), reg, &input_value, 1);
}

/**
 * @brief Read an one-byte unsigned value from the specified register
 *   of the AMS88XX.
 * @param reg The register to read from.
 * @param output_value The output value to write to.
 * @return I2C_OK on succes, error code otherwise.
 */
i2c_err_t amg88xx_read8(amg88xx_t* this, uint8_t reg, uint8_t* output_value) {
        return i2c_read_reg(&(this->device), reg, output_value, 1);
}

/**
 * Read data from the device.
 * @param this An AMG88XX device.
 * @param reg Register to read.
 * @param buf Buffer to write data to.
 * @param num Number of bytes to read.
 * @return I2C_OK on succes, error code otherwise.
 */
i2c_err_t amg88xx_read(amg88xx_t* this, uint8_t reg, uint8_t* buf,
                       uint8_t num) {
        i2c_err_t rc = i2c_write_raw(&(this->device), &reg, 1);
        if (rc != I2C_OK) {
                return rc;
        }
        return i2c_read_raw(&(this->device), buf, num);
}

/**
 * @brief Write data to the device instance.
 * @param this An AMG88XX device.
 * @param reg Register to write.
 * @param buf Buffer to write data from.
 * @param num Number of bytes to write.
 * @return I2C_OK on succes, error code otherwise.
 */
i2c_err_t amg88xx_write(amg88xx_t* this, uint8_t reg, uint8_t* buf,
                        uint8_t num) {
        i2c_err_t rc = i2c_write_raw(&(this->device), &reg, 1);
        if (rc != I2C_OK) {
                return rc;
        }
        return i2c_write_raw(&(this->device), buf, num);
}

/**
 * @brief Initialize an AMG88XX device instance.
 * @param this An AMG88XX device instance.
 * @param address I2C address of AMG88XX.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_init(amg88xx_t* this, uint16_t address) {
        i2c_err_t rc;

        this->device.addr = address;
        this->device.clkr = I2C_CLK_400KHZ;
        this->device.type = I2C_ADDR_7BIT;
        this->device.addr = 0x69;
        this->device.regb = 1;
        this->device.tout = 2000;

        rc = i2c_init(&(this->device));
        if (rc != I2C_OK) {
                goto end;
        }
        Delay_Ms(100);

        this->pctl = AMG88XX_MODE_NORMAL;
        rc = amg88xx_write8(this, AMG88XX_PCTL, this->pctl);
        if (rc != I2C_OK) {
                goto end;
        }

        this->rst = AMG88XX_INITIAL_RESET;
        rc = amg88xx_write8(this, AMG88XX_RST, this->rst);
        if (rc != I2C_OK) {
                goto end;
        }

        rc = amg88xx_disable_interrupt(this);
        if (rc != I2C_OK) {
                goto end;
        }

        this->fps = AMG88XX_FPS_10;
        rc = amg88xx_write8(this, AMG88XX_FPSC, this->fps);
        if (rc != I2C_OK) {
                goto end;
        }

        Delay_Ms(100);
end:
        return rc;
}

/**
 * @brief Disable the interrupt pin on the device.
 * @param this An AMG88XX device instance.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_disable_interrupt(amg88xx_t* this) {
        this->intc &= 0b10;
        return amg88xx_write8(this, AMG88XX_INTC, this->intc);
}

/**
 * @brief Enable the interrupt pin on the device.
 * @param this An AMG88XX device instance.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_enable_interrupt(amg88xx_t* this) {
        this->intc |= 1;
        return amg88xx_write8(this, AMG88XX_INTC, this->intc);
}

/**
 * @brief Set the interrupt mode.
 * @param this An AMG88XX device instance.
 * @param mode Interrupt mode.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_interrupt_mode_set(amg88xx_t* this,
                                     amg88xx_interrupt_mode_t mode) {
        this->intc = (mode << 1) | (this->intc & 0b1);
        return amg88xx_write8(this, AMG88XX_INTC, this->intc);
}


uint8_t amg88xx_interrupt_mode_get(amg88xx_t* this) {
        return this->intc >> 1;
}

/**
 * @brief Read the state of the triggered interrupts on the device.
 * @param this An AMG88XX device instance.
 * @param buf The pointer to the memory where the data will be stored.
 * @param size Number of bytes to read.  Default is 8 bytes.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
uint8_t amg88xx_interrupt_read(amg88xx_t* this, uint8_t* buf, uint8_t size) {
        uint8_t bytes_to_read = fmin(size, (uint8_t) 8);
        return amg88xx_read(this, AMG88XX_INT_OFFSET, buf, bytes_to_read);
}

/**
 * Clear any triggered interrupts.
 * @param this An AMG88XX device instance.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_clear_interrupt(amg88xx_t* this) {
        this->rst = AMG88XX_FLAG_RESET;
        return amg88xx_write8(this, AMG88XX_RST, this->rst);
}

/**
 * @brief Set the interrupt levels.
 * @param this An AMG88XX device instance.
 * @param high The value above which an interrupt will be triggered.
 * @param low The value below which an interrupt will be triggered.
 * @param hysteresis The hysteresis value for interrupt detection.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_interrupt_levels_set(amg88xx_t* this,
                                       float high,
                                       float low,
                                       float hysteresis) {
        i2c_err_t rc;
        int32_t high_conv = high / AMG88XX_PIXEL_TEMP_CONVERSION;
        high_conv = constrain(high_conv, -4095, 4095);
        this->inthl = high_conv & 0xFF;
        this->inthh = (high_conv & 0x0F00) >> 8;
        rc = amg88xx_write8(this, AMG88XX_INTHL, this->inthl);
        if (rc != I2C_OK) {
                goto end;
        }
        rc = amg88xx_write8(this, AMG88XX_INTHH, this->inthh);
        if (rc != I2C_OK) {
                goto end;
        }
        int32_t low_conv = low / AMG88XX_PIXEL_TEMP_CONVERSION;
        low_conv = constrain(low_conv, -4095, 4095);
        this->intll = low_conv & 0xFF;
        this->intlh = (low_conv & 0x0F00) >> 8;
        rc = amg88xx_write8(this, AMG88XX_INTLL, this->intll);
        if (rc != I2C_OK) {
                goto end;
        }
        rc = amg88xx_write8(this, AMG88XX_INTLH, this->intlh);
        if (rc != I2C_OK) {
                goto end;
        }
        int hysteresis_conv = hysteresis / AMG88XX_PIXEL_TEMP_CONVERSION;
        hysteresis_conv = constrain(hysteresis_conv, -4095, 4095);
        this->ihysl = hysteresis_conv & 0xFF;
        this->ihysh = (hysteresis_conv & 0x0F00) >> 8;
        rc = amg88xx_write8(this, AMG88XX_IHYSL, this->ihysl);
        if (rc != I2C_OK) {
                goto end;
        }
        rc = amg88xx_write8(this, AMG88XX_IHYSH, this->ihysh);
        if (rc != I2C_OK) {
                goto end;
        }

end:
        return rc;
}

/**
 * @brief Set the moving average mode.
 * @param this An AMG88XX device instance.
 * @param value If 1 (true) is passed, the output will be twice
 *   the moving average.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_moving_average_mode_set(amg88xx_t* this, uint8_t value) {
        if (value) {
                this->ave |= (1 << 5);
        } else {
                this->ave &= ~(1 << 5);
        }
        return amg88xx_write8(this, AMG88XX_AVE, this->ave);
}

/**
 * @brief Read the onboard thermistor.
 * @param this An AMG88XX device instance.
 * @param output_value A pointer to a float value to store the current
 *   temperature in degrees of Celsius.
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_read_thermistor(amg88xx_t* this, float* output_value) {
        enum { RAW_SIZE = 2 };
        uint8_t raw_value[RAW_SIZE];
        i2c_err_t rc = amg88xx_read(this, AMG88XX_TTHL, raw_value, RAW_SIZE);
        if (rc != I2C_OK) {
                return rc;
        }
        uint16_t recast = ((uint16_t)raw_value[1] << 8)
                | ((uint16_t)raw_value[0]);
        *output_value = signed_magnitude12_to_float(recast)
                * AMG88XX_THERMISTOR_CONVERSION;
        return rc;
}

/**
 * @brief Read infrared sensor raw values.
 * @param this An AMG88XX device instance.
 * @param buf A buffer to store the pixel values in.  Each pixel takes 12 bits
 *   of memory so it is stored as a 2-byte value
 * @param count Number of pixels to read (up to 64.)
 * @return An I2C_OK on success, I2C error code otherwise.
 */
i2c_err_t amg88xx_read_pixels_raw(amg88xx_t* this,
                                  uint8_t* buf,
                                  uint8_t count) {
        uint8_t bytes_to_read = fmin((uint8_t) (count << 1),
                                     (uint8_t) (AMG88XX_PIXEL_ARRAY_SIZE << 1));
        return amg88xx_read(this, AMG88XX_PIXEL_OFFSET,
                            buf, bytes_to_read);
}

/**
 * Read infrared sensor values.
 * @param this An AMG88XX device instance.
 * @param buf A buffer to store the pixel values in.
 * @param count Number of pixels to read (up to 64.)
 */
i2c_err_t amg88xx_read_pixels(amg88xx_t* this, float* buf, uint8_t count) {
        uint8_t bytes_to_read = fmin((uint8_t) (count << 1),
                                     (uint8_t) (AMG88XX_PIXEL_ARRAY_SIZE << 1));
        uint8_t raw_values[bytes_to_read];
        uint8_t pos;
        uint16_t recast;
        i2c_err_t rc = amg88xx_read(this, AMG88XX_PIXEL_OFFSET,
                                    raw_values, bytes_to_read);
        if (rc != I2C_OK) {
                return rc;
        }
        for (uint8_t index = 0; index < count; index++) {
                pos = index << 1;
                recast = ((uint16_t)raw_values[pos + 1] << 8)
                        | ((uint16_t)raw_values[pos]);
                buf[index] = int12_to_float(recast)
                        * AMG88XX_PIXEL_TEMP_CONVERSION;
        }
        return rc;
}

/**
 * Read infrared sensor values as integers.
 *
 * @param this An AMG88XX device instance.
 * @param buf A buffer to store the pixel values in.
 * @param count Number of pixels to read (up to 64.)
 */
i2c_err_t amg88xx_read_pixels_int(amg88xx_t* this, int16_t* buf, 
        uint8_t count)
{
        uint8_t bytes_to_read = fmin((int16_t) (count << 1),
                                     (int16_t) (AMG88XX_PIXEL_ARRAY_SIZE << 1));
        uint8_t raw_values[bytes_to_read];
        uint8_t pos;
        int16_t recast;
        i2c_err_t rc = amg88xx_read(this, AMG88XX_PIXEL_OFFSET,
                                    raw_values, bytes_to_read);
        if (rc != I2C_OK) {
                return rc;
        }
        for (uint8_t index = 0; index < count; index++) {
                pos = index << 1;
                recast = ((int16_t)raw_values[pos + 1] << 8)
                        | ((int16_t)raw_values[pos]);
                buf[index] = recast * AMG88XX_PIXEL_TEMP_CONVERSION;
        }
        return rc;
}

/* amg88xx.c ends here. */
