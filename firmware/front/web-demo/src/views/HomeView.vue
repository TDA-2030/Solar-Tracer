<template>
  <v-container fluid>
    <v-row>
      <!-- 实时数据展示 -->
      <v-col cols="12" md="5">
        <v-card>
          <v-card-title>Current Status</v-card-title>
          <v-card-text>
            <v-row>
              <v-col v-for="(value, key) in realtimeData" :key="key" cols="6">
                <v-card variant="outlined">
                  <v-card-text class="text-center">
                    <div class="text-h6">{{ key }}</div>
                    <div class="text-h4 text-primary">{{ value }}</div>
                  </v-card-text>
                </v-card>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>

      <!-- 图表展示 -->
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>Realtime Data</v-card-title>
          <v-card-text>
            <v-container>
              <h3>Angle</h3>
              <v-sparkline v-for="(axis, index) in axisName" :key="index" 
                :gradient="['#f72047',]" :smooth="2" :padding="8" :line-width="1" auto-draw
                class="sparkline" :model-value="getAngleData(index)"></v-sparkline>

              <v-row no-gutters v-for="(axis, index) in axisName" :key="index" class="pa-3"
                :style="{ color: axis.color }">
                <v-col cols="12" sm="5">
                  <strong>Acceleration {{ axis.name }}:</strong> {{ accelerationData[accelerationData.length - 1][axis.name.toLocaleLowerCase()] }}
                </v-col>
                <v-col cols="12" sm="5">
                  <strong>Angle {{ axis.name }}:</strong> {{ angleData[angleData.length - 1][axis.name.toLocaleLowerCase()] }}
                </v-col>
              </v-row>
            </v-container>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { onMounted, onUnmounted } from 'vue'
import { useSolarStore } from '../stores/solarStore'
import { storeToRefs } from 'pinia'
import axios from 'axios'

const store = useSolarStore()
const { realtimeData, accelerationData, angleData } = storeToRefs(store)


const axisName = [
  { name: 'X', color: 'red' },
  { name: 'Y', color: 'green' },
  { name: 'Z', color: 'blue' }
]

// Expected data format:
// {
//   acceleration: { time: 'HH:mm:ss', x: number, y: number, z: number },
//   angle: { time: 'HH:mm:ss', x: number, y: number, z: number }
// }
// Fetch data from API and update store
const fetchData = () => {
  axios.get('/api/v1/temp/raw')
    .then(response => {
      const data = response.data
      store.updateAngles({
        _angleData: data.angle,
        _accelerationData: data.acc
      })
    })
    .catch(error => {
      console.error('Error fetching data:', error)
    })
}

let intervalId

onMounted(() => {
  console.log('HomeView mounted')
  intervalId = setInterval(fetchData, 150)
})

onUnmounted(() => {
  console.log('HomeView unmounted')
  clearInterval(intervalId)
})

const getAngleData = (index) => {
  return angleData.value.map(data => data[axisName[index].name.toLowerCase()])
}

</script>

<style scoped>
.sparkline {
  border: 1px solid #000;
  border-radius: 2px;
  padding: 4px;
  margin-bottom: 5px;
}
</style>