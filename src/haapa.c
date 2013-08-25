#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <event.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#include "haapa.h"
#include "config.h"

#include "result.h"
#include "proc.h"
#include "time.h"
#include "battery.h"
#include "network.h"

char buffer[1024];

void start_segment() {
#if I3_ENABLED == 1
	strcat(buffer, ",{\"full_text\": \"");
#endif
}

void end_segment(char* color) {
#if I3_ENABLED == 1
	char colorbuffer[64];
	colorbuffer[0] = 0;
	sprintf(colorbuffer, "\", \"color\": \"%s\"}\n", color);
	strcat(buffer, colorbuffer);
#else
	strcat(buffer, " ");
#endif
}

void string(Result* (*function)()) {
	Result *res = function();
	if(res->error) {
		free(res);
		strcat(buffer, "error");
		return;
	}
	strcat(buffer, res->string);
	free(res);
}

void bar(Result *(*function)()) {
	Result *res = function();
	char bar[11];
	char buffbar[13];
	int i;
	float value;
	float tmp_val;
	if(res->error) {
		free(res);
		strcat(buffer, "error");
		return;
	}

	value = (res->value / res->max)*10;
	tmp_val = round(value);

	bar[0] = 0;

	for(i = 0; i < 10; i++) {
		if(i < tmp_val) {
			strcat(bar, "#");
		}else{
			strcat(bar, "_");
		}
	}

	sprintf(buffbar, "[%s]", bar);
	strcat(buffer, buffbar);
	free(res);
}

void percent(Result *(*function)()) {
	Result *res = function();
	char per[5];
	per[0] = 0;
	if(res->error) {
		free(res);
		strcat(buffer, "error");
		return;
	}
	sprintf(per, "%i%%", (int)round((res->value/res->max)*100));
	strcat(buffer, per);
	free(res);
}

void t(char* str) {
	strcat(buffer, str);
}

int always_true() { return 1; }
int (*true)() = always_true;

int always_false() { return 0; }
int (*false)() = always_false;

void tick(int fd, short event, void* arg) {

	buffer[0] = 0;
#if I3_ENABLED == 1
	strcat(buffer, "[{\"full_text\":\" \"}\n");
#endif
	OUTPUT
#if I3_ENABLED == 1
	strcat(buffer, "],");
#endif

#if FORMAT == 1
	printf("\r%s", buffer);
	fflush( stdout );
#else
	printf("%s\n", buffer);
	fflush( stdout );
#endif
}

int main(int argc, const char* argv[]) {
	struct event ev;
	struct timeval tv;

	tv.tv_sec = INTERVAL;
	tv.tv_usec = 0;

#if I3_ENABLED == 1
		printf("{\"version\":1}\n[[{\"full_text\":\"Haapa says hello!\"}],");
		fflush(stdout);
#endif

	event_init();
	event_set(&ev, 0, EV_PERSIST, tick, NULL);
	evtimer_add(&ev, &tv);
	event_dispatch();

	return 0;
}
