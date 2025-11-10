#include "soi.h"
#include "time.h"
#include <freertos/queue.h>
static volatile bool alert_in_progress = false;


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
    {10.0, 40, 10.0, 30, "ALERTA PRECAUTIVA", "ALERTA PRECAUTIVA"},
    {20.0, 60, 20.0, 60, "ALERTA Media", "ALERTA Media"},
    {35.0, 80, 30.0, 120, "ALERTA Critica", "ALERTA Critica"}
};

static char line1[17];
static char line2[17];

static void send_to_front_task(int humedad, float lluvia_mmp, const char* alerta,
                               float inclinacion, float roll, float pitch, const char* fecha)
{
    Datos data;
    data.humedad = humedad;
    data.lluvia_mmp = lluvia_mmp;
    data.inclinacion = inclinacion;
    data.roll = roll;
    data.pitch = pitch;

    // safe copy + ensure null termination
    strncpy(data.alerta, alerta, sizeof(data.alerta) - 1);
    data.alerta[sizeof(data.alerta) - 1] = '\0';

    strncpy(data.fecha, fecha, sizeof(data.fecha) - 1);
    data.fecha[sizeof(data.fecha) - 1] = '\0';

    // Update global current_data so HTTP reads the alert too
    update_data(data.humedad, data.lluvia_mmp, data.inclinacion, data.roll, data.pitch, data.alerta, data.fecha);

    // Optionally clear queue to prioritize this alert (avoid being lost behind many "Normal" messages)
    xQueueReset(dataQueue);

    // Prioritize immediate sending to MQTT queue
    xQueueSendToFront(dataQueue, &data, 0);
}

static void show_data_in_lcd(float inclinacion,float rain,int humidity,float roll,float pitch)
{
    snprintf(line1, sizeof(line1),"I:%d L:%d H:%d%%",
        (int)inclinacion, (int)rain, humidity);

    snprintf(line2, sizeof(line2),"R:%d P:%d",
        (int)roll, (int)pitch);
    clean_lcd();
    write_lcd(line1, 0, 0);
    write_lcd(line2, 0, 1);
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
    alert_in_progress = true;
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
        //blink led
        gpio_set_level(LED_ROJO, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_ROJO, 0);
        vTaskDelay(pdMS_TO_TICKS(500));

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
            alert_in_progress = false;
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

            ESP_LOGI("PRUEBA", "VAMOS A ENVIAR A FRONT");
            // función que manda alerta al front RECORDAR MANDAR TAMBIÉN FECHA Y HORA
            send_to_front_task(humidity,rain,level->alert_message,inclination.inclinacionTotal,inclination.roll,inclination.pitch,fecha);
            ESP_LOGI("R", "%s",level->log_message);
            ESP_LOGI("PRUEBA", "VAMOS A ENVIAR A FRONT");
            //cooldown
            vTaskDelay(pdMS_TO_TICKS(30 * 1000)); // 30s cooldown
            set_buzzer(false);
            return;
        }

    }

}

#include "soi.h"

void buzzer_setup(void) {
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);
}

void buzzer_on(void) {
    gpio_set_level(BUZZER_PIN, 1);
}

void buzzer_off(void) {
    gpio_set_level(BUZZER_PIN, 0);
}





void update_sensor_data_task(void *pvParameters)
{
    Datos data;
    for(;;)
    {
        if (alert_in_progress) {
            vTaskDelay(pdMS_TO_TICKS(1000)); // skip sending during alerts
            continue;
        }

        data.humedad = get_humedad_value();
        data.inclinacion = get_inclinacion().inclinacionTotal;
        data.lluvia_mmp = get_rain();
        data.roll = get_inclinacion().roll;
        data.pitch = get_inclinacion().pitch;
        strcpy(data.alerta, "Normal");
        get_time_string(data.fecha, sizeof(data.fecha));

        current_data = data;
        xQueueSend(dataQueue, &data, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
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

        bool alert_active = false;
        const AlertLevel *active_alert = NULL;

        for (int i = 0; i < (sizeof(alert_levels)/sizeof(alert_levels[0])); i++)
        {
            const AlertLevel *p = &alert_levels[i];
            if ((rain >= p->rain_threshold && humidity >= p->humidity_threshold)
                || (rain >= p->rain_threshold && inclination.inclinacionTotal >= p->inclination_threshold))
            {
                alert_active = true;
                active_alert = p;
                monitor_alert(p); // Handles buzzer, LED, message
                break;
            }
        }

        if (!alert_active)
        {
            show_data_in_lcd(inclination.inclinacionTotal, rain, humidity, inclination.roll, inclination.pitch);
            // Reset system when conditions are normal again
            if (alert_in_progress) {
                ESP_LOGI("ALERTS", "✅ Conditions normalized. Clearing alert state.");
                alert_in_progress = false;

                // Turn off LED and buzzer here (if not already handled)
                gpio_set_level(LED_ROJO, 0);
                gpio_set_level(LED_VERDE, 1);
                buzzer_off();

                // Update data so frontend and HTTP know it's back to normal
                char now[32];
                get_time_string(now, sizeof(now));
                update_data(humidity, rain, inclination.inclinacionTotal,
                            inclination.roll, inclination.pitch, "Normal", now);
                show_data_in_lcd(inclination.inclinacionTotal, rain, humidity, inclination.roll, inclination.pitch);            
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
