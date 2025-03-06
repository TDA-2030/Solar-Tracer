import { defineStore } from 'pinia'

export const useSolarStore = defineStore('solar', {
  state: () => ({
    angleData: Array(20).fill({ x: 0, y: 0, z: 0 }),
    accelerationData: Array(20).fill({ x: 0, y: 0, z: 0 }),
    realtimeData: {
      azimuth: 45,
      elevation: 30,
      light: 850,
      temperature: 28.5,
      voltage: 24.3,
      current: 5.2
    },
    controlData: {
      mode: 'auto', // 'auto' or 'manual'
      azimuth: 45,
      elevation: 30,
      angleDeviation: 5,
      voltageMin: 20,
      voltageMax: 30,
      pos_pid: {
        Kp: 0.1,
        Ki: 0.01,
        Kd: 0.01,
      },
      vel_pid: {
        Kp: 0.1,
        Ki: 0.01,
        Kd: 0.01,
      }
    },
  }),
  actions: {
    updateAngles(newValues) {
      this.angleData.push(newValues._angleData)
      if (this.angleData.length > 20) {
        this.angleData.shift()
      }

      this.accelerationData.push(newValues._accelerationData)
      if (this.accelerationData.length > 20) {
        this.accelerationData.shift()
      }
      this.realtimeData.azimuth = newValues._angleData.x
      this.realtimeData.elevation = newValues._angleData.y
    },
    saveSettingData(newSet) {
      this.controlData = newSet
    },
  },
  getters: {
    // Remove unused getters
    // angleDataX: (state) => state.angleData.map(data => data.x),
    // angleDataY: (state) => state.angleData.map(data => data.y),
    // angleDataZ: (state) => state.angleData.map(data => data.z)
  },
  
})