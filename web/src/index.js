import Vue from "vue";
import VueRouter from 'vue-router'
import Vuex from 'vuex'

import App from "./App"
import ConfigView from "./components/ConfigView"
import FilesView from "./components/FilesView"
import SystemView from "./components/SystemView"
import WiFiView from "./components/WiFiView"

import fileSizeFilter from "./filters/fileSize.filter"
import fileTimeFilter from "./filters/fileTime.filter"

Vue.use(VueRouter);

Vue.filter('fileSize', fileSizeFilter);
Vue.filter('fileTime', fileTimeFilter);

const router = new VueRouter({
  linkActiveClass: 'active',

  routes: [
    { path: '/config',  component: ConfigView },
    { path: '/wifi',    component: WiFiView },
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
