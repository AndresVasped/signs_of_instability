#include "soi.h"
#include "time.h"

static void send_to_front_task(int humedad,float lluvia_mmp,const char* alerta,float inclinacion,float roll, float pitch, const char* fecha)
{
    update_data(humedad,lluvia_mmp,inclinacion,roll,pitch,alerta,fecha);
}

static char current_alert[16] = "Normal";


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
            char fecha[30] = "000000000000000";
            bool resolved = false;
            

            for(int i = 0; i < 30; i++)
            {
                strcpy(current_alert, "Precaution");
                set_buzzer(true);
                gpio_set_level(LED_VERDE, 0);
                //Blink red
                gpio_set_level(LED_ROJO, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(LED_ROJO, 0);
                vTaskDelay(pdMS_TO_TICKS(500));

                // Re-check conditions every second
                rain = get_rain();
                humidity = get_humedad_value();
                inclination = get_inclinacion();

                alert = (rain >= 10.0 && humidity >= 60) ||
                        (rain >= 10.0 && inclination.inclinacionTotal >= 10.0);

                if(!alert) {
                    strcpy(current_alert, "Normal");
                    set_buzzer(false);
                    set_led('G'); // go back to green
                    resolved = true;
                    break;
                }
            }

            if(!resolved) {
                set_buzzer(true);
                set_led('E'); // solid red
                send_to_front_task(humidity, rain,
                                   "ALERTA PRECAUTIVA: Condiciones mantenidas por 30 segundos.",
                                   inclination.inclinacionTotal, inclination.roll, inclination.pitch, fecha);
                ESP_LOGI("R", "%s", "ALERTA PRECAUTIVA lanzada.");
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
            char fecha[30] = "000000000000000";
            bool resolved = false;

            for(int i = 0; i < 30; i++)
            {
                strcpy(current_alert, "Alert");
                set_buzzer(true);
                gpio_set_level(LED_VERDE, 0);
                // Blink red
                gpio_set_level(LED_ROJO, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(LED_ROJO, 0);
                vTaskDelay(pdMS_TO_TICKS(500));

                // Re-check conditions every second
                rain = get_rain();
                humidity = get_humedad_value();
                inclination = get_inclinacion();

                alert = (rain >= 20.0 && humidity >= 80) ||
                     (rain >= 20.0 && inclination.inclinacionTotal >= 20.0);

                if(!alert) {
                    strcpy(current_alert, "Normal");
                    set_buzzer(false);
                    set_led('G'); // go back to green
                    resolved = true;
                    break;
                }
            }

            if(!resolved) {
                set_buzzer(true);
                set_led('E'); // solid red
                send_to_front_task(humidity, rain,
                                   "ALERTA PRECAUTIVA: Condiciones mantenidas por 30 segundos.",
                                   inclination.inclinacionTotal, inclination.roll, inclination.pitch, fecha);
                ESP_LOGI("R", "%s", "ALERTA PRECAUTIVA lanzada.");
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
            char fecha[30] = "000000000000000";
            bool resolved = false;

            for(int i = 0; i < 30; i++)
            {
                strcpy(current_alert, "Critical"); 
                set_buzzer(true);
                gpio_set_level(LED_VERDE, 0);
                // Blink red
                gpio_set_level(LED_ROJO, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(LED_ROJO, 0);
                vTaskDelay(pdMS_TO_TICKS(500));

                // Re-check conditions every second
                rain = get_rain();
                humidity = get_humedad_value();
                inclination = get_inclinacion();

                alert = (rain >= 35.0 && humidity >= 90) ||
                     (rain >= 35.0 && inclination.inclinacionTotal >= 30.0);

                if(!alert) {
                    strcpy(current_alert, "Normal");
                    set_buzzer(false);
                    set_led('G'); // go back to green
                    resolved = true;
                    break;
                }
            }

            if(!resolved) {
                set_buzzer(true);
                set_led('E'); // solid red
                send_to_front_task(humidity, rain,
                                   "ALERTA PRECAUTIVA: Condiciones mantenidas por 30 segundos.",
                                   inclination.inclinacionTotal, inclination.roll, inclination.pitch, fecha);
                ESP_LOGI("R", "%s", "ALERTA PRECAUTIVA lanzada.");
            }
        }
       
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void lcd_task(void *pvParameters)
{
    char line1[17];
    char line2[17]; // only 16 chars max + null terminator

    for(;;)
    {
        float rain = get_rain();
        int humidity = get_humedad_value();
        Inclinaciones inclination = get_inclinacion();

        // --- Row 1: Alert level ---
        if (strcmp(current_alert, "Normal") == 0) {
            snprintf(line1, sizeof(line1), "Todo esta bien");
        } else {
            snprintf(line1, sizeof(line1), "%.16s", current_alert);
        }

        // --- Row 2: Sensor data (no decimals) ---
        snprintf(line2, sizeof(line2),
                 "I:%d L:%d H:%d%%",
                 (int)inclination.inclinacionTotal, (int)rain, humidity);

        clean_lcd();
        write_lcd(line1, 0, 0);
        write_lcd(line2, 0, 1);

        vTaskDelay(pdMS_TO_TICKS(2000)); // refresh every 2s
    }
}
