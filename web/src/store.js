import Vue from 'vue'
import Vuex from 'vuex'
import { createLogger } from 'vuex'

import APIService from './services/api.service'
import ArtNetService from './services/artnet.service'
import ConfigService from './services/config.service'
import LedsService from './services/leds.service'
import SystemService from './services/system.service'
import VFSService from './services/vfs.service'
import WiFiService from './services/wifi.service'

const apiService = new APIService();
const artnetService = new ArtNetService(apiService);
const configService = new ConfigService(apiService);
const ledsService = new LedsService(apiService);
const systemService = new SystemService(apiService);
const vfsService = new VFSService(apiService);
const wifiService = new WiFiService(apiService);

Vue.use(Vuex);

export default new Vuex.Store({
  state: {
    artnet: null,
    artnet_inputs: null,
    artnet_outputs: null,
    config: null,
    leds: null,
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

    /* artnet */
    async loadArtNet({ commit }) {
      const data = await artnetService.get();

      commit('updateArtNet', data);
    },
    async loadArtNetInputs({ commit }) {
      const data = await artnetService.getInputs();

      commit('updateArtNetInputs', data);
    },
    async loadArtNetOutputs({ commit }) {
      const data = await artnetService.getOutputs();

      commit('updateArtNetOutputs', data);
    },

    /* leds */
    async loadLeds({ commit }) {
      const data = await ledsService.get();

      commit('updateLeds', data);
    },
    async loadLedsStatus({ commit }, id) {
      const status = await ledsService.getStatus(id);

      commit('updateLedsStatus', {id, status});
    },

    /* VFS */
    async loadVFS({ commit }) {
      const vfsState = await vfsService.get();

      commit('updateVFS', vfsState);
    },
    async loadVFSRoot({ commit }, { vfsPath  }) {
      const vfsItem = await vfsService.getRoot(vfsPath);

      commit('setVFSRoot', vfsItem );
    },
    async loadVFSDirectory({ commit }, { vfsPath, path }) {
      const item = await vfsService.getDirectory(vfsPath, path);

      commit('setVFSItem', { vfsPath, item });
    },
    async createVFSDirectory({ commit }, { vfsPath, path }) {
      const item = await vfsService.createDirectory(vfsPath, path);

      commit('setVFSItem', { vfsPath, item });
    },
    async uploadFile({ commit }, { vfsPath, path, file, progress }) {
      const item = await vfsService.uploadFile(vfsPath, path, file, progress);

      commit('setVFSItem', { vfsPath, item });
    },
    async deleteFile({ commit }, { vfsPath, path }) {
      const meta = await vfsService.delete(vfsPath, path);

      commit('delVFSItem', { vfsPath, path, meta });
    },
    async deleteDirectory({ commit }, { vfsPath, path }) {
      const meta = await vfsService.deleteDirectory(vfsPath, path);

      commit('delVFSItem', { vfsPath, path, meta });
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

    updateLeds(state, leds) {
      state.leds = leds;
    },
    updateLedsStatus(state, {id, status}) {
      // no reactive map support
      let leds = new Map(state.leds);
      leds.get(id).status = status;
      state.leds = leds;
    },

    updateArtNet(state, artnet) {
      state.artnet = artnet;
      state.artnet_inputs = artnet.inputs;
      state.artnet_outputs = artnet.outputs;
    },
    updateArtNetInputs(state, artnetInputs) {
      state.artnet_inputs = artnetInputs;
    },
    updateArtNetOutputs(state, artnetOutputs) {
      state.artnet_outputs = artnetOutputs;
    },

    updateVFS(state, vfsState) {
      state.vfs = vfsState;
    },
    setVFSRoot(state, vfsItem) {
      vfsService.setVFSRoot(state.vfs, vfsItem);
    },
    setVFSItem(state, { vfsPath, item }) {
      const vfs = state.vfs.map.get(vfsPath);

      if (item.vfs_stat) {
        vfsService.setVFSStat(vfs, item.vfs_stat);
      }

      vfsService.setVFS(vfs, item);
    },
    delVFSItem(state, { vfsPath, path, meta }) {
      const vfs = state.vfs.map.get(vfsPath);

      if (meta.vfs_stat) {
        vfsService.setVFSStat(vfs, meta.vfs_stat);
      }

      vfsService.delVFS(vfs, path);
    },
  },
});
