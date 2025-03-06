<template>
  <v-container>
    <v-row>
      <!-- 模式控制 -->
      <v-col cols="12" >
        <v-row align="center">
          <v-col cols="6">
            <v-radio-group v-model="mode" inline>
              <v-radio label="Auto Mode" value="auto"></v-radio>
              <v-radio label="Manual Mode" value="manual"></v-radio>
            </v-radio-group>
          </v-col>
          <v-col cols="6" class="text-right">
            <v-btn class="mr-4" color="teal" variant="elevated" @click="readSettingData">
              Load
            </v-btn>
            <v-btn color="amber" variant="elevated" @click="sendSettingData">
              Save
            </v-btn>
          </v-col>
        </v-row>
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
          </v-card-text>

        </v-card>
      </v-col>

      <!-- PID控制器设置 -->
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>Position PID Control</v-card-title>
          <v-card-text>
            <v-slider v-model="controlData.pos_pid.Kp" label="Kp" min="0" max="1" step="0.01" thumb-label></v-slider>
            <v-slider v-model="controlData.pos_pid.Ki" label="Ki" min="0" max="1" step="0.01" thumb-label></v-slider>
            <v-slider v-model="controlData.pos_pid.Kd" label="Kd" min="0" max="1" step="0.01" thumb-label></v-slider>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>Velocity PID Control</v-card-title>
          <v-card-text>
            <v-slider v-model="controlData.vel_pid.Kp" label="Kp" min="0" max="1" step="0.01" thumb-label></v-slider>
            <v-slider v-model="controlData.vel_pid.Ki" label="Ki" min="0" max="1" step="0.01" thumb-label></v-slider>
            <v-slider v-model="controlData.vel_pid.Kd" label="Kd" min="0" max="1" step="0.01" thumb-label></v-slider>
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

const sendSettingData = () => {
  axios.post("/api/v1/setting", {
    pid: {
        pos: {
          p: controlData.value.pos_pid.Kp,
          i: controlData.value.pos_pid.Ki,
          d: controlData.value.pos_pid.Kd,
        },
        vel: {
          p: controlData.value.vel_pid.Kp,
          i: controlData.value.vel_pid.Ki,
          d: controlData.value.vel_pid.Kd,
        },
      },
      mode: controlData.value.mode,
      th: {
        maxv: controlData.value.voltageMax,
        minv: controlData.value.voltageMin,
      },
      man: {
        pitch: controlData.value.elevation,
        yaw: controlData.value.azimuth,
      },
  })
    .then(data => {
      console.log(data.data);
    })
    .catch(error => {
      console.log(error);
    });
}

const readSettingData = () => {
  axios.get('/api/v1/setting')
    .then(response => {
      const data = response.data
      store.saveSettingData({
        mode: data.mode,
        azimuth: data.man.yaw,
        elevation: data.man.pitch,
        angleDeviation: data.th.maxv - data.th.minv,
        voltageMin: data.th.minv,
        voltageMax: data.th.maxv,
        pos_pid: {
          Kp: data.pid.pos.p,
          Ki: data.pid.pos.i,
          Kd: data.pid.pos.d,
        },
        vel_pid: {
          Kp: data.pid.vel.p,
          Ki: data.pid.vel.i,
          Kd: data.pid.vel.d,
        }
      })
    })
    .catch(error => {
      console.error('Error fetching data:', error)
    })
}
</script>
