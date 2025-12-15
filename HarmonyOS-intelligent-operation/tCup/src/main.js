// The Vue build version to load with the `import` command
// (runtime-only or standalone) has been set in webpack.base.conf with an alias.

import ElementUI from 'element-ui'
import 'element-ui/lib/theme-chalk/index.css'

import Vue from 'vue'
import App from './App'
import router from './router'
import "babel-polyfill"

import * as echarts from 'echarts'
Vue.prototype.$echarts = echarts

// 导入自定义请求工具（替换直接使用axios）
import request from './utils/request'
Vue.prototype.$http = request

Vue.use(ElementUI)

Vue.config.productionTip = false

Vue.$httpRequestList = []

/* eslint-disable no-new */
new Vue({
  el: '#app',
  router,
  components: { App },
  template: '<App/>'
})

router.beforeEach((to, from, next) => {
  // 取消未完成的请求
  if (Vue.$httpRequestList.length > 0) {
    console.log('Pending request canceled!');
    Vue.$httpRequestList.forEach((item) => {
      item()
    })
    Vue.$httpRequestList = []
  }

  // 登录状态检查（路由守卫增强版）
  const isLogin = localStorage.getItem('isLogin')
  const isLoginPage = to.path === '/login'

  // 未登录且不是登录页 → 重定向到登录页
  if (!isLogin && !isLoginPage) {
    return next({ path: '/login', query: { redirect: to.fullPath } })
  }

  // 已登录且是登录页 → 重定向到首页
  if (isLogin && isLoginPage) {
    return next('/realtime')
  }

  next();
})