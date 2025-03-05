<template>
  <v-app>
    <v-app-bar color="primary">
      <v-app-bar-nav-icon @click="drawer = !drawer"></v-app-bar-nav-icon>
      <v-toolbar-title>Solar Tracker</v-toolbar-title>
    </v-app-bar>

    <v-navigation-drawer v-model="drawer" bottom absolute temporary>
      <v-list dense nav>
        <v-list-item v-for="(item, i) in navItems" :key="i" :value="item" :to="item.route">
          <template v-slot:prepend>
            <v-icon :icon="item.icon"></v-icon>
          </template>
          <v-list-item-title>{{ item.text }}</v-list-item-title>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-main>
      <router-view></router-view>
    </v-main>
  </v-app>
</template>

<script setup>
import { ref } from 'vue'

const drawer = ref(true)
const navItems = [
  { text: 'Home', icon: 'home', route: '/' },
  { text: 'Control', icon: 'highlight', route: '/control' },
  { text: 'Analysis', icon: 'show_chart', route: '/analysis' },
]
</script>

<style scoped></style>
