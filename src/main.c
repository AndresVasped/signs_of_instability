#include <i2c-lcd1602.h>
#include <smbus.h>
#include <mpu6050.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "math.h"

/*-----------------definimos los puertos de la esp32-----------------*/
//sensores 
#define PIN_HUMEDAD  GPIO_NUM_36
#define PIN_LLUVIA   GPIO_NUM_4
//comunicacion i2c
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_SCL_IO GPIO_NUM_22
#define I2C_MASTER_FREQ_HZ 100000
//direcciones I2C en 7 bits
#define LCD_ADDR 0x27
//leds
#define LED_ROJO GPIO_NUM_14
#define LED_VERDE GPIO_NUM_12


static smbus_info_t *smbus_info;
static i2c_lcd1602_info_t *lcd_info;

//un struc para definir las variables de inclinacion que son 2
struct Inclinaciones
{
    float roll;
    float pitch;
    float inclinacionTotal;

};

void set_GPIO()
{
    gpio_config_t io_conf;
    //para los leds
    io_conf.mode=GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask=(1ULL<<LED_ROJO) | (1ULL<<LED_VERDE);//declaramos los puertos de los leds como salidas
    io_conf.intr_type = GPIO_INTR_DISABLE;//desabilitamos funcionabilidades
    //desabilitamos pull up y pull down
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    //agregamos la direccion en memoria
    gpio_config(&io_conf);

    /*El sensor de lluvia va retornar un bit de HIGH o LOW, es decir va ser digital
    por el hecho de que nos va decir si esta lloviendo (1) o no (0)*/
    io_conf.mode=GPIO_MODE_INPUT;
    io_conf.pin_bit_mask=(1ULL<<PIN_LLUVIA);//delaramos el puerto de lluvia como entrada
    //io_conf.pull_up_en = GPIO_PULLUP_ENABLE; //si el sensor de lluvia no tiene pullups lo activamos
    gpio_config(&io_conf);
}
void set_i2c()
{
    i2c_config_t conf;
    conf.mode=I2C_MODE_MASTER;//iniciamos a la esp32 como maestro

    //le pasamos los puertos de SDA y SCL
    conf.sda_io_num=I2C_MASTER_SDA_IO;
    conf.scl_io_num=I2C_MASTER_SCL_IO;

    /*configuramos los pull ups, para simulacion estaran en enable pero a la hora de pasar el firmware a la esp descativarlo(DISABLE)
    y agregar resistencias pull up de 4.7 kΩ a vcc*/
    conf.scl_pullup_en=GPIO_PULLUP_ENABLE;
    conf.sda_pullup_en=GPIO_PULLUP_ENABLE;

    //conf.scl_pullup_en=GPIO_PULLUP_DISABLE;
    //conf.sda_pullup_en=GPIO_PULLUP_DISABLE;

    //configuramos la frecuencia
    conf.master.clk_speed=I2C_MASTER_FREQ_HZ;

    //incializamos el I2C
    i2c_param_config(I2C_NUM_0,&conf);
    i2c_driver_install(I2C_NUM_0,conf.mode,0,0,0);
}
void set_adc()
{
    /*El sensor de humedad es analogo por lo que nos va pasar una frecuencia que luego parsearemos a un valor
    que se pueda entender*/

    //configuramos el sensor de humedad al ser analogo 
    adc1_config_width(ADC_WIDTH_BIT_12);  // Precisión de 12 bits (0-4095)

    /*En la ficha tecnica de la ESP32 ADC1_CHA0 es el GPIO36, en nuestro caso ese esta para el sensor de humedad*/
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);  // Rango 0-3.3V para ADC_ATTEN
}
int get_humedad_value()
{
    int lectura=adc1_get_raw(ADC1_CHANNEL_0);//obtenemos el valor de 0-4095 de la humedad

    /*retornamos el porcentaje de 0 a 100, tener en cuenta que entre mas alto sea el valor de 0 a 4095 el suelo estara menos humedo
    entre mas bajo sera mas humedo, en este caso invertimos eso y retornamos el porcentaje*/
    return 100-((lectura*100)/4095);
    
}
bool get_isRain()
{
    //retornamos el valor de la lluvia HIGH(1) no esta lloviendo LOW(0) si  esta lloviendo
    return (gpio_get_level(PIN_LLUVIA)==0);
}
void mpu6050_init()
{
    /*Estoy usando la libreria MPU6050 mas informacion en la carpeta lib, esta libreria me proporciona la logica para 
    el manejo de la MPU, ahora en esta linea de codigo vamos a instanciar dicho modulo, el ultimo parametro
    es un booleano y indica si la libreria debe hacer la configuracion paar el i2c o no
    en este caso no porque ya la hicimos mas arriba con la funcion set_i2c*/
    mpuBegin(MPU6050_ACCEL_RANGE_2G, MPU6050_GYRO_RANGE_250DPS, true);//iniciamos el modulo del mpu
    mpuSetFilterBandwidth(MPU6050_BAND_21_HZ);
    mpuSetSleepMode(false);
}
/*Para hallar la inclinacion usaremos lo que se conoce en aeronautica como ROLL y PITCH, son dos formas para de una u otra forma
medir la inclinacion o grados de movimiento hacia el eje x o y*/
struct Inclinaciones get_inclinacion()
{
    struct Inclinaciones resultado={0};//(0,0) por defecto

    esp_err_t ret = mpuReadSensors();//leemos los sensores
    if(ret==ESP_OK)//si la respuesta es OK commo 500 de la esp32
    {
       
        //obtenemos los componentes x y z del modulo del acelerometro
        float ax= mpuGetAccelerationX();
        float ay= mpuGetAccelerationY();
        float az= mpuGetAccelerationZ();
        ESP_LOGI("R", "ax: %.2f | ay: %.2f | az: %.2f", ax, ay, az);
        float roll_rad=atan2(ay,sqrt((ax*ax)+(az*az)));
        float pitch_rad=atan2(ax,sqrt((ay*ay)+(az*az)));

        //los pasamos a grados
        resultado.roll=roll_rad*180/M_PI;
        resultado.pitch=pitch_rad*180/M_PI;
        //por si queremos tener un valor estandar en grados
        resultado.inclinacionTotal=sqrt((resultado.roll*resultado.roll)+(resultado.pitch*resultado.pitch));

    }
    if(ret!=ESP_OK)
    {
        ESP_LOGI("R", "mal todo mal");
        ESP_LOGE("R", "Error code: %s", esp_err_to_name(ret));
    }
    return resultado;
}

float get_vibraciones()
{
    esp_err_t ret = mpuReadSensors();//leemos los sensores
    if(ret==ESP_OK)
    {
        float ax= mpuGetAccelerationX();
        float ay= mpuGetAccelerationY();
        float az= mpuGetAccelerationZ();

        //ya que el modulo de acelerometro mide la fuerza en los 3 ejes por pitagoras sacamos la fuerza total que es la vibracion
        float vibracion=sqrt((ax*ax)+(ay*ay)+(az*az));

        return vibracion;
    }
    else
    {
        return ESP_FAIL;
    }
}

/*Con esta funcion lo que quiero lograr es que direcciones el usuario deberia evitar, ya sea norte, sur, este , oeste
Ahora ahi algo que se debe tener en cuenta y es el hecho de que el sensor MPU6050 no tiene canalidad, por lo que no sabe cual es el
Norte o sur... etc, por lo que es importante a la hora del ensamblaje colocar el mpu en una direccion fija*/
const char* get_orientacion(float roll, float pich)
{
    //ESP_LOGI("R","El valor de roll: %.2f",roll);
    //ESP_LOGI("R","El valor de pich: %.2f",pich);
    //obtenemos el valor maximo de ambas inclinaciones
    if(roll>pich)
    {
        if(roll<0)
        {
            return "Evitar Oeste";
        }
        else
        {
            return "Evitar Este";
        }
    }
    else
    {
        if(pich<0)
        {
            return "Evitar Sur";
        }
        else
        {
            return "Evitar Norte";
        }
    }
    
}

void set_led(char value)
{
    if(value=='G')
    {
        gpio_set_level(LED_VERDE,1);
        gpio_set_level(LED_ROJO,0);
    }
    else if(value=='E')
    {
        gpio_set_level(LED_VERDE,0);
        gpio_set_level(LED_ROJO,1);
    }
    
}

void lcd_init()
{
    smbus_info = smbus_malloc();
    smbus_init(smbus_info, I2C_NUM_0, LCD_ADDR);

    // Inicializar LCD
    lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, smbus_info, true, 2, 16,16);
      
}
void write_lcd(const char *text,int column,int row)
{
    i2c_lcd1602_home(lcd_info);
    i2c_lcd1602_move_cursor(lcd_info, column, row);
    i2c_lcd1602_write_string(lcd_info, text);
}
void clean_lcd()
{
    i2c_lcd1602_clear(lcd_info);
}
void alertas_precipitacion()
{
    /*
        umbrales para inclinacion
        normal=5.0
        alerta=10.0
        peligro=20.0
        critico=35.0

        umbrales para vibracion
        normal=0.1
        alerta=0.3
        peligro=0.8
        critico=2.0
    */
    
    float vibracion_del_suelo=get_vibraciones();
    struct Inclinaciones inclinacion=get_inclinacion();
    //ESP_LOGI("R","El valor de inclinacion total: %.2f",inclinacion.inclinacionTotal);

    //obtenemos humedad
    int humedad=get_humedad_value();
    //obtenemos si esta lloviendo
    bool esta_lloviendo=get_isRain();

    //obtenemos la orientacion
    const char* orientacion=get_orientacion(inclinacion.roll,inclinacion.pitch);

    clean_lcd();
    //si humedad 50% + inclinacion grave , lanza alerta
    if(humedad>=50 && inclinacion.inclinacionTotal>10.0)
    {
        set_led('E');
        write_lcd("Peligro",0,0);
        write_lcd(orientacion,0,1);
        
    }
    //si humedad 60% + lluvia
    else if(humedad>60 && esta_lloviendo)
    {
        set_led('E');
        write_lcd("Peligro",0,0);
        write_lcd(orientacion,0,1);
    }
    // inclinacion peligro + vibraciones alerta
    else if(inclinacion.inclinacionTotal>20.0 && vibracion_del_suelo>=0.3)
    {
        set_led('E');
        write_lcd("Peligro",0,0);
        write_lcd(orientacion,0,1);
    }
    //humedad 80% + vibraciones peligrosas
    else if(humedad>=80 && vibracion_del_suelo>=0.8)
    {
        set_led('E');
        write_lcd("Peligro",0,0);
        write_lcd(orientacion,0,1);
    }
    //lluvia + vibraciones peligrosas
    else if(esta_lloviendo && vibracion_del_suelo>=2.0)
    {
        set_led('E');
        write_lcd("Peligro",0,0);
        write_lcd(orientacion,0,1);
    }
    else
    {
        set_led('G');
        write_lcd("Todo bien",0,0);
    }
    
}

void app_main()
{
    //llamamos a las funciones
    set_GPIO();
    set_adc();
    //set_i2c();
    mpu6050_init();
    lcd_init();
    set_led('G');//inicializamos el led en verde
    
    
    while (true)
    {
        alertas_precipitacion();
        vTaskDelay(pdMS_TO_TICKS(500));//agregamos un delay de medio segundo entre lecturas
    }
}