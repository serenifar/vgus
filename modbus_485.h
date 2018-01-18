#ifndef __MBTCP_MSTR___H__
#define __MBTCP_MSTR___H__
#include "usr-410s.h"
unsigned int modbus_get_temperature();
void modbus_update_warn_vaules(struct send_info *info, unsigned int max, unsigned int min);
void modbus_update_heating_score(unsigned int score);
void modbus_update_cooling_score(unsigned int score);
void modbus_safe_mode();
void modbus_recover_from_safe();
int modbus_callback(struct send_info *info_485, struct send_info *info_232);
void set_temp_sensor_breath_led(struct send_info *info_485, int breath_led);
char get_relay_value();
char get_cool_status();
char get_heater_status();
unsigned int  get_bridge_status();
#endif
