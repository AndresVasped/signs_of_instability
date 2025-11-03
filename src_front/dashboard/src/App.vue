<template>
  <div class="dashboard">
    <h1>🌦️ Dashboard</h1>

    <div class="cards">
      <div class="card">
        <h2>🌡️ Humedad</h2>
        <p>{{ humedad }} %</p>
      </div>
      <div class="card">
        <h2>🌧️ Lluvia</h2>
        <p>{{ lluvia }} mm/h</p>
      </div>
      <div class="card">
        <h2>📐 Roll</h2>
        <p>{{ roll.toFixed(2) }}°</p>
      </div>
      <div class="card">
        <h2>📏 Pitch</h2>
        <p>{{ pitch.toFixed(2) }}°</p>
      </div>
      <div class="card">
        <h2>🧭 Inclinación</h2>
        <p>{{ inclinacion.toFixed(2) }}°</p>
      </div>
    </div>

    <div class="extra">
      <p><strong>📅 Fecha:</strong> {{ fecha }}</p>       
    </div>

    <div class="controls">
      <button class="btn danger" @click="controlBuzzer(true)">🔊 Encender Buzzer</button>
      <button class="btn success" @click="controlBuzzer(false)">🔇 Apagar Buzzer</button>
    </div>

    <div class="alerts-container p-6 bg-gray-900 text-white rounded-2xl shadow-lg">
    <h2 class="text-2xl font-bold mb-4 text-center">Últimas alertas</h2>

    <div class="alerts-grid grid grid-cols-1 md:grid-cols-3 gap-6">
      <!-- Alertas de 30s -->
      <div class="alert-box bg-gray-800 p-4 rounded-xl">
        <h3 class="text-xl font-semibold mb-2 text-green-400">30 segundos</h3>
        <ul>
          <li v-for="(alert, index) in last30" :key="index" class="border-b border-gray-700 py-1">
            {{ alert }}
          </li>
          <p>Mensaje de recomendacion por IA:{{ messageFor30 }}</p>
        </ul>
      </div>

      <!-- Alertas de 60s -->
      <div class="alert-box bg-gray-800 p-4 rounded-xl">
        <h3 class="text-xl font-semibold mb-2 text-yellow-400">60 segundos</h3>
        <ul>
          <li v-for="(alert, index) in last60" :key="index" class="border-b border-gray-700 py-1">
            {{ alert }}
          </li>
          <p>Mensaje de recomendacion por IA:{{ messageFor60 }}</p>
        </ul>
      </div>

      <!-- Alertas de 120s -->
      <div class="alert-box bg-gray-800 p-4 rounded-xl">
        <h3 class="text-xl font-semibold mb-2 text-red-400">120 segundos</h3>
        <ul>
          <li v-for="(alert, index) in last120" :key="index" class="border-b border-gray-700 py-1">
            {{ alert }}
          </li>
          <p>Mensaje de recomendacion por IA:{{ messageFor120 }}</p>
        </ul>
      </div>
    </div>
  </div>

  </div>
</template>

<script>
import mqtt from 'mqtt'
import {GoogleGenAI} from "@google/genai";
export default {
  data() {
    return {
      humedad: 0,
      lluvia: 0,
      roll: 0,
      pitch: 0,
      inclinacion: 0,
      fecha: '',
      alerta: '',
      //ultimas alertas
      last30: [],
      messageFor30:'',
      last60: [],
      messageFor60:'',
      last120: [],
      messageFor120:'',
      lastUbidotsSentAt: 0,
      ubidotsMinIntervalMs: 5000
    }
  },
  methods: {
    async getData() 
    {
      const client= mqtt.connect('ws://broker.emqx.io:8083/mqtt');//broker mqtt
      client.on('connect',()=>{//conxion con el broker
        console.log('Conectado al broker MQTT por ws');
        client.subscribe('esp32/sensors');
      });
      client.on('message',(topic,message)=>{
        try
        {
          console.log('mensaje de topic '+topic+" y mensaje: "+message)
          const d=JSON.parse(message.toString());//convertimos el buffer a objeto json
          this.humedad = d.humedad;
          this.lluvia = d.lluvia_mmph;
          this.roll = d.roll;
          this.pitch = d.pitch;
          this.inclinacion = d.inclinacion;
          this.fecha = d.fecha;
          this.alerta = d.riesgo || d.alerta;
          this.lastAlerts();
          //this.send_to_ubidots(d);
        }
        catch(err)
        {
          console.error(' Error al parsear mensaje MQTT:', err);
        }
        
      });
      
    },

    async controlBuzzer(state) {
      const ESP32_IP = "192.168.145.115"; 
      try {
        
        await fetch(`http://${ESP32_IP}/buzzer?state=${state ? 1 : 0}`);
      } catch (e) {
        console.log("Error controlling buzzer", e);
      }
    },

    lastAlerts()
    {
      if(this.alerta==="ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 30 segundos.")
      {
        const date=new Date();
        const formateDate=this.formateDateTime(date)
        const message=`ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 30 segundos a las ${formateDate}`;
        this.last30.unshift(message)
        this.messageFor30=this.AI(this.humedad,this.lluvia_mmph,this.inclinacion,this.roll,this.pitch);
        if (this.last30.length > 3) this.last30.pop(); // mantener solo 3
      }
      else if(this.alerta==="ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 60 segundos.")
      {
        const date=new Date();
        const formateDate=this.formateDateTime(date)
        const message=`ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 60 segundos. ${formateDate}`;
        this.last60.unshift(message)
        this.messageFor60=this.AI(this.humedad,this.lluvia_mmph,this.inclinacion,this.roll,this.pitch);
        if (this.last60.length > 3) this.last60.pop(); // mantener solo 3
      }
      else if(this.alerta==="ALERTA Critica: Condiciones de lluvia y humedad elevadas mantenidas por 120 segundos.")
      {
        const date=new Date();
        const formateDate=this.formateDateTime(date)
        const message=`ALERTA Critica: Condiciones de lluvia y humedad elevadas mantenidas por 120 segundos. ${formateDate}`;
        this.last120.unshift(message)
        this.messageFor120=this.AI(this.humedad,this.lluvia_mmph,this.inclinacion,this.roll,this.pitch);
        if (this.last120.length > 3) this.last120.pop(); // mantener solo 3
      }
    },
    formateDateTime(date)
    {
      const year = date.getFullYear();
      // getMonth() es zero-based, por eso se suma 1
      const month = String(date.getMonth() + 1).padStart(2, '0');
      const day = String(date.getDate()).padStart(2, '0');
      const hours = String(date.getHours()).padStart(2, '0');
      const minutes = String(date.getMinutes()).padStart(2, '0');
      const seconds = String(date.getSeconds()).padStart(2, '0');
      return  `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`;
    },
    async AI(humedad,lluvia,inclinacionTotal,roll,pitch)
    {
      const ai = new GoogleGenAI({apiKey:import.meta.env.VITE_GEMINI_API_KEY});
      const response = await ai.models.generateContent({
        model: "gemini-2.5-flash",
        contents: `En mayo de 2025, las fuertes precipitaciones en regiones como Tabio y Cajicá (Cundinamarca) generaron múltiples deslizamientos de 
        tierra y colapsos viales, especialmente en el sector La Alquería. Estos eventos provocaron el cierre total de vías, afectaron alrededor de 50 
        viviendas y dejaron más de 2.200 familias damnificadas. En total, 19 municipios entraron en alerta roja.
        La detección temprana de deslizamientos de tierra es fundamental para minimizar daños y proteger a las comunidades en zonas de alto riesgo.
        A continuación, se presentan los datos actuales de sensores: 
        Humedad: ${humedad}, Intensidad de lluvia:${lluvia} ,Inclinación total:${inclinacionTotal} ,Roll:${roll} ,Pitch:${pitch}
        Con base en esta información, analiza los datos y genera un breve mensaje que indique el nivel de riesgo , una observación 
        y una recomendacion general sobre la estabilidad del terreno.`,
      });
      console.log(response.text);
      return response.text
    },
    async send_to_ubidots(dataFromEsp32)
    {
      const now = Date.now();
      if (now - this.lastUbidotsSentAt < this.ubidotsMinIntervalMs)//evitamos spaming al ubidots y que nos bloquien el token
      {
        return false;
      }
      this.lastUbidotsSentAt = now;

      const url="https://industrial.api.ubidots.com/api/v1.6/devices/esp32_soi";
      try
      {
        const res = await fetch(url, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'X-Auth-Token': import.meta.env.VITE_UBIDOTS_API_KEY
          },
          body: JSON.stringify(dataFromEsp32)
        });
        if (!res.ok) 
        {
          const text = await res.text();
          console.error('Ubidots error', res.status, text);
          return false;
        }
        return true;
      }
      catch(err) 
      {
        console.error('Network error', err);
        return false;
      }
      
    }
  },
  mounted() {
    this.getData();
  }
}
</script>


