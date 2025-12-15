const state = {
  configs: JSON.parse(localStorage.getItem('monitorConfigs')) || [
    // 默认测试数据
    {
      id: 1,
      name: 'CPU高负载阈值',
      description: '系统CPU利用率告警阈值',
      threshold: 85.5,
      created_at: new Date().toISOString()
    },
    {
      id: 2,
      name: '内存使用率告警',
      description: '内存占用率监控阈值',
      threshold: 90.0,
      created_at: new Date().toISOString()
    }
  ]
};

const mutations = {
  ADD_CONFIG(state, config) {
    config.id = Date.now();
    config.created_at = new Date().toISOString();
    state.configs.push(config);
    localStorage.setItem('monitorConfigs', JSON.stringify(state.configs));
  },
  UPDATE_CONFIG(state, updatedConfig) {
    const index = state.configs.findIndex(c => c.id === updatedConfig.id);
    if (index !== -1) {
      state.configs.splice(index, 1, updatedConfig);
      localStorage.setItem('monitorConfigs', JSON.stringify(state.configs));
    }
  },
  DELETE_CONFIG(state, id) {
    state.configs = state.configs.filter(c => c.id !== id);
    localStorage.setItem('monitorConfigs', JSON.stringify(state.configs));
  }
};

const actions = {
  addConfig({ commit }, config) {
    commit('ADD_CONFIG', config);
  },
  updateConfig({ commit }, config) {
    commit('UPDATE_CONFIG', config);
  },
  deleteConfig({ commit }, id) {
    commit('DELETE_CONFIG', id);
  }
};

export default {
  namespaced: true,
  state,
  mutations,
  actions
};