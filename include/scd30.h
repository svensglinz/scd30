
/* -------------------------------------------------------------
 * This library facilitates communication with the Sensirion SCD30 Sensor via
 * the i2c protocol All functions are implemented according to the interface
 * description in https://sensirion.com/media/documents/D7CEEF4A/6165372F/Sensirion_CO2_Sensors_SCD30_Interface_Description.pdf
 * By default, scd_init() connects to the i2c bus
 * via the file /dev/i2c-1 and to the device address 0x61. These values can be
 * changed by redefining the I2C_PATH and SCD30_ADDR macros
 * -------------------------------------------------------------*/

#ifndef SCD30_H_
#define SCD30_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// device configuration
#define SCD30_ADDR 0x61
#define I2C_PATH "/dev/i2c-1"

// sensor commands
#define SCD30_STOP_CONTINUOUS_MEASURE 0x0104
#define SCD30_GET_DATA_READY_STATUS 0x0202
#define SCD30_READ_MEASUREMENT 0x0300
#define SCD30_REAR_FIRMWARE_VERSION 0xD100
#define SCD30_SOFT_RESET 0xD304
#define SCD30_SET_TEMPERATURE_OFFSET 0x5403
#define SCD30_SET_ALTITUDE_COMPENSATION 0x5102
#define SCD30_SET_FORCED_RECALIBRATION_VALUE 0x5204
#define SCD30_SET_MEASUREMENT_INTERVAL 0x4600
#define SCD30_SET_AUTOMATIC_SELF_CALIBRATION 0x5306
#define SCD30_START_CONTINUOUS_MEASURE 0x0010
// Struct that holds the sensor's measurement
struct scd30_data{
   float co2;
   float temp;
   float humidity;
};

/*
 * @brief Initialize device
 *
 * Conects to the sensor via the address and filepath defined in SCD30_ADDR and I2C_PATH
 *
 * @return 0 on success, -1 on failure
 */
extern int scd30_init();

/*
 * @brief Stops the continuous measurement of the SCD30
 *
 * Stop the continuous measurement of the SCD30. Use scd30_start_continuous_measure() to restart
 *
 * @return 0 on success, -1 on failure
 *
 */
extern int scd30_stop_continous_measure();


/*
 * @brief Trigger continuous measurement with optional ambient pressure compensation
 *
 * Starts continuous measurement of the SCD30 to measure CO2 concentration, humidity and temperature.
 *
 * The CO2 measurement value can be compensated for ambient pressure by
 * feeding the pressure value in mBar to the sensor.
 * Setting the ambient pressure will overwrite previous settings of
 * altitude compensation. Setting the argument to zero will
 * deactivate the ambient pressure compensation (default ambient pressure
 * = 1013.25 mBar).
 *
 * @return 0 on succcess, -1 on error
*/
extern int scd30_start_continuous_measure(uint16_t pressure);

/*
 * @brief poll data-ready flag
 *
 * Checks whether the sensor is able to deliver a new set of measurements.
 * The default measurement interval is 2s, but can be changed with scd30_set_measurement_interval
 *
 * The data-ready flag received from the sensor is checked for integrity using the CRC check as described in
 * .... In case an inconsistency is detected, the function returns 0 (data not ready) and not an error.
 *
 * @return 1 if data is ready, 0 if not ready, -1 on error
 */
extern int scd30_get_data_ready_status();

/*
 * @brief read sensor measurements
 *
 * Reads out the sensor's CO2 / Humidity and Temperature measurements and stores them in the
 * scd30_data struct.
 *
 * This function should only be used immediately after scd_30_get_data_ready_status returns 1,
 * which indicates that the sensor has data ready be be read out.
 *
 * @param buf struct that holds CO2 / Humidity and Temperature values
 * @return 0 on success, -1 otherwise
 */
extern int scd30_read_measurement(struct scd30_data *buf);

/*
 * @brief perform soft reset
 *
 * The SCD30 provides a soft reset mechanism that forces the sensor int
 * the same state as after powering up without the need
 * for removing the power-supply. It does so by restarting its system
 * controller. After soft reset the sensor will reload all calibrated
 * data. However, it is worth noting that the sensor reloads calibration
 * data prior to every measurement by default. This includes
 * previously set reference values from ASC or FRC as well as temperature
 * offset values last setting.
 *
 * @return 0 on success, -1 otherwise
 */
extern int scd30_soft_reset();

/*
 * @brief set temperature offset
 *
 * The on-board RH/T sensor is influenced by thermal self-heating of
 * SCD30 and other electrical components. Design-in alters the
 * thermal properties of SCD30 such that temperature and humidity offset
 * may occur when operating the sensor in end-customer
 * devices. Compensation of those effects is achievable by writing the
 * temperature offset found in continuous operation of the
 * device into the sensor.
 * Temperature offset value is saved in non-volatile memory. The last set
 * value will be used for temperature offset compensation
 * after repowering.
 *
 * @param offset offset in units of 0.01 degrees celcius
 *
 * @return 0 on success, -1 otherwise
 */
extern int scd30_set_temperature_offset(uint16_t offset);

/*
 * @brief get temperature offset
 *
 *  get the temperature offset that is currently saved
 *  on the device in 0.01 degrees celcius
 *
 *  @param offset temperature offset in 0.01 degrees
 *
 *  @returns 0 on success, -1 otherwise
 */
extern int scd30_get_temperature_offset(uint16_t *offset);

/*
 * @brief get the set altitude compensation (in meters)
 * @param offset
 * @return 0 on success, -1 on error
*/
extern int scd30_get_altitude_compensation(uint16_t *offset);

/*
 * @brief set altitude compensation
 *
 * Measurements of CO2 concentration based on the NDIR
 * principle are influenced by altitude. SCD3 offers to compensate
 * deviations due to altitude by using the following command.
 * Setting altitude is disregarded when an ambient pressure is given tot he sensor
 *
 * Altitude value is saved in non-volatile memory.
 * The last set value will be used for altitude compensation after repowering
 *
 * @param offset altitude offset in meters above sea level
 *
 * @returns 0 on success, -1 otherwise
 */
extern int scd30_set_altitude_compensation(uint16_t offset);

/*
 * @brief adjust measurement interval
 *
 * Sets the interval used by the SCD30 sensor to measure in continuous
 * measurement mode. The Initial value is
 * 2 s. The chosen measurement interval is saved in non-volatile memory
 * and thus is not reset to its initial value after power up.
 *
 * The provided interval must be between 2 and 1800 seconds.
 *
 * @param interval new measurement interval in seconds
 */
extern int scd30_set_measurement_interval(uint16_t interval);

/*
 * @brief return measurement interval in seconds
 *
 * @param interval
 * @return 0 on success, -1 on error
 */
extern int scd30_get_measurement_interval(uint16_t *interval);

/*
 *
 */
extern int scd30_set_automatic_self_calibration(uint16_t status);

/*
 * @brief
 */
extern int scd30_get_automatic_self_calibration(uint16_t *status);

/*
 * @brief set forced recalibration value
 * Forced recalibration (FRC) is used to compensate for sensor drifts when a reference value of the CO2 concentration in close
 * proximity to the SCD30 is available. For best results, the sensor has to be run in a stable environment in continuous mode at a
 * measurement rate of 2s for at least two minutes before applying the FRC command and sending the reference value. Setting a
 * reference CO2 concentration by the method described here will always supersede corrections from the ASC and vice-versa.
 *
 * The FRC method imposes a permanent update of the CO2 calibration curve which persists after repowering the sensor. The
 * most recently used reference value is retained in volatile memory and can be read out with the command sequence given below.
 * After repowering the sensor, the command will return the standard reference value of 400 ppm.
 *
 *@return 0 on success, -1 on error
*/
extern int scd30_set_forced_recalibration_value(uint16_t concentration);


/*
 * @brief return the forced recalibration value
*/
extern int scd30_get_forced_recalibration_value(uint16_t *concentration);


/*
 * @brief read firmware version of the sensor
 */
extern int scd30_read_firmware_version();

#ifdef __cplusplus
}
#endif

#endif
