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
        <p>{{ roll != null ? roll.toFixed(2) : '0.00' }}°</p>
      </div>
      <div class="card">
        <h2>📏 Pitch</h2>
        <p>{{ pitch != null ? pitch.toFixed(2) : '0.00' }}°</p>
      </div>
      <div class="card">
        <h2>🧭 Inclinación</h2>
        <p>{{ inclinacion != null ? inclinacion.toFixed(2) : '0.00' }}°</p>
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
import { GoogleGenAI } from "@google/genai";
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
      messageFor30: '',
      last60: [],
      messageFor60: '',
      last120: [],
      messageFor120: '',
      lastUbidotsSentAt: 0,
      ubidotsMinIntervalMs: 5000
    }
  },
  methods: {
    async getData() {
      // Save client to `this` so other methods can use it
      this.client = mqtt.connect("ws://192.168.11.152:1884"); // use websocket port

      this.client.on('connect', () => {
        console.log('Conectado al broker MQTT por ws');
        this.client.subscribe('esp32/sensors');
      });

      this.client.on('message', async (topic, message) => {
        try {
          console.log('Mensaje de topic ' + topic + ": " + message);
          const d = JSON.parse(message.toString());
          this.humedad = d.humedad;
          this.lluvia = d.lluvia_mmph;
          this.roll = Number(d.roll) || 0;
          this.pitch = Number(d.pitch) || 0;
          this.inclinacion = Number(d.inclinacion) || 0;
          this.fecha = d.fecha;
          this.alerta = d.riesgo || d.alerta;

          // Await AI recommendation
          await this.lastAlerts();
        } catch (err) {
          console.error('Error al parsear mensaje MQTT:', err);
        }
      });

    },
    async fetchDataHTTP() {
      const ESP32_IP = "192.168.200.115";
      try {
        const res = await fetch(`http://${ESP32_IP}/data`);
        const d = await res.json();
        this.humedad = d.humedad;
        this.lluvia = d.lluvia_mmph;
        this.roll = Number(d.roll);
        this.pitch = Number(d.pitch);
        this.inclinacion = Number(d.inclinacion);
        this.fecha = d.fecha;
        this.alerta = d.riesgo || d.alerta;
        console.log("current alert", d.riesgo)
        await this.lastAlerts();
      } catch (err) {
        console.error("HTTP fetch failed:", err);
      }
    },


    async controlBuzzer(state) {
      const ESP32_IP = "192.168.200.115";
      try {

        await fetch(`http://${ESP32_IP}/buzzer?state=${state ? 1 : 0}`);
      } catch (e) {
        console.log("Error controlling buzzer", e);
      }
    },
    async lastAlerts() {
      if (!this.alerta) return;

      const date = new Date();
      const formateDate = this.formateDateTime(date);

      if (this.alerta.includes("ALERTA PRECAUTIVA")) {
        const message = `ALERTA PRECAUTIVA: Condiciones de lluvia y humedad elevadas mantenidas por 30 segundos (${formateDate}).`;
        this.last30.unshift(message);
        this.messageFor30 = await this.AI(this.humedad, this.lluvia, this.inclinacion, this.roll, this.pitch);
        if (this.last30.length > 3) this.last30.pop();
      }
      else if (this.alerta.includes("ALERTA MEDIA")) {
        const message = `ALERTA MEDIA: Condiciones de lluvia y humedad elevadas mantenidas por 60 segundos (${formateDate}).`;
        this.last60.unshift(message);
        this.messageFor60 = await this.AI(this.humedad, this.lluvia, this.inclinacion, this.roll, this.pitch);
        if (this.last60.length > 3) this.last60.pop();
      }
      else if (this.alerta.includes("ALERTA CRITICA")) {
        const message = `ALERTA CRITICA: Condiciones de lluvia y humedad elevadas mantenidas por 120 segundos (${formateDate}).`;
        this.last120.unshift(message);
        this.messageFor120 = await this.AI(this.humedad, this.lluvia, this.inclinacion, this.roll, this.pitch);
        if (this.last120.length > 3) this.last120.pop();
      }
    },
    formateDateTime(date) {
      const year = date.getFullYear();
      // getMonth() es zero-based, por eso se suma 1
      const month = String(date.getMonth() + 1).padStart(2, '0');
      const day = String(date.getDate()).padStart(2, '0');
      const hours = String(date.getHours()).padStart(2, '0');
      const minutes = String(date.getMinutes()).padStart(2, '0');
      const seconds = String(date.getSeconds()).padStart(2, '0');
      return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`;
    },
    async AI(humedad, lluvia, inclinacionTotal, roll, pitch) {
      try {
        console.log("Gemini API key:", import.meta.env.VITE_GEMINI_API_KEY);

        const ai = new GoogleGenAI({
          apiKey: import.meta.env.VITE_GEMINI_API_KEY
        });

        console.log("Sending to AI:", { humedad, lluvia, inclinacionTotal, roll, pitch });

        const result = await ai.models.generateContent({
          model: "gemini-2.0-flash",
          temperature: 0.7,
          candidateCount: 1,
          contents: [
            {
              type: "text",
              text: `Datos de sensores:
          - Humedad: ${humedad}%
          - Lluvia: ${lluvia} mm/h
          - Inclinación total: ${inclinacionTotal}°
          - Roll: ${roll}°
          - Pitch: ${pitch}°

          Basado en estos valores, genera un mensaje breve indicando:
          1️⃣ Nivel de riesgo.
          2️⃣ Observación general.
          3️⃣ Recomendación para la comunidad.`
            }
          ]
        });

        // Extract text safely
        const candidate = result?.candidates?.[0];
        const text = candidate?.content?.parts?.[0] || "⚠️ No se generó texto";

        console.log("AI recommendation:", text);
        return text;

      } catch (err) {
        console.error("AI error:", err);
        return "⚠️ Error generando recomendación por IA.";
      }
    },
    async send_to_ubidots(dataFromEsp32) {
      const now = Date.now();
      if (now - this.lastUbidotsSentAt < this.ubidotsMinIntervalMs)//evitamos spaming al ubidots y que nos bloquien el token
      {
        return false;
      }
      this.lastUbidotsSentAt = now;

      const url = "https://industrial.api.ubidots.com/api/v1.6/devices/esp32_soi";
      try {
        const res = await fetch(url, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'X-Auth-Token': import.meta.env.VITE_UBIDOTS_API_KEY
          },
          body: JSON.stringify(dataFromEsp32)
        });
        if (!res.ok) {
          const text = await res.text();
          console.error('Ubidots error', res.status, text);
          return false;
        }
        return true;
      }
      catch (err) {
        console.error('Network error', err);
        return false;
      }

    }
  },
  mounted() {
    this.getData();
    setInterval(this.fetchDataHTTP, 3000);
  }
}
</script>
