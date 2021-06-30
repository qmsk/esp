import Vue from 'vue'
import Vuex from 'vuex'
import { createLogger } from 'vuex'

import APIService from './services/api.service'
import ConfigService from './services/config.service'

const apiService = new APIService();
const configService = new ConfigService(apiService);

Vue.use(Vuex);

export default new Vuex.Store({
  state: {
    config: null,
  },
  plugins: [
    // XXX: only during development
    createLogger(),
  ],
  actions: {
    async loadConfig({ commit }) {
      const config = await configService.get();

      commit('loadConfig', config);
    },
    async uploadConfig({ dispatch }, file) {
      await configService.upload(file);
      await dispatch('loadConfig');
    },
  },
  mutations: {
    loadConfig (state, config) {
      state.config = config;
    }
  },
});
