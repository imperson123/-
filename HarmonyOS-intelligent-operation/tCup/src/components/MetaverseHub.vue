<template>
  <metaverse-scene>
    <div class="el-container metaverse-overlay">
      <div class="el-main hub-content-wrapper">
        <!-- 左侧功能卡片区域 -->
        <div class="left-features">
          <h2 class="section-title">功能中心</h2>
          <div class="features-grid">
            <div class="feature-card metaverse-card" @click="goToFeature('deployment')">
              <div class="feature-icon">
                <i class="el-icon-upload2"></i>
              </div>
              <h3>轻量化部署</h3>
              <p>支持多操作系统快速部署</p>
            </div>
            <div class="feature-card metaverse-card" @click="goToFeature('auto-ops')">
              <div class="feature-icon">
                <i class="el-icon-setting"></i>
              </div>
              <h3>自主运维</h3>
              <p>自动检测和修复系统问题</p>
            </div>
            <div class="feature-card metaverse-card" @click="goToFeature('error-logs')">
              <div class="feature-icon">
                <i class="el-icon-document"></i>
              </div>
              <h3>错误日志</h3>
              <p>实时收集和监控系统错误</p>
            </div>
            <div class="feature-card metaverse-card" @click="goToNFTReport">
              <div class="feature-icon">
                <i class="el-icon-star-on"></i>
              </div>
              <h3>量化报告</h3>
              <p>生成数据故障修复量化报告</p>
            </div>
          </div>
        </div>

        <!-- 右侧社区对话区域 -->
        <div class="right-community">
          <user-chat></user-chat>
        </div>
      </div>
    </div>
  </metaverse-scene>
</template>

<script>
import MetaverseScene from '@/components/MetaverseScene.vue'
import UserChat from '@/components/UserChat.vue'

export default {
  name: 'MetaverseHub',
  components: { 
    MetaverseScene,
    UserChat
  },
  methods: {
    goToNFTReport() {
      this.$router.push('/nft-report')
    },
    goToFeature(feature) {
      switch(feature) {
        case 'deployment':
          this.$message.info('轻量化部署功能')
          break
        case 'auto-ops':
          this.$message.info('自主运维功能')
          break
        case 'error-logs':
          this.$message.info('错误日志功能')
          break
      }
    }
  }
}
</script>

<style scoped>
.metaverse-overlay {
  position: relative;
  z-index: 10;
  pointer-events: auto;
  height: 100%;
}

.hub-content-wrapper {
  display: flex;
  gap: 20px;
  height: 100%;
  padding: 20px;
  box-sizing: border-box;
}

.left-features {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
}

.section-title {
  color: #12f0e0;
  font-size: 24px;
  margin: 0 0 20px 0;
  text-shadow: 0 0 15px rgba(18, 240, 224, 0.5);
  padding: 10px 0;
  border-bottom: 2px solid rgba(18, 240, 224, 0.3);
}

.features-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 20px;
  flex: 1;
  overflow-y: auto;
  padding: 10px;
}

/* 自定义滚动条 */
.features-grid::-webkit-scrollbar {
  width: 8px;
}

.features-grid::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.3);
  border-radius: 4px;
}

.features-grid::-webkit-scrollbar-thumb {
  background: rgba(18, 240, 224, 0.5);
  border-radius: 4px;
}

.feature-card.metaverse-card {
  background: rgba(8, 10, 12, 0.8);
  border: 2px solid rgba(18, 240, 224, 0.3);
  border-radius: 15px;
  padding: 30px;
  text-align: center;
  cursor: pointer;
  transition: all 0.3s ease;
  backdrop-filter: blur(10px);
  box-shadow: 0 10px 30px rgba(18, 240, 224, 0.1);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}

.feature-card.metaverse-card:hover {
  transform: translateY(-8px) scale(1.03);
  border-color: rgba(18, 240, 224, 0.6);
  box-shadow: 0 20px 50px rgba(18, 240, 224, 0.4);
  background: rgba(8, 10, 12, 0.95);
}

.feature-icon {
  width: 60px;
  height: 60px;
  border-radius: 50%;
  background: rgba(18, 240, 224, 0.2);
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 15px;
  border: 2px solid rgba(18, 240, 224, 0.4);
}

.feature-icon i {
  font-size: 28px;
  color: #12f0e0;
}

.feature-card.metaverse-card h3 {
  color: #12f0e0;
  font-size: 20px;
  margin: 0 0 10px 0;
  text-shadow: 0 0 10px rgba(18, 240, 224, 0.5);
}

.feature-card.metaverse-card p {
  color: rgba(18, 240, 224, 0.8);
  font-size: 14px;
  margin: 0;
  line-height: 1.5;
}

.right-community {
  width: 400px;
  flex-shrink: 0;
  height: 100%;
}
</style>

