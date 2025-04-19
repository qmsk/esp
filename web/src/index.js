import Vue from "vue";
import VueRouter from 'vue-router'
import Vuex from 'vuex'

import App from "./App"
import ArtNetView from "./components/ArtNetView"
import ConfigView from "./components/ConfigView"
import FilesView from "./components/FilesView"
import MainView from "./components/MainView"
import LedsView from "./components/LedsView"
import SystemView from "./components/SystemView"
import WiFiView from "./components/WiFiView"

import fileSizeFilter from "./filters/fileSize.filter"
import fileTimeFilter from "./filters/fileTime.filter"
import intervalFilter from "./filters/interval.filter";
import percentageFilter from "./filters/percentage.filter"
import timestampFilter from "./filters/timestamp.filter"

// global error handlers
Vue.config.errorHandler = (error, vm, info) => {
  alert(error.name + ": " + error.message);
};

window.addEventListener("unhandledrejection", (event) => {
  let error = event.reason;

  alert(error.name + ": " + error.message);
});

Vue.use(VueRouter);

Vue.filter('fileSize', fileSizeFilter);
Vue.filter('fileTime', fileTimeFilter);
Vue.filter('interval', intervalFilter);
Vue.filter('percentage', percentageFilter);
Vue.filter('timestamp', timestampFilter);

const router = new VueRouter({
  linkActiveClass: 'active',

  routes: [
    { path: '/',        component: MainView },
    { path: '/config',  component: ConfigView },
    { path: '/wifi',    component: WiFiView },
    { path: '/artnet',  component: ArtNetView },
    { path: '/leds',    component: LedsView },
    { path: '/files',   component: FilesView },
    { path: '/system',  component: SystemView },
  ],
});

import store from "./store"

var vue = new Vue({
  el: '#app',
  components: { App },
  template: '<App />',
  router,
  store,
});
