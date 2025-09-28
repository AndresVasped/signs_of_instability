
<template>
  <span>Humedad: {{humedad}}</span><br>
  <span>Lluvia: {{lluvia}}</span><br>
  <span>Roll: {{roll}}</span><br>
  <span>Pitch:{{pitch}}</span><br>
  <span>Fecha:{{fecha}}</span><br>
  <span>Alerta:{{alerta}}</span><br>
</template>


<style scoped>
  body
  {
    font-family: 'Inter', sans-serif;
    margin: 0;
    padding: 2rem;
    background: linear-gradient(135deg, var(--bg-start), var(--bg-end));
    color: var(--text-color);
    min-height: 100vh;
    box-sizing: border-box;
  }
</style>


<script>
import { ref, onMounted } from "vue";

export default 
{
  data() 
  {
    return {
      humedad: 0,
      lluvia: 0,
      roll: 0,
      pitch: 0,
      fecha: '',
      alerta: ''
    }
  },
  methods:{
    async getData()
    {
      const ESP32_IP = "192.168.145.115"; // la IP que le asignó el Wi-Fi
      const url = `http://${ESP32_IP}/data`;
      while(true)//constantemente va estar escuchando lo que le envia la esp32
      {
        try
        {
          const res=await fetch(url)
          const d=await res.json()
          console.log(d)
          //aqui enviariamos los datos para que se muestren
          // this.humedad = d.humedad;
        }
        catch(e)
        {
          console.log('error',e);
        }
          // espera 1 segundo antes de volver a pedir
          await new Promise(resolve => setTimeout(resolve, 1000));
      }
    },
  },
  mounted() {
    this.getData();
  }
}
</script>