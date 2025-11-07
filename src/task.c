#include "soi.h"
#include "time.h"
#include <freertos/queue.h>

/*Vamos a usar algo bastante comun a la hora de trabajar con multiples task y son las colas
las usare especificamente para la actualizacion periodica de datos y la actualizacion de datos solo si hay alguna alerta
por lo que si llega haber una alerta la cola priorizara esta y la actualizara primero antes que actualizarla por normal*/
static QueueHandle_t dataQueue;

void init_queue()
{
    dataQueue = xQueueCreate(10, sizeof(Datos));
}
//struct para almacenar los umbrales
typedef struct {
    float rain_threshold;
    int humidity_threshold;
    float inclination_threshold;
    int monitoring_time;
    const char* alert_message;
    const char* log_message;
} AlertLevel;

static const AlertLevel alert_levels[] = {
    {10.0, 60, 10.0, 30, "ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 30 segundos.", "ALERTA PRECAUTIVA"},
    {20.0, 80, 20.0, 60, "ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 60 segundos.", "ALERTA PRECAUTIVA"},
    {35.0, 90, 30.0, 120, "ALERTA Critica: Condiciones de lluvia y humedad elevadas mantenidas por 120 segundos.", "ALERTA Citica"}
};

static char line1[17];
static char line2[17];

static void send_to_front_task(int humedad,float lluvia_mmp,const char* alerta,float inclinacion,float roll, float pitch, const char* fecha)
{
    Datos data;
    data.humedad = humedad;
    data.lluvia_mmp = lluvia_mmp;
    data.inclinacion = inclinacion;
    data.roll = roll;
    data.pitch = pitch;
    strncpy(data.alerta, alerta, sizeof(data.alerta)-1);
    strncpy(data.fecha, fecha, sizeof(data.fecha)-1);
    // Priorizar envío inmediato a la cola
    xQueueSendToFront(dataQueue, &data, 0);//alfrente de la cola
}
static void show_data_in_lcd(float inclinacion,float rain,int humidity,float roll,float pitch)
{
    snprintf(line1, sizeof(line1),"I:%d L:%d H:%d%%",
        (int)inclinacion, (int)rain, humidity);

    snprintf(line2, sizeof(line2),"R:%d P:%d",
        (int)roll, (int)pitch);
}
static void alerts_in_lcd(int value,float inclinacion,float rain,int humidity)
{
    
    snprintf(line1, sizeof(line1),"Alerta: %d",value);//escribimos en el lcd el tiempo que esta transcurriendo
    snprintf(line2, sizeof(line2),"I:%d L:%d H:%d%%",
                 (int)inclinacion, (int)rain, humidity);
    clean_lcd();
    write_lcd(line1, 0, 0);
    write_lcd(line2, 0, 1);   
}

void get_time_string(char *buffer, size_t max_len)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}
static void monitor_alert(const AlertLevel* level)
{
    //hacemos parpadear el led rojo y sonar dos bits del buzzer
    set_led('F');
    set_buzzer(true);
    vTaskDelay(pdMS_TO_TICKS(1000));
    set_buzzer(false);
    set_buzzer(true);
    vTaskDelay(pdMS_TO_TICKS(1000));
    set_buzzer(false);

    /*Esto que hice es curioso sobre la arquitectura de C, veras estoy apuntando al espacio de memoria de la estructura
    AlertLevel, y esto porque los datos que contienen son muy masivos o pues no tanto pero si son grandes
    por lo que accedemos a su espacio en memoria especificamente al de monitoring_time para poder recorrer el tiempo de cada uno*/
    for(int i=0;i<level->monitoring_time;i++)
    {
        //llamamos a la funcion del lcd para mostrar en pantalla que esta ocurriendo y el tiempo trancurrido
        alerts_in_lcd(i,get_inclinacion().inclinacionTotal,get_rain(),get_humedad_value());
        //pasa un segundo
        vTaskDelay(pdMS_TO_TICKS(1000));
        //volvemos a chequear las condiciones
        float rain = get_rain();
        int humidity = get_humedad_value();
        Inclinaciones inclination = get_inclinacion();
        
        bool alert = (rain >= level->rain_threshold && humidity >= level->humidity_threshold)
                        || (rain >= level->rain_threshold && inclination.inclinacionTotal >= level->inclination_threshold);
        if(!alert)
        {
            set_buzzer(false);
            clean_lcd();//limpiamos el lcd
            set_led('G');//volvemos a poner el led en verde
            break;
        }
        //si llega al final
        if(i==level->monitoring_time-1)
        {
            set_buzzer(true);
            set_led('E');
            //fecha
            char fecha[32];
            get_time_string(fecha,sizeof(fecha));
            // función que manda alerta al front RECORDAR MANDAR TAMBIÉN FECHA Y HORA
            send_to_front_task(humidity,rain,level->alert_message,inclination.inclinacionTotal,inclination.roll,inclination.pitch,fecha);
            ESP_LOGI("R", "%s",level->log_message);
            //cooldown
            vTaskDelay(pdMS_TO_TICKS(30 * 1000)); // 30s cooldown
            set_buzzer(false);
            return;
        }

    }

}




void update_sensor_data_task(void *pvParameters)
{
    Datos data;
    for(;;)
    {
        data.humedad=get_humedad_value();
        data.inclinacion=get_inclinacion().inclinacionTotal;
        data.lluvia_mmp=get_rain();
        data.roll=get_inclinacion().roll;
        data.pitch=get_inclinacion().pitch;
        strcpy(data.alerta, "Normal");
        get_time_string(data.fecha, sizeof(data.fecha));
        //enviamos a la cola
        xQueueSend(dataQueue,&data,0);
        vTaskDelay(pdMS_TO_TICKS(1000)); // update every second
    }
}
//esta funcion simplemente piblicara cada 5 segundos a mqtt
void publish_mqtt_task(void *pvParameters)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) pvParameters;
    Datos data;

    for(;;)
    {
        if (xQueueReceive(dataQueue, &data, portMAX_DELAY))
        {
            publish(client, &data);
        }
    }
}

void alerts_task(void *pvParameters)
{
    for(;;)
    {
        float rain = get_rain();
        int humidity = get_humedad_value();
        Inclinaciones inclination = get_inclinacion();
        bool exits_warming=false;//si hay peligro
        for(int i=0;i<(sizeof(alert_levels)/sizeof(alert_levels[0]));i++)
        {
            const AlertLevel *p = &alert_levels[i];//esapcio en memoria del struct
            if((rain >= p->rain_threshold && humidity >= p->humidity_threshold) 
                || (rain >= p->rain_threshold && inclination.inclinacionTotal >= p->inclination_threshold))
            {
                exits_warming=true;
                monitor_alert(p);
                break;
            }
        }
        if(!exits_warming)
        {
            show_data_in_lcd(inclination.inclinacionTotal,rain,humidity,inclination.roll,inclination.pitch);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));//delay

    }
}



