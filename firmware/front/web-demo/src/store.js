import Vue from 'vue'
import Vuex from 'vuex'
import axios from 'axios'

Vue.use(Vuex)

export default new Vuex.Store({
  state: {
    accelerationData: Array(20).fill({ x: 0, y: 0, z: 0 }),
    angleData: Array(20).fill({ x: 0, y: 0, z: 0 })
  },
  mutations: {
    updateAccelerationData(state, newData) {
      state.accelerationData.push(newData)
      if (state.accelerationData.length > 20) {
        state.accelerationData.shift()
      }
    },
    updateAngleData(state, newData) {
      state.angleData.push(newData)
      if (state.angleData.length > 20) {
        state.angleData.shift()
      }
    }
  },
  actions: {
    fetchData({ commit }) {
      axios.get('/api/v1/temp/raw')
        .then(response => {
          const data = response.data
          // Expected data format:
          // {
          //   acceleration: { time: 'HH:mm:ss', x: number, y: number, z: number },
          //   angle: { time: 'HH:mm:ss', x: number, y: number, z: number }
          // }
          commit('updateAccelerationData', data.acceleration)
          commit('updateAngleData', data.angle)
        })
        .catch(error => {
          console.error(error)
        })
    }
  }
})
