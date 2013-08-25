#ifndef CONFIG_H
#define CONFIG_H

#define OUTPUT \
s( t("\u21CB "); string(network_ip); ,"#5F9F74", true) \
s( t("\u2764 "); bar(proc_cpu); ,"#B94557", true) \
s( t("\u2263 "); bar(proc_memory); ,"#B28D4C", true) \
s( t("\u2607 "); string(battery_status); t(": "); bar(battery_capacity); ,"#9933CC", true) \
s( t("| "); string(time_date); ,"#FFFFFF", true) \


#define FORMAT 0					/* 0: \n 1: ontop */
#define I3_ENABLED 1				/* is I3 format enabled */
#define INTERVAL 1 					/* 1 - N */
#define BATTERY_LOCATION "/sys/class/power_supply/BAT0/"
#define NETWORK_INTERFACE "wlan0"
#define PROC_LOAD_TIME 0
#define TIME_FORMAT "%H:%M:%S"

#endif
