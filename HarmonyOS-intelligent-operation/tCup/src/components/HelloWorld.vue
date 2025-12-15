<template>
  <div height="100%">
  <el-container>
      <el-header v-if="showHeader" height="142px" class="header-align">
        <div class="header-inner">
          <el-menu :default-active="activeMenu"
                   router
                   @open="handleOpen"
                   @close="handleClose"
                   mode="horizontal"
                   background-color="#252525"
                   text-color="white"
                   active-text-color="#409EFF"
                   class="header-menu">
            <el-menu-item class="header-item logo-title">
              <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
                <path stroke="none" d="M0 0h24v24H0z" fill="none"></path>
                <path d="M14 3v4a1 1 0 0 0 1 1h4"></path>
                <path d="M17 21h-10a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2h7l5 5v11a2 2 0 0 1 -2 2z"></path>
                <path d="M12 11.5a3 3 0 0 0 -3 2.824v1.176a3 3 0 0 0 6 0v-1.176a3 3 0 0 0 -3 -2.824z"></path>
              </svg>
              <span class="neon-bluegreen">鸿蒙玲珑核（鸿蒙量化模型运维）</span>
            </el-menu-item>
            <!-- Temporarily hide realtime dashboard menu so user can navigate elsewhere -->
            <el-menu-item v-if="false" class="header-item" index="/realtime"><span class="neon-bluegreen">系统实时监测</span></el-menu-item>
            <el-menu-item class="header-item" index="/prediction"><span class="neon-bluegreen">量化模型预测指标</span></el-menu-item>
            <el-menu-item class="header-item" index="/check"><span class="neon-bluegreen">量化模型异常检测</span></el-menu-item>
            <el-menu-item class="header-item" index="/overview/configs"><span class="neon-bluegreen">量化模型监控配置</span></el-menu-item>
            <el-menu-item class="header-item" index="/nft-report"><span class="neon-bluegreen">量化报告</span></el-menu-item>
            <el-menu-item class="header-item" index="/nft-market"><span class="neon-bluegreen">模型库</span></el-menu-item>
            <el-menu-item class="header-item" index="/metaverse-hub"><span class="neon-bluegreen">鸿蒙智能中心</span></el-menu-item>
            <el-menu-item class="header-item img-item">
              <img src="../assets/HUST.png"/>
            </el-menu-item>
          </el-menu>
        </div>
      </el-header>
      <el-main>
        <router-view></router-view>
      </el-main>
  </el-container>
  </div>
</template>

<script>
export default {
  name: 'HelloWorld',
  methods:{
    beginPre(){
      this.$message({
        message: '等待24秒将会出现预测结果',
        type: 'warning',
        duration:2000,
        showClose:true
      });
    },
    handleOpen() {},
    handleClose() {}
  },
  computed: {
    activeMenu() {
      const route = this.$route
      const { meta, path } = route
      // 优先使用meta中定义的firstMenu，否则使用当前路径
      return meta.firstMenu || path
    },
    showHeader() {
      // 始终显示 header，方便切换到其他页面（包括实时监测主页面）
      return true
    }
  }
}
</script>

<style scoped>
/* Header and layout styles for main layout */
#system-title {
  padding-top: 20px;
}
.el-container {
  padding: 0;
  margin: 0;
  height: 100%;
  background: transparent !important;
  box-shadow: none !important;
}
.el-header {
  background: transparent !important;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: none !important;
}
.header-inner {
  width: 100%;
  margin: 0 auto;
  /* 优化导航按钮嵌入感和紧凑度 */
  .glitch-form-wrapper {
    --bg-color: #0d0d0d;
    --primary-color: #00f2ea;
    --secondary-color: #a855f7;
    --text-color: #e5e5e5;
    --font-family: "Fira Code", Consolas, "Courier New", Courier, monospace;
    --glitch-anim-duration: 0.5s;
    display: flex;
    align-items: center;
    height: 56px;
    margin: 0 4px;
  }
  .glitch-card {
    background-color: var(--bg-color);
    width: 170px;
    min-width: 120px;
    max-width: 200px;
    height: 48px;
    display: flex;
    align-items: center;
    border: 1px solid rgba(0, 242, 234, 0.2);
    box-shadow: 0 0 12px rgba(0, 242, 234, 0.08), inset 0 0 6px rgba(0, 0, 0, 0.4);
    overflow: hidden;
    margin: 0;
    padding: 0 10px;
  }
  .logo-title .glitch-form-wrapper {
    height: 64px;
    margin-right: 16px;
  }
  .logo-title .glitch-card {
    width: 220px;
    height: 56px;
    font-size: 1.2rem;
    padding-left: 18px;
  }
  border-radius: 10px;
  padding: 0 20px;
  margin-top: 10px;
}
.header-item {
  color: #eeeeee;
  font-size: 2.2vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 8px 12px;
  white-space: nowrap;
  /* 让按钮内容垂直居中 */
  .card-title span {
    display: flex;
    align-items: center;
    font-size: 1rem;
    letter-spacing: 0.04em;
  }
  .header-menu {
    gap: 0;
  }
  .header-item {
    padding: 0 !important;
    margin: 0 !important;
    background: transparent !important;
    border: none !important;
    box-shadow: none !important;
  }
}
/* 顶部菜单项深绿色字体 */
/* 顶部菜单项荧光蓝绿色字体 */
.header-item {
  color: #00fff0 !important;
  font-size: 2.2vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 8px 12px;
  background: transparent !important;
  border: none !important;
  box-shadow: none !important;
  white-space: nowrap;
}
.header-item:hover {
  color: #00ffe0 !important;
}
.logo-title {
  font-size: 4vh;
  flex: 0 0 auto;
  justify-content: flex-start;
  font-weight: 700;
  color: #00fff0 !important;
  white-space: nowrap;
}
.header-menu {
  display: flex;
  flex-wrap: nowrap;
  overflow-x: auto;
  scrollbar-width: none;
}
.header-menu::-webkit-scrollbar {
  display: none;
}
.neon-bluegreen {
  color: #00fff0 !important;
}
.img-item {
  flex: 0 0 auto;
  padding: 0;
  background: transparent;
  display: block;
  margin: 0;
}
/* 激活菜单项样式 */
.el-menu-item.is-active {
  border-bottom: 4px solid #409EFF;
  color: #409EFF;
  background: transparent;
}

/* Glitch logo/button styles (from Uiverse snippet) */
.glitch-form-wrapper {
  --bg-color: #0d0d0d;
  --primary-color: #00f2ea;
  --secondary-color: #a855f7;
  --text-color: #e5e5e5;
  --font-family: "Fira Code", Consolas, "Courier New", Courier, monospace;
  --glitch-anim-duration: 0.5s;
  display: flex;
  align-items: center;
}
.glitch-card {
  background-color: var(--bg-color);
  width: 280px;
  border: 1px solid rgba(0, 242, 234, 0.2);
  box-shadow: 0 0 20px rgba(0, 242, 234, 0.1), inset 0 0 10px rgba(0, 0, 0, 0.5);
  overflow: hidden;
}
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background-color: rgba(0, 0, 0, 0.3);
  padding: 0.2em 0.8em;
  border-bottom: 1px solid rgba(0, 242, 234, 0.2);
}
.card-title { color: var(--primary-color); font-weight:700; display:flex; align-items:center; gap:0.4em; }
.card-title svg { width:18px; height:18px; stroke: var(--primary-color); }
.card-dots span { display:inline-block; width:6px; height:6px; border-radius:50%; background:#333; margin-left:4px; }
</style>