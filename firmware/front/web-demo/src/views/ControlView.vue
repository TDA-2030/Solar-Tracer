<template>
  <v-container>
    <v-row>
      <!-- 模式控制 -->
      <v-col cols="12">
        <v-radio-group v-model="mode" inline>
          <v-radio label="Auto Mode" value="auto"></v-radio>
          <v-radio label="Manual Mode" value="manual"></v-radio>
        </v-radio-group>
      </v-col>

      <!-- 角度控制 -->
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>Angle Control</v-card-title>
          <v-card-text>
            <v-slider v-model="controlData.azimuth" label="Azimuth" min="0" max="360" step="1" thumb-label
              :disabled="mode === 'auto'"></v-slider>

            <v-slider v-model="controlData.elevation" label="Elevation" min="0" max="90" step="1" thumb-label
              :disabled="mode === 'auto'"></v-slider>
          </v-card-text>
        </v-card>
      </v-col>

      <!-- 阈值设置 -->
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>Threshold Settings</v-card-title>
          <v-card-text>
            <v-text-field v-model="controlData.angleDeviation" label="Angle Deviation (°)" type="number"></v-text-field>

            <v-range-slider v-model="voltageRange" label="Voltage Range" min="0" max="50" step="0.1"
              thumb-label></v-range-slider>
            <v-btn variant="outlined" color="red accent-4" @click="set_color">
              Save
            </v-btn>
          </v-card-text>

        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { computed } from 'vue' // 添加 computed 导入
import { useSolarStore } from '../stores/solarStore'
import { storeToRefs } from 'pinia'
import axios from 'axios'

const store = useSolarStore()
const { controlData } = storeToRefs(store)

const mode = computed({
  get: () => controlData.value.mode,
  set: (val) => {
    controlData.value.mode = val
  }
})

const voltageRange = computed({
  get: () => [controlData.value.voltageMin, controlData.value.voltageMax],
  set: (val) => {
    controlData.value.voltageMin = val[0]
    controlData.value.voltageMax = val[1]
  }
})

const set_color = () => {
  axios.post("/api/v1/light/brightness", {
    data: controlData.value
  })
    .then(data => {
      console.log(data.data);
    })
    .catch(error => {
      console.log(error);
    });
}
</script>
