#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include "usr-410s.h"
#include "modbus_485.h"
#include "vgus.h"
#define pr() printf("%s %s %d\n", __FILE__, __func__, __LINE__)

static unsigned short  ModBusCRC (unsigned char *ptr, int size) 
{
	unsigned short a,b,tmp,CRC16,V;    
	CRC16=0xffff;
	for (a=0;a<size;a++){
		CRC16=*ptr^CRC16;
			for (b=0;b<8;b++){
				tmp=CRC16 & 0x0001;
				CRC16 =CRC16 >>1;
				if (tmp)
					CRC16=CRC16 ^ 0xa001;
			}  
			*ptr++; 
	}  
	V = ((CRC16 & 0x00FF) << 8) | ((CRC16 & 0xFF00) >> 8); 
	return V;   
}

#define HEATING_SCORE_MAX 3
#define COOLING_SCORE_MAX 3

struct modbus_info
{
	unsigned int temperature;
	unsigned int warn_max;
	unsigned int warn_min;
	unsigned int heating_score;
	unsigned int cooling_score;
	unsigned int heating_save;
	unsigned int cooling_save;
	unsigned int extremity;

};

struct modbus_info modbus_info = {240, 750, 230, 0, 0, 0, 0,500};

static int get_temperature(struct send_info *info_485)
{
	unsigned char rbuf[32];
	char sbuf[] = {0x01, 0x03, 0x00, 0x00,0x00, 0x02, 0xc4, 0x0b};
	unsigned int tem;
	int len;
	unsigned short crc;
	copy_to_buf(info_485, sbuf, sizeof(sbuf));
	len = send_and_recv_data(info_485, (char *)rbuf, 32);
       	if (len < 2){
		return -1;
	}

	 crc = ModBusCRC(rbuf, len - 2);
	 if (crc != ((rbuf[len - 2] << 8) + rbuf[len - 1])){
	 	return -1;
	 }
	
	 tem = (rbuf[3] << 8) + rbuf[4];	
	if (tem == 0)
		tem = (rbuf[5] << 8) + rbuf[6];	

	return tem;
	
}

//int cycle_period = 5;
//int cycle_value = 0; 
//#define HEATER_SW   0x01
//#define COOLER_SW   0x00
//#define open_heater(sw)  (sw) |= (1 << HEATER_SW)
//#define open_cooler(sw)  (sw) |= (1 << COOLER_SW)
//static char get_relay_value()
//{
//	char sw = 0;
//	if (modbus_info.heating_score - cycle_value > 0)
//		open_heater(sw);
//	if (modbus_info.cooling_score - cycle_value > 0)
//		open_cooler(sw);
//	cycle_value++;
//	if (cycle_value <  HEATING_SCORE_MAX)
//		cycle_value = 0;
//	return sw;
//}

//static char get_relay_value()
//{
//	char sw = 0;
//	sw |= modbus_info.heating_score << 2;
//	sw |= modbus_info.cooling_score;
//	return sw;
//}
static char get_relay_value()
{
	char sw = 0;
	sw |= modbus_info.heating_score > 0 ? 1 << 4  : 0;
	sw |= modbus_info.cooling_score > 0 ? 1 << 2  : 0;
	return sw;
}
unsigned char relay_buf[] = {0x02, 0x0F, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00, 0x00};
int set_relay(struct send_info *info_485)
{
	unsigned char rbuf[32];
	int len;
	unsigned short crc;
	char sw = get_relay_value();
	if (sw == relay_buf[7])
		return 0;
	else
		relay_buf[7] = sw;
	crc = ModBusCRC(relay_buf, 8);
	
	relay_buf[8] = crc >> 8;
	relay_buf[9] = crc & 0xff;
	copy_to_buf(info_485, (char*)relay_buf, sizeof(relay_buf));
	len = send_and_recv_data(info_485, (char *)rbuf, 32);

       	if(len < 0){
		printf("send error\n");
		return -1;
	}
	 crc = ModBusCRC(rbuf, len - 2);
	 if (crc != ((rbuf[len - 2] << 8) + rbuf[len - 1])){
	 	return -1;
	 }
	return 0;	
}

unsigned int modbus_get_temperature()
{
	return modbus_info.temperature;
}

int interval_time = 5;
int interval_value = 0;
int red = 1;
int breath_led = 1;
int extremity = 0;
int start_warn = 0;
int temp_sensor_failure = 0;

void set_temp_sensor_breath_led(struct send_info *info_485, int breath_led)
{
	unsigned char rbuf[32];
	char sbufopen[] = {0x00, 0x06, 0x00, 0x06,0x00, 0x01, 0xa9, 0xda};
	char sbufclose[] = {0x00, 0x06, 0x00, 0x06,0x00, 0x00, 0x68, 0x1a};
	unsigned int tem;
	int len;
	unsigned short crc;
	if (breath_led)
		copy_to_buf(info_485, sbufopen, sizeof(sbufopen));
	else
		copy_to_buf(info_485, sbufclose, sizeof(sbufclose));

	len = send_and_recv_data(info_485, (char *)rbuf, 32);
       	if (len < 2){
		return;
	}

	 crc = ModBusCRC(rbuf, len - 2);
	 if (crc != ((rbuf[len - 2] << 8) + rbuf[len - 1])){
	 	return;
	 }
	 tem = (rbuf[3] << 8) + rbuf[4];	
	if (tem == 0)
		tem = (rbuf[5] << 8) + rbuf[6];	

	return;
		
} 


int modbus_callback(struct send_info *info_485, struct send_info *info_232)
{
	int tem;
	if (interval_value == 1){
		interval_value++;
		set_relay(info_485);
	}else if(interval_value == 3){
		interval_value++;
		if (temp_sensor_failure > 5){
			set_temp_sensor_breath_led(info_485, 0);
		}	
	}
	else if (interval_value == 5){
		if ((tem = get_temperature(info_485)) > 0){
			modbus_info.temperature = tem;
			temp_sensor_failure = 0;
		}
		else{
			if (temp_sensor_failure < 5)
				temp_sensor_failure++;
			
		}

		tem = modbus_info.temperature;
		temperature_update_curve(info_232, (unsigned int)tem);
		interval_value = 0;

		if((unsigned int )tem > modbus_info.warn_max || (unsigned int)tem < modbus_info.warn_min){
			set_warn_icon(info_232, red &0x01);
			red  = red > 0 ? 0 : 1;	
			start_warn = 1;
		}
		else{
			if (start_warn == 1){
				set_warn_icon(info_232, 0);
				start_warn = 0;
			}
		}

		set_breath_led(info_232, breath_led & 0x01);
		breath_led = breath_led > 0 ? 0 : 1;


		if ((unsigned int)tem >= modbus_info.extremity){
			modbus_info.cooling_score = COOLING_SCORE_MAX;
			extremity = 1;
		}
		if (extremity == 1 && (unsigned int)tem <= modbus_info.warn_max){
			extremity = 0;
			modbus_info.cooling_score = 0;
		}
	}
	else{
		interval_value++;
	}

	return 0;
}

void modbus_update_warn_vaules(struct send_info *info_232, unsigned int max, unsigned int min)
{
	if (!max)
		max = modbus_info.warn_max;
	if (!min) 
		min = modbus_info.warn_min;
	if (modbus_info.warn_max != max || modbus_info.warn_min != min){
		if(temperature_draw_warn(info_232, max, min) < 0)
			return;
		modbus_info.warn_max = max;
		modbus_info.warn_min = min;
		send_data(info_232);
	}
}

void modbus_safe_mode()
{
	modbus_info.heating_save = modbus_info.heating_score;
	modbus_info.cooling_save = modbus_info.cooling_score;	
	modbus_info.cooling_score = 0;
	modbus_info.heating_score =0;
}

void modbus_recover_from_safe()
{
	modbus_info.cooling_score = modbus_info.cooling_save;
	modbus_info.heating_score = modbus_info.heating_save;
}

void modbus_update_heating_score(unsigned int score)
{
	modbus_info.heating_score = score < HEATING_SCORE_MAX ? score : HEATING_SCORE_MAX;
}

void modbus_update_cooling_score(unsigned int score)
{
	modbus_info.cooling_score = score < COOLING_SCORE_MAX ? score : COOLING_SCORE_MAX;
}
