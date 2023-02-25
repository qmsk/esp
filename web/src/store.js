import Vue from 'vue'
import Vuex from 'vuex'
import { createLogger } from 'vuex'

import APIService from './services/api.service'
import ConfigService from './services/config.service'
import SystemService from './services/system.service'
import VFSService from './services/vfs.service'
import WiFiService from './services/wifi.service'

const apiService = new APIService();
const configService = new ConfigService(apiService);
const systemService = new SystemService(apiService);
const vfsService = new VFSService(apiService);
const wifiService = new WiFiService(apiService);

Vue.use(Vuex);

export default new Vuex.Store({
  state: {
    config: null,
    system: null,
    wifi: null,
    wifi_scan: null,
    vfs: null,
  },
  plugins: [
    // XXX: only during development
    createLogger(),
  ],
  actions: {
    /* config */
    async loadConfig({ commit }) {
      const config = await configService.get();

      commit('loadConfig', config);
    },
    async postConfig({ state, dispatch }, formdata) {
      await configService.post(state.config, formdata);

      await dispatch('loadConfig');
    },
    async uploadConfig({ dispatch }, file) {
      await configService.upload(file);
      await dispatch('loadConfig');
    },

    /* system */
    async loadSystem({ commit }) {
      const data = await systemService.get();

      commit('updateSystem', data);
    },
    async loadSystemTasks({ commit }) {
      const data = await systemService.getTasks();

      commit('updateSystemTasks', data);
    },
    async restartSystem({ dispatch }) {
      await systemService.restart();

      await dispatch('loadSystem');
    },

    /* wifi */
    async loadWiFi({ commit }) {
      const data = await wifiService.get();

      commit('updateWiFi', data);
    },
    async scanWiFi({ commit }) {
      const data = await wifiService.scan();

      commit('updateWiFiScan', data);
    },

    /* VFS */
    async loadVFS({ commit }) {
      const data = await vfsService.getTree();

      commit('updateVFS', data);
    },
  },
  mutations: {
    loadConfig (state, config) {
      state.config = config;
    },
    updateSystem(state, system) {
      state.system = system;
    },
    updateSystemTasks(state, tasks) {
      let prevTasks = new Map();

      if (state.system && state.system.tasks) {
        for (const task of state.system.tasks) {
          prevTasks.set(task.number, task);
        }
      }

      for (const task of tasks) {
        const prevTask = prevTasks.get(task.number);

        if (prevTask) {
          task.prev_runtime = prevTask.runtime;
          task.prev_total_runtime = prevTask.total_runtime;
        }
      }

      state.system.tasks = tasks;
    },
    updateWiFi(state, wifi) {
      state.wifi = wifi;
    },
    updateWiFiScan(state, wifi_scan) {
      state.wifi_scan = wifi_scan;
    },
    updateVFS(state, vfs) {
      state.vfs = vfs;
    },
  },
});
