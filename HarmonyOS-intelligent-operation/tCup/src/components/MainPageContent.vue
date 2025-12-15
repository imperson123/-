<template>
  <div class="main-page-content">
    <!-- 顶部系统指标 -->
    <div class="top-indicators">
      <!-- 原有指标卡片代码保持不变 -->
    </div>

    <!-- 问答组件 - 移至中间位置 -->
    <div class="qa-container">
      <qa-component></qa-component>
    </div>

    <!-- 底部功能卡片 - 可滚动 -->
    <div class="intro-cards-wrapper">
      <div class="intro-cards" ref="cardsContainer">
        <div class="intro-card metaverse-card" @click="goToFeature('deployment')">
          <h3>轻量化部署</h3>
          <p>支持多操作系统快速部署</p>
        </div>
        <div class="intro-card metaverse-card" @click="goToFeature('auto-ops')">
          <h3>自主运维</h3>
          <p>自动检测和修复系统问题</p>
        </div>
        <div class="intro-card metaverse-card" @click="goToFeature('error-logs')">
          <h3>错误日志</h3>
          <p>实时收集和监控系统错误</p>
        </div>
        <div class="intro-card metaverse-card" @click="goToNFTMarket">
          <h3>模型库</h3>
          <p>购买数据故障修复NFT方案</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import QaComponent from './result/realTime/QaComponent.vue'

export default {
  name: 'MainPageContent',
  components: { 
    QaComponent
  },
  methods: {
    goToFeature(feature) {
      // 根据功能跳转
      switch(feature) {
        case 'deployment':
          this.$message.info('轻量化部署功能')
          break
        case 'auto-ops':
          this.$message.info('自主运维功能')
          break
        case 'error-logs':
          this.$router.push('/error-logs')
          break
      }
    },
    goToNFTMarket() {
      this.$router.push('/nft-market')
    }
  }
}
</script>

<style scoped>
.main-page-content {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  padding: 20px;
}

.top-indicators {
  padding: 20px;
  flex-shrink: 0;
}

.qa-container {
  padding: 20px;
  margin-bottom: 20px;
  background: rgba(8, 10, 12, 0.7);
  border-radius: 10px;
  border: 1px solid rgba(18, 240, 224, 0.3);
  backdrop-filter: blur(10px);
  box-shadow: 0 10px 30px rgba(18, 240, 224, 0.1);
  flex-shrink: 0;
}

.intro-cards-wrapper {
  flex: 1;
  overflow-y: auto;
  overflow-x: hidden;
  padding: 10px;
}

/* 自定义滚动条 */
.intro-cards-wrapper::-webkit-scrollbar {
  width: 8px;
}

.intro-cards-wrapper::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.3);
  border-radius: 4px;
}

.intro-cards-wrapper::-webkit-scrollbar-thumb {
  background: rgba(18, 240, 224, 0.5);
  border-radius: 4px;
}

.intro-cards-wrapper::-webkit-scrollbar-thumb:hover {
  background: rgba(18, 240, 224, 0.7);
}

.intro-cards {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 20px;
  padding: 20px;
}

.intro-card.metaverse-card {
  background: rgba(8, 10, 12, 0.8);
  border: 2px solid rgba(18, 240, 224, 0.3);
  border-radius: 15px;
  padding: 25px;
  text-align: center;
  cursor: pointer;
  transition: all 0.3s ease;
  backdrop-filter: blur(10px);
  box-shadow: 0 10px 30px rgba(18, 240, 224, 0.1);
}

.intro-card.metaverse-card:hover {
  transform: translateY(-5px);
  border-color: rgba(18, 240, 224, 0.6);
  box-shadow: 0 15px 40px rgba(18, 240, 224, 0.3);
  background: rgba(8, 10, 12, 0.9);
}

.intro-card.metaverse-card h3 {
  color: #12f0e0;
  font-size: 20px;
  margin: 0 0 10px 0;
  text-shadow: 0 0 10px rgba(18, 240, 224, 0.5);
}

.intro-card.metaverse-card p {
  color: rgba(18, 240, 224, 0.8);
  font-size: 14px;
  margin: 0;
}
</style>

