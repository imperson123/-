import Vue from 'vue'
import Router from 'vue-router'
import HelloWorld from '@/components/HelloWorld'
import Login from '@/components/Login'  // 导入登录组件
import ConfigManager from '@/components/ConfigManager'  // 导入配置管理组件

Vue.use(Router)

// 路由守卫函数
function requireAuth(to, from, next) {
  // 检查是否已登录
  if (localStorage.getItem('isLogin')) {
    next();  // 已登录，继续访问
  } else {
    next({ path: '/login', query: { redirect: to.fullPath } });  // 未登录，重定向到登录页
  }
}

export default new Router({
  routes: [
    // 登录路由
    {
      path: '/login',
      name: 'Login',
      component: Login,
      // 已登录用户访问登录页时重定向到概览页
      beforeEnter: (to, from, next) => {
        if (localStorage.getItem('isLogin')) {
          next('/overview');
        } else {
          next();
        }
      }
    },
    {
      path: '/',
      name: 'HelloWorld',
  redirect: "/overview/configs",
      component: HelloWorld,
      children: [
        {
          path: "/realtime",
          name: "RealTime",
          redirect: "/realtime/mainPage",
          component: () => import("../components/realTime"),
          beforeEnter: requireAuth,  // 应用路由守卫
          children: [
            {
              path: "/realtime/mainPage",
              name: "mainPage",
              component: () => import("../components/result/realTime/mainPage"),
              meta: {
                firstMenu: '/realtime',
                secondMenu: '/realtime'
              }
            },
            {
              path: "/realtime/cpu",
              name: "cpu",
              component: () => import("../components/result/realTime/cpu"),
              meta: {
                firstMenu: '/realtime',
                secondMenu: '/realtime'
              }
            },
            {
              path: "/realtime/disk",
              name: "disk",
              component: () => import("../components/result/realTime/disk"),
              meta: {
                firstMenu: '/realtime',
                secondMenu: '/realtime'
              }
            },
            {
              path: "/realtime/memory",
              name: "memory",
              component: () => import("../components/result/realTime/memory"),
              meta: {
                firstMenu: '/realtime',
                secondMenu: '/realtime'
              }
            },
            {
              path: "/realtime/network",
              name: "network",
              component: () => import("../components/result/realTime/network"),
              meta: {
                firstMenu: '/realtime',
                secondMenu: '/realtime'
              }
            }
          ]
        },
        {
          path: "/check",
          name: "Check",
          redirect: "/check/M1D1",
          component: () => import("../components/Check"),
          beforeEnter: requireAuth,  // 应用路由守卫
          children: [
            {
              path: "/check/M1D1",
              name: "CheckM1D1",
              component: () => import("../components/result/check/M1D1"),
              meta: {
                firstMenu: '/check',
                secondMenu: '/check/M1D1'
              }
            },
            {
              path: "/check/M1D2",
              name: "CheckM1D2",
              component: () => import("../components/result/check/M1D2"),
              meta: {
                firstMenu: '/check',
                secondMenu: '/check/M1D2'
              }
            },
            {
              path: "/check/M1D3",
              name: "CheckM1D3",
              component: () => import("../components/result/check/M1D3"),
              meta: {
                firstMenu: '/check',
                secondMenu: '/check/M1D3'
              }
            }
          ]
        },
        {
          path: "/prediction",
          name: "Prediction",
          redirect: "/prediction/M1D0",
          component: () => import("../components/Prediction"),
          beforeEnter: requireAuth,  // 应用路由守卫
          children: [
            {
              path: "/prediction/M1D0",
              name: "M1D0",
              component: () => import("../components/result/prediction/M1D0"),
              meta: {
                firstMenu: '/prediction',
                secondMenu: '/prediction/M1D0'
              }
            },
            {
              path: "/prediction/M1D1",
              name: "PredictionM1D1",
              component: () => import("../components/result/prediction/M1D1"),
              meta: {
                firstMenu: '/prediction',
                secondMenu: '/prediction/M1D1'
              }
            },
            {
              path: "/prediction/M1D2",
              name: "PredictionM1D2",
              component: () => import("../components/result/prediction/M1D2"),
              meta: {
                firstMenu: '/prediction',
                secondMenu: '/prediction/M1D2'
              }
            },
            {
              path: "/prediction/M1D3",
              name: "PredictionM1D3",
              component: () => import("../components/result/prediction/M1D3"),
              meta: {
                firstMenu: '/prediction',
                secondMenu: '/prediction/M1D3'
              }
            }
          ]
        },
        // 添加配置管理路由（CRUD页面）
        {
          path: "/overview/configs",
          name: "ConfigManager",
          component: ConfigManager,
          // 临时移除登录守卫，便于开发时直接打开配置管理页面
          meta: {
            firstMenu: '/overview',
            secondMenu: '/overview/configs'
          }
        },
        // NFT报告路由
        {
          path: "/nft-report",
          name: "NFTReport",
          component: () => import("../components/NFTReport"),
          beforeEnter: requireAuth,
          meta: {
            firstMenu: '/nft-report',
            secondMenu: '/nft-report'
          }
        },
        // 元宇宙中心路由
        {
          path: "/metaverse-hub",
          name: "MetaverseHub",
          component: () => import("../components/MetaverseHub"),
          beforeEnter: requireAuth,
          meta: {
            firstMenu: '/metaverse-hub',
            secondMenu: '/metaverse-hub'
          }
        },
        // NFT市场路由
        {
          path: "/nft-market",
          name: "NFTMarket",
          component: () => import("../components/NFTMarket"),
          beforeEnter: requireAuth,
          meta: {
            firstMenu: '/nft-market',
            secondMenu: '/nft-market'
          }
        }
      ]
    }
  ]
})