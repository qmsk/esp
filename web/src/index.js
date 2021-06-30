import Vue from "vue";
import VueRouter from 'vue-router'
import Vuex from 'vuex'

import App from "./App"
import ConfigView from "./components/ConfigView"
import SystemView from "./components/SystemView"

Vue.use(VueRouter);

const router = new VueRouter({
  linkActiveClass: 'active',

  routes: [
    { path: '/config', component: ConfigView },
    { path: '/system', component: SystemView },
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
