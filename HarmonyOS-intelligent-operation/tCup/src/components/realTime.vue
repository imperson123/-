<template>
  <div class="full-screen-container">
    <el-container style="background-color: #252525; height: 100vh;">
      <el-aside width="200px" class="sidebar">
        <el-menu 
          background-color="#252525" 
          text-color="#fff" 
          active-text-color="#12f0e0"
          class="sidebar-menu"
        >
          <el-menu-item index="1" @click="toMainPage" class="menu-item-clickable">
            <i class="el-icon-s-home"></i>
            <span slot="title">概览</span>
          </el-menu-item>
          <el-menu-item index="2" @click="toCPU" class="menu-item-clickable">
            <i class="el-icon-cpu"></i>
            <span slot="title">CPU利用率</span>
          </el-menu-item>
          <el-menu-item index="3" @click="toMemory" class="menu-item-clickable">
            <i class="el-icon-memory"></i>
            <span slot="title">内存占用率</span>
          </el-menu-item>
          <el-menu-item index="4" @click="toDisk" class="menu-item-clickable">
            <i class="el-icon-disk"></i>
            <span slot="title">磁盘占用率</span>
          </el-menu-item>
          <el-menu-item index="5" @click="toNetwork" class="menu-item-clickable">
            <i class="el-icon-link"></i>
            <span slot="title">以太网</span>
          </el-menu-item>
          <el-menu-item index="6" @click="toConfigManager" class="menu-item-clickable">
            <i class="el-icon-setting"></i>
            <span slot="title">量化模型监控配置</span>
          </el-menu-item>
        </el-menu>
      </el-aside>
      <el-main class="main-content">
        <router-view></router-view>
      </el-main>
    </el-container>
  </div>
</template>

<script>
export default {
  name: 'RealTime',
  methods: {
    toMainPage() {
      this.$router.push('/realtime/mainPage');
    },
    toCPU() {
      this.$router.push('/realtime/cpu');
    },
    toMemory() {
      this.$router.push('/realtime/memory');
    },
    toDisk() {
      this.$router.push('/realtime/disk');
    },
    toNetwork() {
      this.$router.push('/realtime/network');
    },
    toConfigManager() {
      this.$router.push('/overview/configs');
    }
  },
  mounted() {
    // 页面加载时不自动请求数据，点击菜单项时跳转并由子页面加载数据
  }
};
</script>

<style scoped>
.full-screen-container {
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  margin: 0;
  padding: 0;
}

.el-container {
  height: 100%;
  width: 100%;
}

.sidebar {
  height: 100%;
  background-color: #252525 !important;
  border-right: 1px solid #444;
}

.main-content {
  height: 100%;
  padding: 20px;
  margin: 0;
  background-color: #1a1a1a;
}

/* 修复菜单样式 */
.el-menu {
  border-right: none;
}

.el-menu-item {
  height: 60px;
  line-height: 60px;
  font-size: 16px;
  cursor: pointer;
  transition: all 0.3s ease;
}

.el-menu-item:hover {
  background-color: rgba(18, 240, 224, 0.1) !important;
  color: #12f0e0 !important;
}

.el-menu-item.is-active {
  background-color: rgba(18, 240, 224, 0.2) !important;
  color: #12f0e0 !important;
  border-right: 3px solid #12f0e0;
}

.menu-item-clickable {
  pointer-events: auto !important;
  cursor: pointer !important;
}

.el-menu-item .el-icon {
  margin-right: 10px;
  font-size: 18px;
}
</style>