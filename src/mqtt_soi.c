#include <soi.h>


/*Vamos a hacer la comunicacion MQTT para eso debemos tener un par de cosas en cuenta
nuestra esp32 estara conectada a la raspberrypi y se suscribira a un topic llamado esp32/sensors
donde enviara todos los datos que le vayamos a pasar para asi poderlos capturarlos por el broker y 
enviarlo a otros dispositivos suscritos*/


static char json_buffer[256]; //buffer de json
//funcion de datos json que enviaremos
int json_structure()
{
    Datos data_sensor=get_sensor_data();
    int len=snprintf(json_buffer, sizeof(json_buffer),
        "{"
        "\"humedad\":%d,"
        "\"lluvia_mmph\":%.2f,"
        "\"inclinacion\":%.2f,"
        "\"roll\":%.2f,"
        "\"pitch\":%.2f,"
        "\"riesgo\":\"%s\","
        "\"fecha\":\"%s\""
        "}",
        data_sensor.humedad,data_sensor.lluvia_mmp,data_sensor.inclinacion,data_sensor.roll,data_sensor.pitch,data_sensor.alerta,data_sensor.fecha);

    return len;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED://El cliente ha establecido correctamente una conexión con el broker. El cliente ya está listo para enviar y recibir datos.
            ESP_LOGI("MQTT", "Conectado al broker MQTT");
            esp_mqtt_client_subscribe(client, "esp32/sensors", 0);//se suscribe automaticamente y al topic sensors
            esp_mqtt_client_subscribe(client, "esp32/buzzer", 0);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW("MQTT","Desconeccion con el broker");
            break;
        default:
            break;
        
    }

    case MQTT_EVENT_DATA:
    ESP_LOGI("MQTT", "Mensaje recibido en topic: %.*s", event->topic_len, event->topic);
    ESP_LOGI("MQTT", "Contenido: %.*s", event->data_len, event->data);

    if (strncmp(event->topic, "esp32/buzzer", event->topic_len) == 0) {
        // Extract message
        if (event->data_len > 0) {
            char msg[8] = {0};
            snprintf(msg, sizeof(msg), "%.*s", event->data_len, event->data);

            if (strcmp(msg, "1") == 0) {
                ESP_LOGI("MQTT", "Encendiendo buzzer...");
                buzzer_on(); // you must implement this
            } else if (strcmp(msg, "0") == 0) {
                ESP_LOGI("MQTT", "Apagando buzzer...");
                buzzer_off(); // you must implement this
            }
        }
    }
    break;

}
/*cofiguracion del cliente mqtt*/
esp_mqtt_client_handle_t client_config()
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker={
            .address.uri=BROKER_URI,//la uri del broker de las raspberrypi
        },
        .credentials={
            .client_id="esp32_sensor",//el id del cliente 
            .username=NULL,//el username estara por defecto en null ya que es publico
        },
        .network={
            .reconnect_timeout_ms=5000,//intentara reconectarse cada 5 segundos por si se desconecta o falla algo de la coneccion con la pi
        },
        .session={
            .keepalive=120,//120 segundos de comunicacion con el broker es la que recomienda la documentacion
        },
        .task={
            .priority=5,//la prioridad RTOS que le asignamos con su espacio espacio en ram
            .stack_size=4096
        }
    
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);

    return client;
}

void publish(esp_mqtt_client_handle_t client, const Datos *data)
{
    char json_buffer[256];
    int len = snprintf(json_buffer, sizeof(json_buffer),
        "{"
        "\"humedad\":%d,"
        "\"lluvia_mmph\":%.2f,"
        "\"inclinacion\":%.2f,"
        "\"roll\":%.2f,"
        "\"pitch\":%.2f,"
        "\"riesgo\":\"%s\","
        "\"fecha\":\"%s\""
        "}",
        data->humedad,
        data->lluvia_mmp,
        data->inclinacion,
        data->roll,
        data->pitch,
        data->alerta,
        data->fecha);

    esp_mqtt_client_publish(client, "esp32/sensors", json_buffer, len, 0, 0);
    ESP_LOGI("MQTT", "Publicado correctamente: %s", json_buffer);
    vTaskDelay(pdMS_TO_TICKS(2000));
}
