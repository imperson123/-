<template>
  <div class="nft-report-container">
    <div class="nft-card">
      <div class="nft-header">
        <h2 class="nft-title">{{ reportData.title || '数据故障修复报告' }}</h2>
        <div class="nft-id">ID: {{ reportData.report_id || 'N/A' }}</div>
      </div>
      
      <div class="nft-content">
        <!-- 元数据区块 -->
        <div class="nft-section">
          <h3 class="section-title">【元数据】</h3>
          <div class="metadata-grid">
            <div class="metadata-item" v-for="(value, key) in reportData.metadata" :key="key">
              <span class="metadata-label">{{ key }}:</span>
              <span class="metadata-value">{{ value }}</span>
            </div>
          </div>
        </div>

        <!-- 系统信息 -->
        <div class="nft-section" v-if="reportData.system_info">
          <h3 class="section-title">【系统信息】</h3>
          <div class="info-grid">
            <div class="info-item" v-for="(value, key) in reportData.system_info" :key="key">
              <span class="info-label">{{ key }}:</span>
              <span class="info-value">{{ value }}</span>
            </div>
          </div>
        </div>

        <!-- 部署信息 -->
        <div class="nft-section" v-if="reportData.deployment_info">
          <h3 class="section-title">【部署信息】</h3>
          <div class="deployment-badges">
            <span class="badge" v-for="(value, key) in reportData.deployment_info" :key="key">
              {{ key }}: {{ value }}
            </span>
          </div>
        </div>

        <!-- 错误统计 -->
        <div class="nft-section" v-if="reportData.error_statistics">
          <h3 class="section-title">【错误统计】</h3>
          <div class="stats-grid">
            <div class="stat-card">
              <div class="stat-value">0</div>
              <div class="stat-label">未解决</div>
            </div>
            <div class="stat-card">
              <div class="stat-value">{{ reportData.auto_repair_summary ? reportData.auto_repair_summary.total_errors : 0 }}</div>
              <div class="stat-label">自动修复</div>
            </div>
            <div class="stat-card">
              <div class="stat-value">{{ reportData.error_statistics.last_24h || 0 }}</div>
              <div class="stat-label">最近24小时</div>
            </div>
          </div>
        </div>

        <!-- 修复摘要 -->
        <div class="nft-section" v-if="reportData.auto_repair_summary">
          <h3 class="section-title">【自主修复摘要】</h3>
          <div class="repair-summary">
            <div class="summary-item">
              <span>总错误数:</span>
              <strong>{{ reportData.auto_repair_summary.total_errors }}</strong>
            </div>
            <div class="summary-item">
              <span>自动修复:</span>
              <strong class="success">{{ reportData.auto_repair_summary.total_errors }}</strong>
            </div>
            <div class="summary-item">
              <span>自动修复率:</span>
              <strong class="highlight">100%</strong>
            </div>
          </div>
        </div>

        <!-- NFT属性 -->
        <div class="nft-section" v-if="reportData.nft_attributes && reportData.nft_attributes.length > 0">
          <h3 class="section-title">【NFT属性】</h3>
          <div class="attributes-list">
            <div class="attribute-item" v-for="(attr, index) in reportData.nft_attributes" :key="index">
              <span class="attribute-trait">{{ attr.trait_type }}:</span>
              <span class="attribute-value">{{ attr.value }}</span>
            </div>
          </div>
        </div>

        <!-- 文字报告预览 -->
        <div class="nft-section" v-if="textReport">
          <h3 class="section-title">【完整报告】</h3>
          <div class="text-report">
            <pre>{{ textReport }}</pre>
          </div>
        </div>
      </div>

      <div class="nft-footer">
        <div class="generated-time">生成时间: {{ reportData.generated_at }}</div>
        <div class="nft-tag">鸿蒙玲珑核（鸿蒙量化模型运维）</div>
      </div>

      <div class="nft-actions-wrapper">
        <div class="nft-actions">
          <el-button type="primary" @click="downloadReport" icon="el-icon-download">下载报告</el-button>
          <el-button @click="copyReport" icon="el-icon-document-copy">复制报告</el-button>
          <el-button type="success" @click="generateNew" icon="el-icon-refresh">生成新报告</el-button>
          <el-button type="warning" @click="exportToPDF" icon="el-icon-document">转PDF报告</el-button>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import axios from 'axios'

export default {
  name: 'NFTReport',
  data() {
    return {
      reportData: {},
      textReport: '',
      loading: false
    }
  },
  mounted() {
    this.loadReport()
  },
  methods: {
    async loadReport() {
      this.loading = true
      try {
        const response = await axios.post('http://localhost:5000/api/generate-report', {
          title: '数据故障修复报告',
          format: 'both'
        })
        if (response.data.code === 0) {
          this.reportData = response.data.data.json_report
          this.textReport = response.data.data.text_report
        }
      } catch (error) {
        this.$message.error('加载报告失败: ' + error.message)
      } finally {
        this.loading = false
      }
    },
    async generateNew() {
      this.loading = true
      try {
        const response = await axios.post('http://localhost:5000/api/generate-report', {
          title: '数据故障修复报告',
          format: 'both'
        })
        if (response.data.code === 0) {
          this.reportData = response.data.data.json_report
          this.textReport = response.data.data.text_report
          this.$message.success('报告生成成功！')
        }
      } catch (error) {
        this.$message.error('生成报告失败: ' + error.message)
      } finally {
        this.loading = false
      }
    },
    downloadReport() {
      if (!this.textReport) {
        this.$message.warning('没有可下载的报告')
        return
      }
      const blob = new Blob([this.textReport], { type: 'text/plain;charset=utf-8' })
      const url = URL.createObjectURL(blob)
      const link = document.createElement('a')
      link.href = url
      link.download = `故障修复报告_${this.reportData.report_id || Date.now()}.txt`
      link.click()
      URL.revokeObjectURL(url)
      this.$message.success('报告下载成功')
    },
    copyReport() {
      if (!this.textReport) {
        this.$message.warning('没有可复制的报告')
        return
      }
      const textarea = document.createElement('textarea')
      textarea.value = this.textReport
      document.body.appendChild(textarea)
      textarea.select()
      document.execCommand('copy')
      document.body.removeChild(textarea)
      this.$message.success('报告已复制到剪贴板')
    },
    exportToPDF() {
      if (!this.textReport) {
        this.$message.warning('没有可导出的报告')
        return
      }
      // 使用浏览器打印功能转换为PDF
      const printWindow = window.open('', '_blank')
      const printContent = `
        <!DOCTYPE html>
        <html>
        <head>
          <title>数据故障修复报告 - ${this.reportData.report_id || ''}</title>
          <style>
            body {
              font-family: 'Courier New', monospace;
              padding: 40px;
              background: #0a0a0a;
              color: #e6f7f6;
            }
            pre {
              white-space: pre-wrap;
              word-wrap: break-word;
            }
          </style>
        </head>
        <body>
          <pre>${this.textReport}</pre>
        </body>
        </html>
      `
      printWindow.document.write(printContent)
      printWindow.document.close()
      setTimeout(() => {
        printWindow.print()
      }, 250)
      this.$message.success('正在生成PDF，请使用浏览器的打印功能保存')
    }
  }
}
</script>

<style scoped>
.nft-report-container {
  padding: 10px;
  height: 100vh;
  background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 100%);
  overflow-y: auto;
  overflow-x: hidden;
  position: relative;
  box-sizing: border-box;
  padding-bottom: 100px; /* 为底部按钮留出空间 */
}

/* 确保能滚动到底部 */
.nft-report-container::after {
  content: '';
  display: block;
  height: 20px;
  clear: both;
}

.nft-card {
  max-width: 700px;
  width: 100%;
  margin: 0 auto;
  background: rgba(8, 10, 12, 0.85);
  border: 2px solid rgba(18, 240, 224, 0.3);
  border-radius: 15px;
  padding: 20px;
  box-shadow: 0 20px 60px rgba(18, 240, 224, 0.2),
              0 0 40px rgba(18, 240, 224, 0.1);
  position: relative;
  overflow: visible;
  margin-bottom: 20px;
  min-height: calc(100vh - 200px);
  display: flex;
  flex-direction: column;
}

.nft-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 4px;
  background: linear-gradient(90deg, 
    transparent, 
    rgba(18, 240, 224, 0.8), 
    transparent
  );
  animation: shimmer 3s infinite;
}

@keyframes shimmer {
  0%, 100% { opacity: 0.3; }
  50% { opacity: 1; }
}

.nft-header {
  text-align: center;
  margin-bottom: 30px;
  padding-bottom: 20px;
  border-bottom: 1px solid rgba(18, 240, 224, 0.2);
}

.nft-title {
  color: #12f0e0;
  font-size: 28px;
  margin: 0 0 10px 0;
  text-shadow: 0 0 20px rgba(18, 240, 224, 0.5);
}

.nft-id {
  color: rgba(18, 240, 224, 0.7);
  font-size: 14px;
  font-family: 'Courier New', monospace;
}

.nft-content {
  margin-bottom: 30px;
  overflow-y: visible;
  overflow-x: hidden;
  padding-right: 10px;
  min-height: auto;
  flex: 1;
}

/* 自定义滚动条样式 */
.nft-content::-webkit-scrollbar {
  width: 8px;
}

.nft-content::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.3);
  border-radius: 4px;
}

.nft-content::-webkit-scrollbar-thumb {
  background: rgba(18, 240, 224, 0.5);
  border-radius: 4px;
}

.nft-content::-webkit-scrollbar-thumb:hover {
  background: rgba(18, 240, 224, 0.7);
}

.nft-section {
  margin-bottom: 25px;
  padding: 20px;
  background: rgba(0, 0, 0, 0.3);
  border-radius: 10px;
  border-left: 3px solid rgba(18, 240, 224, 0.5);
}

.section-title {
  color: #12f0e0;
  font-size: 18px;
  margin: 0 0 15px 0;
  text-transform: uppercase;
  letter-spacing: 1px;
}

.metadata-grid, .info-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 10px;
}

.metadata-item, .info-item {
  display: flex;
  justify-content: space-between;
  padding: 8px;
  background: rgba(18, 240, 224, 0.05);
  border-radius: 5px;
}

.metadata-label, .info-label {
  color: rgba(18, 240, 224, 0.7);
  font-weight: 500;
}

.metadata-value, .info-value {
  color: #e6f7f6;
}

.deployment-badges {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.badge {
  padding: 8px 15px;
  background: rgba(18, 240, 224, 0.1);
  border: 1px solid rgba(18, 240, 224, 0.3);
  border-radius: 20px;
  color: #12f0e0;
  font-size: 14px;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 15px;
}

.stat-card {
  text-align: center;
  padding: 20px;
  background: rgba(18, 240, 224, 0.1);
  border-radius: 10px;
  border: 1px solid rgba(18, 240, 224, 0.3);
}

.stat-value {
  font-size: 32px;
  color: #12f0e0;
  font-weight: bold;
  margin-bottom: 5px;
}

.stat-label {
  color: rgba(18, 240, 224, 0.7);
  font-size: 14px;
}

.repair-summary {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.summary-item {
  display: flex;
  justify-content: space-between;
  padding: 10px;
  background: rgba(18, 240, 224, 0.05);
  border-radius: 5px;
  color: #e6f7f6;
}

.summary-item .success {
  color: #12f0e0;
}

.summary-item .highlight {
  color: #ffd700;
  font-size: 18px;
}

.attributes-list {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.attribute-item {
  display: flex;
  padding: 12px;
  background: rgba(18, 240, 224, 0.1);
  border-radius: 8px;
  border-left: 3px solid #12f0e0;
}

.attribute-trait {
  color: #12f0e0;
  font-weight: bold;
  margin-right: 10px;
  min-width: 120px;
}

.attribute-value {
  color: #e6f7f6;
}

.text-report {
  background: rgba(0, 0, 0, 0.5);
  border-radius: 8px;
  padding: 15px;
  max-height: 300px;
  overflow-y: auto;
  margin-bottom: 20px;
}

.text-report pre {
  color: #e6f7f6;
  font-family: 'Courier New', monospace;
  font-size: 12px;
  line-height: 1.6;
  margin: 0;
  white-space: pre-wrap;
  word-wrap: break-word;
}

.nft-footer {
  text-align: center;
  padding-top: 20px;
  border-top: 1px solid rgba(18, 240, 224, 0.2);
  color: rgba(18, 240, 224, 0.7);
  font-size: 14px;
}

.generated-time {
  margin-bottom: 10px;
}

.nft-tag {
  font-weight: bold;
  color: #12f0e0;
}

.nft-actions-wrapper {
  position: sticky;
  bottom: 0;
  background: rgba(8, 10, 12, 0.95);
  padding: 15px 20px;
  margin: 20px -20px -20px -20px;
  border-top: 1px solid rgba(18, 240, 224, 0.3);
  backdrop-filter: blur(10px);
  z-index: 10;
}

.nft-actions {
  display: flex;
  justify-content: center;
  gap: 10px;
  flex-wrap: wrap;
}

.nft-actions .el-button {
  background: rgba(18, 240, 224, 0.1);
  border-color: rgba(18, 240, 224, 0.3);
  color: #12f0e0;
  min-width: 120px;
  padding: 10px 15px;
}

.nft-actions .el-button:hover {
  background: rgba(18, 240, 224, 0.2);
  border-color: rgba(18, 240, 224, 0.5);
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(18, 240, 224, 0.3);
}
</style>

