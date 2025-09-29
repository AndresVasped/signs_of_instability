#include <soi.h>
#include "esp_https_server.h"

/*Acontinuacion vamos a usar una comunicacion Long polling con el cliente, osea el frontend

para ello el frontend estara constamente revisando si ya llego un nuevo dato, lo ideal seria
usar Websockets...
pero sinceramente no entendi bien como implementarlo en el framework ESPIDF por lo que
decidi utilizar por ahora long polling
*/

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

//comunicacion get el cliente nos pedira datos constamente y nosotros con gusto se los enviaremos
static esp_err_t http_handler_get(httpd_req_t *req)
{
    /*le decimos al front que vamos a enviar un json
    esto es util porque en el front a la hora de hacer una promesa con fetch le podemos
    implementar el fetch(..).json()
    para que capture el json automaticamente*/

    httpd_resp_set_type(req,"application/json");

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    int len_json_buffer=json_structure();
    return httpd_resp_send(req,json_buffer,len_json_buffer);
}

static esp_err_t http_handler_buzzer(httpd_req_t *req)
{
    char buf[10];
    int ret = httpd_req_get_url_query_str(req, buf, sizeof(buf));
    if (ret == ESP_OK) {
        char param[10];
        if (httpd_query_key_value(buf, "state", param, sizeof(param)) == ESP_OK) {
            int state = atoi(param);
            set_buzzer(state == 1);
        }
    }

    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}


void http_init_server()
{
    httpd_handle_t server = NULL;//servidor http
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();//configuracion por defecto como poner el puerto en 80 numeros de conexiones..etc
    httpd_start(&server, &cfg);//iniciar el servidor http

    static const httpd_uri_t dataLP = {
        .uri="/data",
        .method=HTTP_GET,
        .handler=http_handler_get,
        .user_ctx=NULL
    
    };
    httpd_register_uri_handler(server, &dataLP);//definimos la estructura del edpoint
    
    static const httpd_uri_t buzzerEP = {
        .uri       = "/buzzer",
        .method    = HTTP_GET,
        .handler   = http_handler_buzzer,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &buzzerEP); //definimos la estructura del edpoint

}


