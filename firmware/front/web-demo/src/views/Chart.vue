<template>
  <v-container fluid>
    <v-row>
      <v-col cols="12" md="6">
        <h3>Acceleration Data</h3>
        <v-sparkline
          :value="accelerationX"
          :gradient="['#f72047', '#ffd200', '#1feaea']"
          :smooth="2"
          :padding="8"
          :line-width="1"
          stroke-linecap="round"
          gradient-direction="top"
          auto-draw
          class="sparkline-border"
        ></v-sparkline>
        <v-sparkline
          :value="accelerationY"
          :gradient="['#f72047', '#ffd200', '#1feaea']"
          :smooth="2"
          :padding="8"
          :line-width="1"
          stroke-linecap="round"
          gradient-direction="top"
          auto-draw
          class="sparkline-border"
        ></v-sparkline>
        <v-sparkline
          :value="accelerationZ"
          :gradient="['#f72047', '#ffd200', '#1feaea']"
          :smooth="2"
          :padding="8"
          :line-width="1"
          stroke-linecap="round"
          gradient-direction="top"
          auto-draw
          class="sparkline-border"
        ></v-sparkline>
      </v-col>
      <v-col cols="12" md="6">
        <h3>Angle Data</h3>
        <v-sparkline
          :value="angleX"
          :gradient="['#f72047', '#ffd200', '#1feaea']"
          :smooth="2"
          :padding="8"
          :line-width="1"
          stroke-linecap="round"
          gradient-direction="top"
          auto-draw
          class="sparkline-border"
        ></v-sparkline>
        <v-sparkline
          :value="angleY"
          :gradient="['#f72047', '#ffd200', '#1feaea']"
          :smooth="2"
          :padding="8"
          :line-width="1"
          stroke-linecap="round"
          gradient-direction="top"
          auto-draw
          class="sparkline-border"
        ></v-sparkline>
        <v-sparkline
          :value="angleZ"
          :gradient="['#f72047', '#ffd200', '#1feaea']"
          :smooth="2"
          :padding="8"
          :line-width="1"
          stroke-linecap="round"
          gradient-direction="top"
          auto-draw
          class="sparkline-border"
        ></v-sparkline>
      </v-col>
    </v-row>
    <v-row>
      <v-col cols="12" md="6">
        <v-card class="pa-3">
          <v-row>
            <v-col cols="12">
              <div class="data-item" style="background-color: aquamarine;">
                <strong>Acceleration X:</strong> {{ accelerationX[accelerationX.length - 1] }}
              </div>
            </v-col>
            <v-col cols="12">
              <div class="data-item">
                <strong>Angle X:</strong> {{ angleX[angleX.length - 1] }}
              </div>
            </v-col>
            <v-col cols="12">
              <div class="data-item" style="background-color: aquamarine;">
                <strong>Acceleration Y:</strong> {{ accelerationY[accelerationY.length - 1] }}
              </div>
            </v-col>
            <v-col cols="12">
              <div class="data-item">
                <strong>Angle Y:</strong> {{ angleY[angleY.length - 1] }}
              </div>
            </v-col>
            <v-col cols="12">
              <div class="data-item" style="background-color: aquamarine;">
                <strong>Acceleration Z:</strong> {{ accelerationZ[accelerationZ.length - 1] }}
              </div>
            </v-col>
            <v-col cols="12">
              <div class="data-item">
                <strong>Angle Z:</strong> {{ angleZ[angleZ.length - 1] }}
              </div>
            </v-col>
          </v-row>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      timer: null
    }
  },
  computed: {
    accelerationX() {
      return this.$store.state.accelerationData.map(data => data.x)
    },
    accelerationY() {
      return this.$store.state.accelerationData.map(data => data.y)
    },
    accelerationZ() {
      return this.$store.state.accelerationData.map(data => data.z)
    },
    angleX() {
      return this.$store.state.angleData.map(data => data.x)
    },
    angleY() {
      return this.$store.state.angleData.map(data => data.y)
    },
    angleZ() {
      return this.$store.state.angleData.map(data => data.z)
    }
  },
  methods: {
    updateData: function() {
      this.$store.dispatch("fetchData")
    }
  },
  mounted() {
    clearInterval(this.timer)
    this.timer = setInterval(this.updateData, 200);
  },
  destroyed: function() {
    clearInterval(this.timer)
  }
};
</script>

<style>
.sparkline-border {
  border: 2px solid #000;
  border-radius: 8px;
  padding: 4px;
}

.data-item {
  border: 1px solid #51c53a;
  padding: 8px;
  margin: 4px;
}
</style>
