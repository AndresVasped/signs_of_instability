#include <soi.h>

void app_main()
{
    //llamamos a las funciones de inicializacion
    set_GPIO();
    set_adc();
    mpu6050_init();
    lcd_init();
    set_led('G');
    set_buzzer(false);
    wifi_sta_init();
    init_time();
    while(!tiene_wifi())
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    http_init_server();
    esp_mqtt_client_handle_t mqtt_client=client_config();
    init_queue();
    /*Acontinuacion vamos a hacer las tareas para cada task
    lo que haremos es asiganrle un tiempo en CPU por nivel de prioridad
    
    una cosa que me llamo la atencion es que como ven uno de los patrametros que le paso
    a cada tarea es 4095, que significa eso? en freeRTOS cada tarea puede tener un espacio
    en memoria RAM donde guardara variables locales, las funciones estaticas que cree entre
    otras cosas
    
    uno puede darle un espacio a cada tarea en este cas 4095 bytes o mas o menos 
    4kb por tarea
    
    si fueran tareas mas pesadas donde necesitaramos de mas memoria le podriamos dar
    8kb
    
    o si fueran mas sencillas pues menos como 2kb
    
    el tamaño promedio y lo que la documentacion dice es que con 4095 esta bastante bien*/
    
    xTaskCreate(alerts_task, "tarea de alertas", 8090, NULL, 5, NULL);
    xTaskCreate(update_sensor_data_task, "update_sensor_data", 4096, NULL, 4, NULL);
    xTaskCreate(publish_mqtt_task, "tarea critica", 4096, (void*)mqtt_client, 5, NULL);
    


    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI("MAIN", "Tasks ejecutándose...");
    }
    /*Algo a tener en cuenta es que para tener una rutina de interrupcion podriamos 
    intentarlo desde el dashboard es decir un boton de panico o algo
    que interrumpa inmediantamente las tareas que esta haciendo el programa para darle 
    prioridad a esa tarea de interrupcion*/

}