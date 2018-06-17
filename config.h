#ifndef CONFIG_H_
#define CONFIG_H_

#include <pthread.h>
//#include <linux/types.h>
#include <stdint.h>
//#include <stdlib.h>
//#include "lpd8806led.h"

#include "zone_ctrl.h"

#define PIN_CFGS 10
#define MAX_GPIO 25
#define MQTT_ID_LEN 20

#define CFGFILE     "./config.cfg"

#define LABEL_SIZE 40

struct CALENDARday {
  int hour;
  int minute;
  int *zruntimes;
};

struct GPIOcfg {
  int pin;
  int input_output;
  int set_pull_updown;
  int receive_mode;  // INT_EDGE_RISING  Mode to pass to wiringPi function wiringPiISR
  //int receive_state;  // LOW | HIGH  State from wiringPi function digitalRead (PIN)
  int last_event_state; // 0 or 1
  int on_state; // this is the on state should be if pin is on high or low 1 or 0
  long last_event_time; 
  char name[LABEL_SIZE];
  int startup_state;
  int shutdown_state;
  int dz_idx;
  int ignore_requests;
  int zone;
  int default_runtime;
  bool master_valve;
};

struct GPIOCONTROLcfg {
  char socket_port[6];
  char name[20];
  char docroot[512];
  char mqtt_address[20];
  char mqtt_user[50];
  char mqtt_passwd[50];
  char mqtt_topic[50];
  char mqtt_dz_sub_topic[50];
  char mqtt_dz_pub_topic[50];
  char mqtt_ID[MQTT_ID_LEN];
  int dzidx_system;
  int dzidx_24hdelay;
  int dzidx_allzones;
  bool enableMQTTdz;
  bool enableMQTTaq;
  int pinscfgs;
  int zones;
  bool system;
  bool delay24h;
  long delay24h_time;
  bool master_valve;
  struct GPIOcfg *gpiocfg;
  struct CALENDARday cron[7];
  //time_t cron_update;
  long cron_update;
  int log_level;
  struct szRunning currentZone;
  char cache_file[512];
  bool eventToUpdateHappened;
};



//struct GPIOcfg _gpioconfig_[NUM_CFGS];
extern struct GPIOCONTROLcfg _gpioconfig_;
//struct HTTPDcfg _httpdconfig_;

void readCfg (char *cfgFile);
bool remount_root_ro(bool readonly);
void write_cache();
void read_cache();

// Few states not definen in wiringPI
#define TOGGLE 2
#define BOTH 3
#define NONE -1

#define BOUNCE_LOW 4
#define BOUNCE_HIGH 5

#define YES true
#define NO false
/*
#define ON 0
#define OFF 1
*/
#define NO_CHANGE 2
#endif /* CONFIG_H_ */