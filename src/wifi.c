#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <soi.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>


/*inicializamos la flash con el nvs (non volatile storage)
Que es la NVS y porque la necesitamos? en la memoria flash de la esp32 esta almacena un
sistema de almacenamiento de clave valor (similar a un JSON pero a nivel de cpu)
como ya dije este vive a nivel de la memoria flash por lo que persistira aun si 
apagamos la esp32
podemos guardar datos como el nombre de la red WIFI y su contraseña entre otros datos utiles
en resumen el WIFI necesita escribir o leer sobre ese NSV para poder funcionar correctamente
==========================================================================================

la parte de codigo que se viene mas adelante es muy dificil de entender hasta para mi 
tratare de explicarles de una forma bastante breve que hace cada linea de codigo

me apoye de chatGPT y de la misma documentacion de ESPIDF para poder inicializar el modulo
WIFI de la esp32

si van a tratar de hacer algun tipo de modificacion trate de leer la documentacion de 
ESPRESSIF o esta pagina https://medium.com/@fatehsali517/how-to-connect-esp32-to-wifi-using-esp-idf-iot-development-framework-d798dc89f0d6
 acerca de este modulo WIFI de lo contrario intente dejarlo justo como esta

*/

/*Vamos a inicializar en sta el wifi, que quiero decir con esto
un STA (station mode) basicamente con el STA estamos haciendo que la esp32 
se comporte como un dispositivo mas de la red a la que estamos conectada,
esta misma red le asigna una ip de las que el access point tiene es decir el rango de ips 
y ahora es un dispositivo local de esa misma red, por lo tanto otras ips de ese mismo rango
pueden comunicarse con la esp32. Ahora podriamos crear nuestra propia red y que la esp32
actua como un mini router con su propio AC sin embargo el challenge no nos pide eso, nos pide
que la esp32 acceda a una red local ya establecida por eso usamos STA

================================================================================

si se fijan la funcion wifi_sta_init debe retornar un "esp_err_t" es decir una respuesta
de validacion, si efectivamente entra o no a la red*/

/*Acontinuacion creare un par de eventos, esto servira para tener orden, por ejemplo
si la coneccion de red se llega a caer nos vendra util, tambien para visualizar en que
orden inicio cada cosa y asi
 */

static int num_de_reintentos=0; //variable global de numero de intentos para wl handler

static bool wifi_conectado=false; //variable que usaremos para verificar si el wifi esta conectado o no

/*Una cosa para tener en cuenta es que esta funcion necesita de un parametro llamado base
que es un evento y de un id de dicho evento, entonces basicamente veanlo como si fuera 
una tabla en SQL donde la base es dicha tabla y id es el contenido de esa tabla
entonces tenemos en base por ejemplo WIFI_EVENT que es como tal el nombre del evento
y dentro de ese eveto se encuentran sus contenidos por id como wifi_sta_connected 
Entonces en resumen base te dice el grupo del que proviene un evento y el id te dice
que evento es exactamente.

Por buenas practicas deberiamos agregar el base para que no haya confusiones de id
porque puede haber mas de uno
sin embargo aca no lo hago porque solo voy a apuntar a una red*/

static void wifi_event_handler ( void *event_handler_arg, esp_event_base_t base, 
    int32_t id_de_evento, void *event_data)  
{ 
    //verificamos si el la coneccion para poder entrar
    if (id_de_evento == WIFI_EVENT_STA_START) 
    { 
        ESP_LOGI("R", "WIFI CONECTANDO....\n" ); 
        esp_wifi_connect();
        wifi_conectado=false;
    } 
    else  if (id_de_evento == WIFI_EVENT_STA_CONNECTED) 
    { 
        ESP_LOGI("R","WiFi CONECTADO\n" ); 
        wifi_conectado=true;
    } 
    else  if (id_de_evento == WIFI_EVENT_STA_DISCONNECTED) //si no logra la coneccion entonce la intentamos 5 veces
    { 
        ESP_LOGI("R","WiFi perdió la conexión\n" ); 
        wifi_conectado=false;
        if (num_de_reintentos< 5 ){ esp_wifi_connect ();num_de_reintentos++; ESP_LOGI("R", "Reintentando conectar...\n" );} 
    } 
    else  if (id_de_evento == IP_EVENT_STA_GOT_IP)//si captamos la ip el driver de wifi se configuro perfectamente
    { 
        ESP_LOGI("R","Wifi obtuvo IP...\n\n" ); 
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("R", "Wifi obtuvo IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        num_de_reintentos=0;
        wifi_conectado=true;
    } 
}

esp_err_t wifi_sta_init()
{
    esp_err_t ret = nvs_flash_init();//inicializamos el nvs

    /*En esta condicional lo que estamos haciendo es verificar si el NVS ya esta escrito
    o si contiene datos basura que no permite que el modulo de wifi pueda leer o escribir
    si es asi simplemente borramos dichos datos y volvemos a inicializar el nvs
    ESP_ERR_NVS_NO_FREE_PAGES: significa que la partición nvs está llena o dañada
    ESP_ERR_NVS_NEW_VERSION_FOUND: significa que tenemos datos viejos*/
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());//borramos el contenido que tenga el nvs
        ESP_ERROR_CHECK(nvs_flash_init());//volvemos a inicializarlo
    }

    /*Estas lineas son dificiles de explicar pero lo que basicamente hace es inicializar el
    netif que es basicamente para inciar el modulo de red ip (dhcp,ip mascaras..)
    y enlazarlas al driver del wifi
    gracias a esto la esp32 puede tener una ip propia, ademas si se dan cuenta
    en la linea final le estamos agregrando un sta es quiere decir que vamos a instanciar
    el driver el modo sta osea como ya dije vamos a entrar a una red local y que pueda
    asignarse su ip automaticamente, tambien podriamos como ap y ethernet*/

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta(); // interfaz STA

    /*Una vez ya configurado el driver de wifi lo inicializamos*/

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /*Basicamente estas lineas estamos haciendo un callback por eso tiene como parametro
    nuestra funcion, sin necesidad de pasarle los parametros que ya establecimos

    para la primera funcion le estamos diciendo que nos muestre todos los eventos que sean
    sobre el WIFI para la segunda los eventos para IP

    Vean estas lineas de codigo como una especie de  equivalente al override de java
    */
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);//evento para wifi
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);//evento para ip

    /*lo que viene acontinuacion es bastante importante y ya es la parte final a la 
    hora de conectarse a una red (OJO recuerden que tenemos la esp en modo STA es decir
    vamos a conectarnos a una red y dicha red mediante su Access Point nos asignara una ip
    si quisieramo que nuestra esp cree una red y actue como un pequeño router tendriamos
    que modificar el codigo a modo AP)*/

    wifi_config_t wifi_configuration={//accedemos al struct del wifi configuration en modo sta
        .sta={
            .ssid="",
            .password=""
        }//ssid y password las dejaremos en blanco en la siguinete linea copiaremos la contraseña y su ssid
    };
    //copiamos el ssid y contraseña y las agregamos al struct de wifi configuration
    strcpy((char*)wifi_configuration.sta.ssid,WIFI_SSID);
    strcpy((char*)wifi_configuration.sta.password,WIFI_PASS);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));//le decimos al chip que no vamos a a ser un AP si no un STA
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration));//le pasamos las configuraciones del driver
    ESP_ERROR_CHECK(esp_wifi_start());//arracamos el wifi con las configuraciones dadas
    ESP_ERROR_CHECK(esp_wifi_connect());//y nos conectamos a la red con las contreseñas y ssid dadas en este punto nos darian una ip

    return ESP_OK;//retornamos un Ok si todo se configuro bien
}

bool tiene_wifi()
{
    return wifi_conectado;
}


void init_time(void)
{
    ESP_LOGI("TIME", "Initializing SNTP...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");  // Use NTP server
    sntp_init();

    setenv("TZ", "GMT-5", 1);
    tzset();

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI("TIME", "Waiting for system time... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year > (2016 - 1900)) {
        ESP_LOGI("TIME", "Time synchronized!");
    } else {
        ESP_LOGW("TIME", "Failed to sync time");
    }
}
