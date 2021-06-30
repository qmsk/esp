import Vue from 'vue'
import Vuex from 'vuex'
import { createLogger } from 'vuex'

import APIService from './services/api.service'
import ConfigService from './services/config.service'
import SystemService from './services/system.service'

const apiService = new APIService();
const configService = new ConfigService(apiService);
const systemService = new SystemService(apiService);

Vue.use(Vuex);

export default new Vuex.Store({
  state: {
    config: null,
    system: null,
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
    async uploadConfig({ dispatch }, file) {
      await configService.upload(file);
      await dispatch('loadConfig');
    },

    /* system */
    async loadSystem({ commit }) {
      const data = await systemService.get();

      commit('updateSystem', data);
    },
    async restartSystem({ dispatch }) {
      await systemService.restart();

      await dispatch('loadSystem');
    }
  },
  mutations: {
    loadConfig (state, config) {
      state.config = config;
    },
    updateSystem(state, system) {
      state.system = system;
    },
  },
});
