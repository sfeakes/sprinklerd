#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>

#include "sd_cron.h"
#include "utils.h"
#include "config.h"

#define DELAY24H_SEC 86400

bool check_delay24h()
{
  if (_gpioconfig_.delay24h_time > 0) {
    time_t now;
    time(&now);
    if (difftime(now, _gpioconfig_.delay24h_time) > DELAY24H_SEC) { // 24hours in seconds
      enable_delay24h(false);
      return true;
    }
  }
  return false; 
}

void enable_delay24h(bool state)
{
  _gpioconfig_.delay24h = state;
  if (state) {
    time(&_gpioconfig_.delay24h_time);
    logMessage(LOG_INFO, "Turning on rain Delay\n");
  } else {
    _gpioconfig_.delay24h_time = 0;
    logMessage(LOG_INFO, "Turning off rain Delay\n");
  }
}

void check_cron() {
  if (_gpioconfig_.cron_update > 0) {
    logMessage(LOG_DEBUG, "Checking if CRON needs to be updated\n");
    time_t now;
    time(&now);
    if (difftime(now, _gpioconfig_.cron_update) > TIMEDIFF_CRON_UPDATE) {
      logMessage(LOG_INFO, "Updating CRON\n");
      write_cron();
      _gpioconfig_.cron_update = 0;
    }
  }
}

void write_cron() {
  FILE *fp;
  int min;
  int hour;
  int day;
  int zone;

  bool fs = remount_root_ro(false);

  fp = fopen(CRON_FILE, "w");
  if (fp == NULL) {
    logMessage(LOG_ERR, "Open file failed '%s'\n", CRON_FILE);
    remount_root_ro(true);
    //fprintf(stdout, "Open file failed 'sprinkler.cron'\n");
    return;
  }
  fprintf(fp, "***** AUTO GENERATED DO NOT EDIT *****\n");

  for (day=0; day <= 6; day++) {
    if (_gpioconfig_.cron[day].hour >= 0 && _gpioconfig_.cron[day].minute >= 0) {
      //length += sprintf(buffer+length, ", \"d%d-starttime\" : \"%.2d:%.2d\" ",day,_gpioconfig_.cron[day].hour,_gpioconfig_.cron[day].minute);
      min = _gpioconfig_.cron[day].minute;
      hour = _gpioconfig_.cron[day].hour;
      for (zone=0; zone < _gpioconfig_.zones; zone ++) {
        if (_gpioconfig_.cron[day].zruntimes[zone] > 0) {
          fprintf(fp, "%d %d * * %d root /usr/bin/curl -s -o /dev/null 'localhost?type=cron&zone=%d&runtime=%d&state=on'\n",min,hour,day,zone+1,_gpioconfig_.cron[day].zruntimes[zone]);
          min = min + _gpioconfig_.cron[day].zruntimes[zone];
          // NSF Check if to incrument hour.
          if (min >= 60) {
            min = min-60;
            hour = hour+1;
          }
          if (hour >= 24) {
            logMessage(LOG_ERR, "DON'T SUPPORT ZONES RUNTIMES GOING INTO NEXT DAY\n");
            break;
          }
        }
      }
    }
  }
  fprintf(fp, "***** AUTO GENERATED DO NOT EDIT *****\n");
  fclose(fp);

  remount_root_ro(fs);
}

void read_cron() {
  FILE *fp;
  //int i;
  char *line = NULL;
  int rc;
  int hour;
  int minute;
  int day;
  int zone;
  int runtime;
  size_t len = 0;
  ssize_t read_size;
  regex_t regexCompiled;
  //char regexString="([0-9]*) ([0-9]*) \* \* ([0-9]*) .*zone&z([0-9]*).*runtime=([0-9]*).*";
  //char *regexString="([0-9]{1,2}) ([0-9]{1,2}) \\* \\* ([0-9]{1,2}) .*zone&z([0-9]{1,2}).*runtime=([0-9]{1,2}).*";
  //char *regexString="([0-9]*)\\s([0-9]*)\\s.*\\s([0-9]*)\\s.*zone&z([0-9]*).*runtime=([0-9]*).*$";
  //char *regexString="([0-9]*)\\s([0-9]*)\\s.*\\s([0-9]*)\\s.*zone&z([0-9]*).*runtime=([0-9]*).$";
  char *regexString="([0-9]{1,2})\\s([0-9]{1,2})\\s.*\\s([0-9]{1,2})\\s.*zone=([0-9]{1,2}).*runtime=([0-9]{1,2}).*$";
  size_t maxGroups = 7;
  regmatch_t groupArray[maxGroups];
  //static char buf[100];

  // reset cron config
  for (day=0; day <= 6; day++) {
    _gpioconfig_.cron[day].hour = -1;
    _gpioconfig_.cron[day].minute = -1;
    for (zone=0; zone < _gpioconfig_.zones; zone ++) {
      _gpioconfig_.cron[day].zruntimes[zone] = 0;
    }
  }

  if (0 != (rc = regcomp(&regexCompiled, regexString, REG_EXTENDED))) {
      //printf("regcomp() failed, returning nonzero (%d)\n", rc);
      logMessage(LOG_ERR, "regcomp() failed, returning nonzero (%d)\n", rc);
      return;
      //exit(EXIT_FAILURE);
   }

  fp = fopen(CRON_FILE, "r");
  if (fp == NULL) {
    logMessage(LOG_ERR, "Open file failed '%s'\n", CRON_FILE);
    //fprintf(stdout, "Open file failed 'sprinkler.cron'\n");
    return;
  }

  while ((read_size = getline(&line, &len, fp)) != -1) {
    //printf("Read from cron:-\n  %s", line);
    //lc++;
    //rc = regexec(&regexCompiled, line, maxGroups, groupArray, 0);
    if (0 == (rc = regexec(&regexCompiled, line, maxGroups, groupArray, REG_EXTENDED))) {
      // Group 1 is minute
      // Group 2 is hour
      // Group 3 is day of week
      // Group 4 is zone
      // Group 5 is runtime
      if (groupArray[2].rm_so == (size_t)-1) {
        logMessage(LOG_ERR, "No matching information from cron file\n");
      } else {
        minute = str2int((line + groupArray[1].rm_so), (groupArray[1].rm_eo - groupArray[1].rm_so));
        hour = str2int((line + groupArray[2].rm_so), (groupArray[2].rm_eo - groupArray[2].rm_so));
        day = str2int((line + groupArray[3].rm_so), (groupArray[3].rm_eo - groupArray[3].rm_so));
        zone = str2int((line + groupArray[4].rm_so), (groupArray[4].rm_eo - groupArray[4].rm_so));
        runtime = str2int((line + groupArray[5].rm_so), (groupArray[5].rm_eo - groupArray[5].rm_so));
        logMessage(LOG_DEBUG, "Read from cron Day %d | Time %d:%d | Zone %d | Runtime %d\n",day,hour,minute,zone,runtime);

        if (day < 7 && zone <= _gpioconfig_.zones) {
          if (_gpioconfig_.cron[day].hour == -1) { // Only get the first starttime
            _gpioconfig_.cron[day].hour = hour;
            _gpioconfig_.cron[day].minute = minute;
          }
          _gpioconfig_.cron[day].zruntimes[zone-1] = runtime;
        }
      }
    } else {
      logMessage(LOG_DEBUG, "regexp no match (%d)\n", rc);
    }
  }
/*
  for (day=0; day <= 6; day++) {
    logMessage(LOG_DEBUG, "DAY %d Start time %d:%d\n",day,_gpioconfig_.cron[day].hour,_gpioconfig_.cron[day].minute);
    for (zone=0; zone < _gpioconfig_.zones; zone ++) {
      logMessage(LOG_DEBUG, "DAY %d zone %d runtime %d\n",day,zone+1,_gpioconfig_.cron[day].zruntimes[zone]);
    }
  }
*/
  fclose(fp);
}
/*
void write_cache(struct arconfig *ar_prms) {
  FILE *fp;

  fp = fopen(ar_prms->cache_file, "w");
  if (fp == NULL) {
    logMessage(LOG_ERR, "Open file failed '%s'\n", ar_prms->cache_file);
    return;
  }

  fprintf(fp, "%d\n", ar_prms->Percent);
  fprintf(fp, "%d\n", ar_prms->PPM);
  fclose(fp);
}

void read_cache(struct arconfig *ar_prms) {
  FILE *fp;
  int i;

  fp = fopen(ar_prms->cache_file, "r");
  if (fp == NULL) {
    logMessage(LOG_ERR, "Open file failed '%s'\n", ar_prms->cache_file);
    return;
  }

  fscanf (fp, "%d", &i);
  logMessage(LOG_DEBUG, "Read Percent '%d' from cache\n", i);
  if (i > -1 && i< 102)
    ar_prms->Percent = i;

  fscanf (fp, "%d", &i);
  logMessage(LOG_DEBUG, "Read PPM '%d' from cache\n", i);
  if (i > -1 && i< 5000)
    ar_prms->PPM = i;

  fclose (fp);
}
*/