/*En este archivo ira todas aquellas directivas C y librerias junto a las 
funciones que necesitemos*/
#pragma once //directiva para evitar duplicaciones a la hora de llamar el .h en distintos .c

#include <smbus.h>
#include <mpu6050.h>
#include <i2c-lcd1602.h>
#include "esp_log.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*-----------------definimos los puertos de la esp32-----------------*/
//sensores 
#define PIN_HUMEDAD  GPIO_NUM_36
#define PIN_LLUVIA   GPIO_NUM_34
//comunicacion i2c
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_SCL_IO GPIO_NUM_22
#define I2C_MASTER_FREQ_HZ 100000
//direcciones I2C en 7 bits
#define LCD_ADDR 0x27
//leds
#define LED_ROJO GPIO_NUM_14
#define LED_VERDE GPIO_NUM_12
//Buzzer
#define BUZZER GPIO_NUM_27
//wifi
#define WIFI_SSID "squidslife"//nombre de la red wifi
#define WIFI_PASS "squidwirelessmagic"//contraseña de la red wifi



typedef struct
{
    float roll;
    float pitch;
    float inclinacionTotal;

}Inclinaciones;

typedef struct
{
    int humedad;
    float lluvia_mmp;
    float roll;
    float pitch;
    float inclinacion;
    char alerta[40];
    char fecha[20];
}Datos;

//funciones de set
void set_GPIO(void);
void set_i2c(void);
void set_adc(void);
void mpu6050_init(void);
void set_buzzer(bool state);
void lcd_init();
void write_lcd(const char *text,int column,int row);
void clean_lcd();

//funciones de get
void set_led(char value);
int get_humedad_value(void);
float get_rain(void);
Inclinaciones get_inclinacion(void);
const char* get_orientacion(float roll, float pitch);
void update_data(int humedad,float lluviaMMP,float inclinacion,float roll,float pitch,const char* alerta,const char* fecha);
Datos get_sensor_data();
void init_time(void);



//task
void precuation_rain_task(void *pvParameters);
void alert_rain_task(void *pvParameters);
void critical_rain_task(void *pvParameters);
void update_sensor_data_task(void *pvParameters);
void lcd_task(void *pvParameters);
void get_time_string(char *buffer, size_t max_len);



//wifi
esp_err_t wifi_sta_init();
bool tiene_wifi();
//http
void http_init_server();