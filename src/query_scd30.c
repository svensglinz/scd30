#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <string.h>
#include "scd30.h"
#include <time.h>

#define SHM_NAME "/sensor_data"
#define SHM_SIZE (sizeof(struct scd30_data))
#define MAX_RETRY 20

#define MIN_NOTIFY_INTERVAL (30*60)
#define CO2_NOTIFY_LIMIT 1000

static time_t cur_time;
static time_t last_notify = 0;

int send_gotify_notification(double co2) {
		printf("dispatching notification\n");
		char command[512];  // Ensure enough space for the entire command
		snprintf(command, sizeof(command),
         	"curl \"http://192.168.1.124:6001/message?token=Ab.NVc7gVczXseL\" "
         	"-F \"title=Bad Air Quality\" "
         	"-F \"message=Co2 Level = %f\" "
         	"-F \"priority=5\"",
         	co2);
		system(command);
}

int main() {
    uint8_t data[10];
	
    // create shared memory region
    int shm_fd;
    if ((shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666)) < 0) {
        perror("creating shared memory failed");
        return -1;
    }

    // set size of memory region
    if (ftruncate(shm_fd, sizeof(struct scd30_data)) < 0) {
        perror("resizing shared memory failed");
        exit(1);
    }

    void *shm_ptr;
    if ((shm_ptr = mmap(NULL, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0)) < 0) {
        perror("memory mapping failed");
        close(shm_fd);
        return -1;
    }

    // initiate connection
    scd30_init();

    // READ OUT VALUES
    while (1) {
        while (!scd30_get_data_ready_status()) {
            sleep(10); // query only every 10 seconds
        }

        struct scd30_data sensor_data;

        if (scd30_read_measurement(&sensor_data) < 0) {
            perror("error reading measurements");
        }

        // process data from the sensor!
        printf("temperature read: %f\n", sensor_data.temp);
        printf("humidity read: %f\n", sensor_data.humidity);
        printf("co2 read: %f\n", sensor_data.co2);
	
	// dispatch notification 
	time(&cur_time);
	if (sensor_data.co2 >= CO2_NOTIFY_LIMIT && cur_time - MIN_NOTIFY_INTERVAL >= last_notify) {
		send_gotify_notification(sensor_data.co2);
		last_notify = cur_time;
	}

        // write result to shared memory
        int retry = 0;
        while (flock(shm_fd, LOCK_EX) == -1 && retry < MAX_RETRY) {usleep(1000);}

        if (retry >= MAX_RETRY) {
            perror("Failed to acquire write lock");
            continue;
        }
        memcpy(shm_ptr, &sensor_data, sizeof(struct scd30_data));
        flock(shm_fd, LOCK_UN); 
  }

    // clean up
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(SHM_NAME);
    close(shm_fd);
}





