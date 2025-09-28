#include "soi.h"
#include "time.h"

static void send_to_front_task(int humedad,float lluvia_mmp,const char* alerta,float inclinacion,float roll, float pitch, const char* fecha)
{
    update_data(humedad,lluvia_mmp,inclinacion,roll,pitch,alerta,fecha);
}


void precuation_rain_task(void *pvParameters)
{

    for(;;)
    {
        float rain = get_rain();
        int humidity = get_humedad_value();
        Inclinaciones inclination = get_inclinacion();

        bool alert = (rain >= 10.0 && humidity >= 60) ||
                     (rain >= 10.0 && inclination.inclinacionTotal >= 10.0);

        if(alert)
        {
            set_led('F');
            // RECORDAR MANDAR FECHA Y HORA
            char fecha[30]="000000000000000";
            for(int i = 0; i < 30; i++)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));

                rain = get_rain();
                humidity = get_humedad_value();
                inclination = get_inclinacion();
                bool alert = (rain >= 10.0 && humidity >= 60) ||
                     (rain >= 10.0 && inclination.inclinacionTotal >= 10.0);
                if(!alert)
                {
                    set_buzzer(false);
                    set_led('G');
                    break;
                }

                if(i == 29)
                {
                    set_buzzer(true);
                    set_led('E');
                    // función que manda alerta al front RECORDAR MANDAR TAMBIÉN FECHA Y HORA
                    send_to_front_task(humidity,rain,"ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 30 segundos.",inclination.inclinacionTotal,inclination.roll,inclination.pitch,fecha);
                    ESP_LOGI("R", "%s", "ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 30 segundos.");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void alert_rain_task(void *pvParameters)
{
    for(;;)
    {
        float rain = get_rain();
        int humidity = get_humedad_value();
        Inclinaciones inclination = get_inclinacion();

        bool alert = (rain >= 20.0 && humidity >= 80) ||
                     (rain >= 20.0 && inclination.inclinacionTotal >= 20.0);

        if(alert)
        {
            set_led('F');
            // RECORDAR MANDAR FECHA Y HORA
            char fecha[30]="000000000000000";
            for(int i = 0; i < 60; i++)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));

                rain = get_rain();
                humidity = get_humedad_value();
                inclination = get_inclinacion();
                bool alert = (rain >= 20.0 && humidity >= 80) ||
                     (rain >= 20.0 && inclination.inclinacionTotal >= 20.0);
                if(!alert)
                {
                    set_buzzer(false);
                    set_led('G');
                    break;
                }

                if(i == 59)
                {
                    set_buzzer(true);
                    set_led('E');
                    // función que manda alerta al front RECORDAR MANDAR TAMBIÉN FECHA Y HORA
                    send_to_front_task(humidity,rain,"ALERTA: Condiciones de lluvia y humedad elevadas mantenidas por 60 segundos.",inclination.inclinacionTotal,inclination.roll,inclination.pitch,fecha);
                    ESP_LOGI("R", "%s", "ALERTA: Condiciones de lluvia y humedad elevadas mantenidas por 60 segundos.");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void critical_rain_task(void *pvParameters)
{
    for(;;)
    {
        float rain = get_rain();
        int humidity = get_humedad_value();
        Inclinaciones inclination = get_inclinacion();

        bool alert = (rain >= 35.0 && humidity >= 90) ||
                     (rain >= 35.0 && inclination.inclinacionTotal >= 30.0);

        if(alert)
        {
            set_led('F');
            // RECORDAR MANDAR FECHA Y HORA
            char fecha[30]="000000000000000";
            for(int i = 0; i < 120; i++)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));

                rain = get_rain();
                humidity = get_humedad_value();
                inclination = get_inclinacion();
                bool alert = (rain >= 35.0 && humidity >= 90) ||
                     (rain >= 35.0 && inclination.inclinacionTotal >= 30.0);
                if(!alert)
                {
                    set_buzzer(false);
                    set_led('G');
                    break;
                }

                if(i == 119)
                {
                    set_buzzer(true);
                    set_led('E');
                    // función que manda alerta al front RECORDAR MANDAR TAMBIÉN FECHA Y HORA
                    send_to_front_task(humidity,rain,"ALERTA CRITICA: Condiciones de lluvia y humedad elevadas mantenidas por 2 minutos.",inclination.inclinacionTotal,inclination.roll,inclination.pitch,fecha);
                    ESP_LOGI("R", "%s", "ALERTA CRITICA: Condiciones de lluvia y humedad elevadas mantenidas por 2 minutos.");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

