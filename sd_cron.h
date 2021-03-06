#ifndef SD_CRON_H_
#define SD_CRON_H_

#include <stdbool.h>

//#define CRON_FILE "./sprinkler.cron"
#define CRON_FILE "/etc/cron.d/sprinklerd"
#define TIMEDIFF_CRON_UPDATE 20

void check_cron();
void write_cron();
void read_cron();
void enable_delay24h(bool state);
bool check_delay24h();
void reset_delay24h_time();
void enable_calendar(bool state);
bool setTodayChanceOfRain(int percent);
bool setTodayRainTotal(float rain);

#endif // SD_CRON_H_
