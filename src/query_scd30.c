#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <string.h>
#include "scd30.h"
#include <time.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <getopt.h>

#define PORT_DEFAULT 8080
#define INTERVAL_DEFAULT 30

// global vars

pthread_mutex_t lock;
struct scd30_data sensor_data;
int port, interval;

void handle(int client_fd) {
	char buffer[1024];
	read(client_fd, buffer, sizeof(buffer) - 1);

	if (strncmp(buffer, "GET /", 5) == 0) {
		char body[128];
		char response[512];
		pthread_mutex_lock(&lock);
		snprintf(body, sizeof(body), "{\"temp\": %f, \"humidity\": %f, \"co2\": %f}", sensor_data.temp, sensor_data.humidity, sensor_data.co2);
		pthread_mutex_unlock(&lock);

		snprintf(response, sizeof(response),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: %ld\r\n"
		"Connection: close\r\n"
		"\r\n"
		"%s", strlen(body), body);

		send(client_fd, response, strlen(response), 0);
	}
	close(client_fd);
}

void *serve(void *args) {

	int server_fd, client_fd;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	int opt = 1;
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
	listen(server_fd, 10);

	while (1) {
		client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
		if (client_fd >= 0) handle(client_fd);
	}
	return NULL;
}

int main(int argc, char** argv) {

	// defautl value
	interval = INTERVAL_DEFAULT;
	port = PORT_DEFAULT;
	// parse arguments
	int c; 
	while ((c = getopt(argc, argv, "i:p:")) != -1)
		switch(c) {
			case 'i': interval = atoi(optarg);
			break;
			case 'p': port = atoi(optarg);
			break;
			case '?': fprintf(stderr, "Unknown option %s\n", optopt); exit(1);
		 }

//  initiate connection
    scd30_init();

   pthread_t server_thread;
   pthread_create(&server_thread, NULL, serve, NULL);

    while (1) {
        while (!scd30_get_data_ready_status()) {
            sleep(interval); // query only every 10 seconds
        }

	pthread_mutex_lock(&lock);
        if (scd30_read_measurement(&sensor_data) < 0) {
            perror("error reading measurements");
	    continue;
        }
	pthread_mutex_unlock(&lock);
  }

 pthread_join(server_thread, NULL);

}





