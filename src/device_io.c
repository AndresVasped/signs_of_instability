#include <soi.h>


int get_humedad_value()
{
    int lectura = adc1_get_raw(ADC1_CHANNEL_0); // get raw ADC (0–4095)
    int humedad = 100 - ((lectura * 100) / 4095); // invert scale: low = wet, high = dry

    ESP_LOGI("HUMEDAD", "raw=%d | humedad=%d%%", lectura, humedad);

    return humedad;
}

float get_rain()
{
    int lectura = adc1_get_raw(ADC1_CHANNEL_6);
    float voltage = (lectura * 3.3) / 4095.0; // Convert ADC reading to voltage (0 - 3.3V)
    float inverted_voltage = 3.3 - voltage;
    float rain_mm = 0.0;

    if (inverted_voltage < 0.8) {
        rain_mm = 0.0;  // Dry
    }
    else if (inverted_voltage < 1.5) {
        rain_mm = (voltage - 0.8) * 20.0; // 0–14 mm/h
    }
    else if (inverted_voltage < 2.2) {
        rain_mm = 14.0 + (voltage - 1.5) * 30.0; // 14–35 mm/h
    }
    else {
        rain_mm = 35.0 + (voltage - 2.2) * 50.0; // 35–80 mm/h
    }

    ESP_LOGI("RAIN", "raw=%d | V=%.2f | V(inv)=%.2f | rain=%.2f mm/h",
             lectura, voltage, inverted_voltage, rain_mm);

    return rain_mm;
}


Inclinaciones get_inclinacion()
{
    Inclinaciones resultado={0};//(0,0,0) por defecto

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

/*Con esta funcion lo que quiero lograr es que direcciones el usuario deberia evitar, ya sea norte, sur, este , oeste
Ahora ahi algo que se debe tener en cuenta y es el hecho de que el sensor MPU6050 no tiene canalidad, por lo que no sabe cual es el
Norte o sur... etc, por lo que es importante a la hora del ensamblaje colocar el mpu en una direccion fija*/
const char* get_orientacion(float roll, float pitch)
{
    ESP_LOGI("R","El valor de roll: %.2f",roll);
    ESP_LOGI("R","El valor de pich: %.2f",pitch);
    //obtenemos el valor maximo de ambas inclinaciones
    if(roll>pitch)
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
        if(pitch<0)
        {
            return "Evitar Sur";
        }
        else
        {
            return "Evitar Norte";
        }
    }
    
}

static Datos current_data={
    .humedad=0,
    .lluvia_mmp=0.0f,
    .roll=0.0f,
    .pitch=0.0f,
    .inclinacion=0.0f,
    .alerta="Normal",
    .fecha="0000-00-00 00:00:00"
};

void update_data(int humedad,float lluviaMMP,float inclinacion,float roll,float pitch,const char* alerta,const char* fecha)
{
    current_data.humedad=humedad;
    current_data.lluvia_mmp=lluviaMMP;
    current_data.inclinacion=inclinacion;
    current_data.roll =roll;
    current_data.pitch=pitch;

    //evitamos truncamiento
    strncpy(current_data.alerta, alerta, sizeof(current_data.alerta) - 1);
    current_data.alerta[sizeof(current_data.alerta) - 1] = '\0';
    
    strncpy(current_data.fecha, fecha, sizeof(current_data.fecha) - 1);
    current_data.fecha[sizeof(current_data.fecha) - 1] = '\0';
}

Datos get_sensor_data()
{
    return current_data;
}

