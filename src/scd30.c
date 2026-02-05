#include "scd30.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* -------------------------------------------------------------
 * This library facilitates communication with the Sensirion SCD30 Sensor via
 * the i2c protocol All functions are implemented according to the interface
 * description in LINK GOES HERE By default, scd_init() connects to the i2c bus
 * via the file /dev/i2c-1 and to the device address 0x61. These values can be
 * changed by redefining the I2C_PATH and SCD30_ADDR macros
 * -------------------------------------------------------------*/

typedef struct {
	int fd;
	uint8_t address; 
} scd30_device_t;

#define SCD30_CHECKSUM_INIT 0xFF
#define SCD30_CHECKSUM_POLY 0x31

int scd30_calculate_crc(uint8_t*, size_t);
float scd30_convert_measure(uint8_t *data);
int scd30_write_cmd(uint16_t);
int scd30_write_cmd_arg(uint16_t, uint16_t);

// instantiate file descriptor to SCD30
int scd30_init(scd30_device_t* handle) {
  // set address ? 
  if ((handle->fd = open(I2C_PATH, O_RDWR)) < 0) {
    return -1;
  }
  if ((ioctl(handle->fd, I2C_SLAVE, handle->address)) < 0) {
    return -1;
  }
  return 0;
}

int scd30_start_continuous_measure(sdc30_device_t* handle, uint16_t pressure) {
	uint16_t pressure_no = htons(pressure);
// build in error handling
	scd30_write_cmd_arg(SCD30_START_CONTINUOUS_MEASURE, pressure_no);
	return 0;
}
// write 2 byte command to device
int scd30_write_cmd(scd30_device_t* handle, uint16_t cmd) {
    // convert to network order
    uint16_t cmd_no = htons(cmd);
    if (write(scd30_fd, &cmd_no, 2) < 0) {
        return -1;
    }
    return 0;
}

// write 2 byte command and 2 byte argument to device
int scd30_write_cmd_arg(scd30_device_t* handle, uint16_t cmd, uint16_t arg) {
    // convert to network order
    uint16_t cmd_no = htons(cmd);
    uint16_t arg_no = htons(arg);

    uint8_t check = scd30_calculate_crc((uint8_t*) &arg_no, 2);

    uint16_t buf[] = {cmd_no, arg_no, check};
    if (write(scd30_fd, buf, 5) < 0) {
        return -1;
    }
    return 0;
}

int scd30_read(scd30_device_t* handle, void *buf, size_t size) {
    if (read(scd30_fd, buf, size) < 0) {
        return -1;
    }
    return 0;
}

// calculate the crc bit by bit
int scd30_calculate_crc(uint8_t *data, size_t len) {
  uint8_t crc = SCD30_CHECKSUM_INIT;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ SCD30_CHECKSUM_POLY;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

/*
 * compare received crc to calculated crc
 * Return 1 on match, 0 on mismatch
 */
int scd30_validate_crc(uint8_t *data, size_t len, uint8_t checksum) {
  int crc = scd30_calculate_crc(data, len);
  return crc == checksum;
}

// Send stop signal to the sensor to stop measuring data
int scd30_stop_continous_measure(scd30_device_t* handle) {
   if (scd30_write_cmd(SCD30_STOP_CONTINUOUS_MEASURE) < 0) {
    return -1;
  }
  return 0;
}

/*
 * check if data can be read from the sensor.
 * Return 1 if ready, 0 if not ready
 */
int scd30_get_data_ready_status(scd30_device_t* handle) {
  uint8_t read_buff[3];

  // send request
  if (scd30_write_cmd(SCD30_GET_DATA_READY_STATUS) < 0) {
    return -1;
  }

  usleep(10000);

  // read data
  if (read(scd30_fd, read_buff, sizeof(read_buff)) < 0) {
    return -1;
  }

  // return not ready on crc
  if (scd30_validate_crc(read_buff, 2, read_buff[2]) == 0) {
    return 0;
  }
  // return lsb of result (1 / 0)
  return read_buff[1];
}

// reinterpret big endian 4 byte value (without cast) values as float
float scd30_convert_measure(uint8_t *data) {
  uint32_t data_no = *(uint32_t *)data;
  uint32_t data_ho = ntohl(data_no);
  return *(float *)&data_ho;
}

// read sensor values into scd30_data struct
int scd30_read_measurement(scd30_device_t* handle, struct scd30_data *buf) {
  uint8_t read_buff[18];

  // send read command
  if (scd30_write_cmd(SCD30_READ_MEASUREMENT) < 0) {
      return -1;
    }

  // minimum waiting period
  usleep(10000);
  if (read(scd30_fd, &read_buff, sizeof(read_buff)) < 0) {
      return -1;
    }

  // evaluate data & check checksum
  for (int i = 0; i < 6; i++) {
    if (scd30_validate_crc(&read_buff[3 * i], 2, read_buff[3 * i + 2]) == 0) {
      return -1;
    }
  }

  // convert co2
  uint8_t co2_vals[] = {read_buff[0], read_buff[1], read_buff[3], read_buff[4]};
  float co2 = scd30_convert_measure(co2_vals);

  // convert humidity
  uint8_t hum_vals[] = {read_buff[12], read_buff[13], read_buff[15],
                        read_buff[16]};
  float humidity = scd30_convert_measure(hum_vals);

  // convert temperature
  uint8_t temp_vals[] = {read_buff[6], read_buff[7], read_buff[9],
                         read_buff[10]};
  float temp = scd30_convert_measure(temp_vals);

  buf->temp = temp;
  buf->humidity = humidity;
  buf->co2 = co2;

  return 0;
}

int scd30_soft_reset(scd30_device_t* handle) {
  if (scd30_write_cmd(SCD30_SOFT_RESET) < 0) {
    return -1;
  }
  return 0;
}

int scd30_set_temperature_offset(uint16_t offset) {

  if (scd30_write_cmd_arg(SCD30_SET_TEMPERATURE_OFFSET, offset) < 0) {
    return -1;
  }

  // confirm update
  uint16_t cur_offset;
  if (scd30_get_temperature_offset(&cur_offset) < 0) {
    return -1;
  }

  return (cur_offset == offset)? 0 : -1;
}


int scd30_get_temperature_offset(scd30_device_t* handle, uint16_t *offset) {
    uint8_t buf[3];

    if (scd30_write_cmd(SCD30_SET_TEMPERATURE_OFFSET) < 0) {
        return -1;
    }

    usleep(10000);

    if (scd30_read(buf, 2) < 0) {
      return -1;
    }
    // validate checksum
    uint16_t res = ntohs(*(uint16_t*)buf);
    *offset = res;
    return 0;
}

int scd30_set_altitude_compensation(scd30_device_t* handle, uint16_t offset) {

  if (scd30_write_cmd_arg(SCD30_SET_ALTITUDE_COMPENSATION, offset) < 0) {
    return -1;
  }

  usleep(10000);

  uint16_t cur_offset;
  if (scd30_get_altitude_compensation(&cur_offset) < 0) {
    return -1;
  }

  return (offset == cur_offset)? 0 : -1;
}

int scd30_get_altitude_compensation(scd30_device_t* handle, uint16_t *offset) {
  if (scd30_write_cmd(SCD30_SET_ALTITUDE_COMPENSATION) < 0) {
    return -1;
  }
  usleep(10000);

  uint8_t buf[2];
  if (scd30_read(buf, 2) < 0) {
    return -1;
  }

  uint16_t res = ntohs(*(uint16_t *)buf);
  *offset = res;
  return 0;
}

int scd30_set_forced_recalibration_value(scd30_device_t* handle, uint16_t concentration) {
  if (scd30_write_cmd_arg(SCD30_SET_FORCED_RECALIBRATION_VALUE, concentration) < 0) {
    return -1;
  }

  // validation ?
    return 0;
}

int scd30_get_forced_recalibration_value(scd30_device_t* handle, uint16_t *concentration) {
  if (scd30_write_cmd(SCD30_SET_FORCED_RECALIBRATION_VALUE) < 0) {
    return -1;
  }

  uint8_t buf[2];
  if (scd30_read(buf, 2) < 0) {
    return -1;
  }

  uint16_t res = ntohs(*(uint16_t *)buf);
  *concentration = res;
  return 0;
}

int scd30_set_measurement_interval(scd30_device_t* handle, uint16_t interval) {

  if (scd30_write_cmd_arg(SCD30_SET_MEASUREMENT_INTERVAL, interval) < 0) {
    return -1;
  }

  // verify
  uint16_t cur_interval;
  if (scd30_get_measurement_interval(&cur_interval) < 0) {
    return -1;
  }

  return (cur_interval == interval) ? 0 : -1;
}

int scd30_get_measurement_interval(scd30_device_t* handle, uint16_t *interval) {
  if (scd30_write_cmd(SCD30_SET_MEASUREMENT_INTERVAL) < 0) {
    return -1;
  }

  uint8_t buf[2];
  if (scd30_read(buf, 2) < 0) {
    return -1;
  }

  uint16_t res = ntohs(*(uint16_t *)buf);
  *interval = res;
  return 0;
}


int scd30_set_automatic_self_calibration(scd30_device_t* handle, uint16_t status) {
  if (scd30_write_cmd_arg(SCD30_SET_AUTOMATIC_SELF_CALIBRATION, status) < 0) {
    return -1;
  }

  // validate
  uint16_t cur_status;
  if (scd30_get_automatic_self_calibration(&cur_status) < 0) {
    return -1;
  }

  return (cur_status == status)? 0 : -1;
}

int scd30_get_automatic_self_calibration(scd30_device_t* handle, uint16_t *status) {
  if (scd30_write_cmd(SCD30_SET_AUTOMATIC_SELF_CALIBRATION) < 0) {
    return - 1;
  }

  uint8_t buf[2];
  if (scd30_read(buf, 2) < 0) {
    return -1;
  }

  uint16_t res = ntohs(*(uint16_t *)buf);
  *status = res;
  return 0;
}
