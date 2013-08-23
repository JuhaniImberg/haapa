#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <event.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "haapa.h"
#include "config.h"

#ifdef INCLUDE_MPD
#include "mpd.h"
#endif

/* Status generation */

void status_time(char* str) {
	time_t timer;
    char buffer[SEGMENT_LENGTH];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, SEGMENT_LENGTH, TIME_FORMAT, tm_info);
    
    strncpy(str, buffer, SEGMENT_LENGTH);
}

void status_load(char* str) {
	FILE* f;
	float loads[3];

	f = fopen("/proc/loadavg", "r");

	if(f == NULL) {
		return;
	}

	fscanf(f, "%f %f %f", &loads[0], &loads[1], &loads[2]);

	fclose(f);

	if(LOAD_TIME != 3) {
		snprintf(str, SEGMENT_LENGTH, "%.2f", loads[LOAD_TIME]);
	}else{
		snprintf(str, SEGMENT_LENGTH, "%.2f %.2f %.2f", loads[0], loads[1], loads[2]);
	}
}

void status_uptime(char* str) {
	FILE* f;
	float uptime = 0;
	time_t timer;
    char buffer[SEGMENT_LENGTH];
    struct tm* tm_info;

	f = fopen("/proc/uptime", "r");

	if(f == NULL) {
		return;
	}

	fscanf(f, "%f", &uptime);

	fclose(f);

    timer = uptime;
    tm_info = localtime(&timer);

    strftime(buffer, SEGMENT_LENGTH, UPTIME_FORMAT, tm_info);

	strncpy(str, buffer, SEGMENT_LENGTH);
}

float cpu_prev_total = 0;
float cpu_prev_idle = 0;

void status_cpu(char* str) {
	FILE* f;
	float total = 0;
	float idle = 0;
	int i;
	float usage;
	float diff_idle;
	float diff_total;

	char ident[5];

	int a[7];

	f = fopen("/proc/stat", "r");

	if(f == NULL) {
		return;
	}

	fscanf(f, "%s %i %i %i %i %i %i %i", 
		ident, &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6]);

	fclose(f);

	idle = a[3];

	for(i = 0; i < 7; i++) {
		total += a[i];
	}

	if(cpu_prev_total > 0) {
		diff_idle = idle - cpu_prev_idle;
		diff_total = total - cpu_prev_total;
		usage = 100 * (diff_total - diff_idle) / diff_total;
		if(CPU_USAGE_DISPLAY==1) {
			status_bar(str, usage);
		}else{
			snprintf(str, SEGMENT_LENGTH, "%.0f%%", usage);
		}
	}else{
		if(CPU_USAGE_DISPLAY==1) {
			status_bar(str, 0);
		}else{
			strncpy(str, "", SEGMENT_LENGTH);
		}
	}

	cpu_prev_total = total;
	cpu_prev_idle = idle;
}

void status_mem(char* str) {
	FILE* f;
	char u[9][16];
	int mem_total, mem_free, mem_buffers, mem_cached, mem_used;

	f = fopen("/proc/meminfo", "r");

	if(f == NULL) {
		return;
	}

	fscanf(f, "%s %i %s %s %i %s %s %i %s %s %i %s", 
		u[0], &mem_total, u[1],
		u[2], &mem_free, u[3],
		u[4], &mem_buffers, u[5],
		u[6], &mem_cached, u[7]);

	fclose(f);

	mem_used = mem_total - mem_free - mem_buffers - mem_cached;

	if(MEM_USAGE_DISPLAY == 2) {
		mem_used /= 1024;
		mem_total /= 1024;
		snprintf(str, SEGMENT_LENGTH, "%i/%i MB", mem_used, mem_total);
	}else if(MEM_USAGE_DISPLAY == 1) {
		status_bar(str, ((float)mem_used/mem_total)*100);
	}else{
		snprintf(str, SEGMENT_LENGTH, "%.0f%%", ((float)mem_used/(float)mem_total)*100);
	}

}

void status_bat(char* str) {
	FILE* f;
	int capacity;
	char capacity_file[128];
	capacity_file[0] = 0;
	strncat(capacity_file, BAT_LOCATION, 64);
	strncat(capacity_file, "capacity", 64);

	f = fopen(capacity_file, "r");

	if(f == NULL) {
		return;
	}

	fscanf(f, "%i", &capacity);

	fclose(f);

	if(BAT_DISPLAY==1) {
		status_bar(str, capacity);
	}else{
		snprintf(str, SEGMENT_LENGTH, "%i%%", capacity);
	}
}

void status_bat_status(char* str) {
	FILE* f;
	char status[64];
	char status_file[128];
	status_file[0] = 0;
	strncat(status_file, BAT_LOCATION, 64);
	strncat(status_file, "status", 64);

	f = fopen(status_file, "r");

	if(f == NULL) {
		return;
	}

	fscanf(f, "%s", status);

	fclose(f);

	snprintf(str, SEGMENT_LENGTH, "%s", status);
}

void status_ip(char* str) {
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, IP_INTERFACE, IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	/* display result */
	snprintf(str, SEGMENT_LENGTH,
		"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

}

/* Status helpers */

void status_bar(char* str, float value) {
	static int bar_length = 11;
	char bar[bar_length];
	int i;
	float tmp_val = round(value/ (float)(bar_length-1));

	bar[0] = 0;

	for(i = 1; i < bar_length; i++) {
		if(i <= tmp_val) {
			strcat(bar, "#");
		}else{
			strcat(bar, "_");
		}
	}

	snprintf(str, SEGMENT_LENGTH, BAR_FORMAT, bar);

}

void status_text(char* str, char* in) {
	strncpy(str, in, SEGMENT_LENGTH);
}

/* i3 helpers */

void status_i3_start(char* str) {
	if(I3_ENABLED){
		strcpy(str, "[{\"full_text\":\" \"}\n");
	}
}

void status_i3_block(char* str) {
	if(I3_ENABLED) {
		strncpy(str, ",{\"full_text\": \"", 24);
	}
}

void status_i3_color(char* str, char* color) {
	if(I3_ENABLED) {
		snprintf(str, 32, "\", \"color\": \"%s\"}\n", color);
	}
}

void status_i3_end(char* str) {
	if(I3_ENABLED) {
		strcpy(str, "],");
	}
}

void flush(char* buffer, char* str) {
	strncat(buffer, str, SEGMENT_LENGTH);
	if(I3_ENABLED==0) {
		strncat(buffer, SEGMENT_SEPERATOR, SEGMENT_LENGTH);
	}
	str[0] = 0;
}

void flush2(char* buffer, char* str) {
	strncat(buffer, str, SEGMENT_LENGTH);
	str[0] = 0;
}

void flush3(char* buffer, char* str) {
	strncat(buffer, str, SEGMENT_LENGTH);
	strncat(buffer, SEGMENT_SEPERATOR, SEGMENT_LENGTH);
	str[0] = 0;
}

void tick(int fd, short event, void* arg) {

	char buffer[1024];
	char result[SEGMENT_LENGTH];

	buffer[0] = 0;
	result[0] = 0;
	OUTPUT

	if(FORMAT==1) {
		printf("\r%s", buffer);
		fflush( stdout );
	}
	else{
		printf("%s\n", buffer);
		fflush( stdout );
	}
}

int main(int argc, const char* argv[]) {
	struct event ev;
	struct timeval tv;

	tv.tv_sec = INTERVAL;
	tv.tv_usec = 0;

	if(I3_ENABLED) {
		printf("{\"version\":1}\n[[{\"full_text\":\"hello!\"}],");
		fflush(stdout);
	}

	event_init();
	event_set(&ev, 0, EV_PERSIST, tick, NULL);
	evtimer_add(&ev, &tv);
	event_dispatch();

	return 0;
}
