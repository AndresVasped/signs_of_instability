#include <soi.h> 
#include <time.h>
#include <sys/time.h>

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
    //para los leds y buzzer
    io_conf.mode=GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask=(1ULL<<LED_ROJO) | (1ULL<<LED_VERDE) | (1ULL<<BUZZER);//declaramos los puertos de los leds y buzzer como salidas
    io_conf.intr_type = GPIO_INTR_DISABLE;//desabilitamos funcionabilidades
    //desabilitamos pull up y pull down
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    //agregamos la direccion en memoria
    gpio_config(&io_conf);
}

/*Esta funcion es solo si queremos configurar a I2C*/
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
    /*por otro lado ADC1_CHA3 es GPIO39 en nuestro caso el sensor de lluvia*/
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
}

void mpu6050_init()
{
    /*Estoy usando la libreria MPU6050 mas informacion en la carpeta lib, esta libreria me proporciona la logica para 
    el manejo de la MPU, ahora en esta linea de codigo vamos a instanciar dicho modulo, el ultimo parametro
    es un booleano y indica si la libreria debe hacer la configuracion paar el i2c o no
    en este caso no porque ya la hicimos mas arriba con la funcion set_i2c*/
    mpuBegin(MPU6050_ACCEL_RANGE_2G, MPU6050_GYRO_RANGE_250DPS, true);//iniciamos el modulo del mpu
    mpuSetFilterBandwidth(MPU6050_BAND_21_HZ);
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

/*Para esta funcion agregamos G (Good) si todo esta bien encendemos el led verde, E (Error) si ahi problemas encendemos el rojo*/
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
    else if(value=='F')
    {
        gpio_set_level(LED_VERDE, 0);
        gpio_set_level(LED_ROJO, 0);
    }
    
}

void set_buzzer(bool state)
{
    
    if(state)
    {
        gpio_set_level(BUZZER,1);
    }
    else
    {
        gpio_set_level(BUZZER,0);
    }
}

void get_time_string(char *buffer, size_t max_len)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}


void update_sensor_data_task(void *pvParameters)
{
    for(;;)
    {
        int humedad = get_humedad_value();
        float lluvia = get_rain();
        Inclinaciones incl = get_inclinacion();

        char fecha[30];
        get_time_string(fecha, sizeof(fecha));

        update_data(humedad, lluvia, incl.inclinacionTotal, incl.roll, incl.pitch, "Normal", fecha);

        

        update_data(humedad, lluvia, incl.inclinacionTotal, incl.roll, incl.pitch, "Normal", fecha);

        vTaskDelay(pdMS_TO_TICKS(1000)); // update every second
    }
}


