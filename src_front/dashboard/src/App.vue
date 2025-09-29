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
  </div>
</template>

<script>
export default {
  data() {
    return {
      humedad: 0,
      lluvia: 0,
      roll: 0,
      pitch: 0,
      inclinacion: 0,
      fecha: '',
      alerta: ''
    }
  },
  methods: {
    async getData() {
      const ESP32_IP = "192.168.145.115"; 
      const url = `http://${ESP32_IP}/data`;
      while (true) {
        try {
          const res = await fetch(url);
          const d = await res.json();
          this.humedad = d.humedad;
          this.lluvia = d.lluvia_mmph;
          this.roll = d.roll;
          this.pitch = d.pitch;
          this.inclinacion = d.inclinacion;
          this.fecha = d.fecha;
          this.alerta = d.riesgo || d.alerta;
        } catch (e) {
          console.log('error', e);
        }
        await new Promise(resolve => setTimeout(resolve, 1000));
      }
    },

    async controlBuzzer(state) {
      const ESP32_IP = "192.168.145.115"; 
      try {
        
        await fetch(`http://${ESP32_IP}/buzzer?state=${state ? 1 : 0}`);
      } catch (e) {
        console.log("Error controlling buzzer", e);
      }
    }
  },
  mounted() {
    this.getData();
  }
}
</script>
