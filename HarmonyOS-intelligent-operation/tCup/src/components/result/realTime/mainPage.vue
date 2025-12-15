<template>
  <metaverse-scene>
    <div class="el-container metaverse-overlay">
      <div class="el-main main-content">
        <!-- 实时数据图表区域 -->
        <div class="charts-container">
          <!-- 顶部：CPU和内存趋势图 -->
          <div class="top-charts">
            <div class="chart-box">
              <div id="cpuChart" style="width: 100%; height: 100%;"></div>
            </div>
            <div class="chart-box">
              <div id="memoryChart" style="width: 100%; height: 100%;"></div>
            </div>
          </div>

          <!-- 底部：磁盘和网络图表 -->
          <div class="bottom-charts">
            <div class="chart-box">
              <div id="diskChart" style="width: 100%; height: 100%;"></div>
            </div>
            <div class="chart-box">
              <div id="networkChart" style="width: 100%; height: 100%;"></div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </metaverse-scene>
</template>

<script>
import MetaverseScene from '@/components/MetaverseScene.vue'

export default {
  name: 'mainPage',
  components: { 
    MetaverseScene
  },
  data() {
    return {
      cpuData: [],
      memoryData: [],
      diskData: [],
      networkData: { recv: [], send: [] },
      timers: []
    }
  },
  methods: {
    initCPUChart() {
      const chart = this.$echarts.init(document.getElementById('cpuChart'))
      const numSlice = 30
      const xid = []
      for (let i = 0; i < numSlice; i++) {
        xid.push(((numSlice - i)) + '秒前')
      }
      xid[numSlice - 1] = '现在'

      const option = {
        animation: false,
        grid: { x: 40, y: 50, x2: 40, y2: 30 },
        title: {
          top: '3%',
          left: 'center',
          text: '模型推理耗时 (ms)',
          textStyle: { color: '#12f0e0' }
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: { animation: false }
        },
        xAxis: [{
          type: 'category',
          boundaryGap: false,
          data: xid,
          axisLine: { lineStyle: { color: '#12f0e0' } },
          axisLabel: { textStyle: { color: '#e6f7f6' }, interval: Math.max(Math.floor(xid.length/5) - 1, 0) }
        }],
        yAxis: [{
          type: 'value',
          axisLine: { lineStyle: { color: '#12f0e0' } },
          axisLabel: { textStyle: { color: '#e6f7f6' }, formatter: '{value} ms' },
        }],
        series: [{
          name: '推理耗时',
          type: 'line',
          showSymbol: false,
          data: this.cpuData,
          color: '#12f0e0',
          areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [
            { offset: 0, color: 'rgba(18, 240, 224, 0.3)' },
            { offset: 1, color: 'rgba(18, 240, 224, 0.05)' }
          ]}}
        }]
      }
      chart.setOption(option)
      return chart
    },
    initMemoryChart() {
      const chart = this.$echarts.init(document.getElementById('memoryChart'))
      const numSlice = 30
      const xid = []
      for (let i = 0; i < numSlice; i++) {
        xid.push(((numSlice - i)) + '秒前')
      }
      xid[numSlice - 1] = '现在'

      const option = {
        animation: false,
        grid: { x: 40, y: 50, x2: 40, y2: 30 },
        title: {
          top: '3%',
          left: 'center',
          text: '模型内存占用 (%)',
          textStyle: { color: '#12f0e0' }
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: { animation: false }
        },
        xAxis: [{
          type: 'category',
          boundaryGap: false,
          data: xid,
          axisLine: { lineStyle: { color: '#12f0e0' } },
          axisLabel: { textStyle: { color: '#e6f7f6' }, interval: Math.max(Math.floor(xid.length/5) - 1, 0) }
        }],
        yAxis: [{
          type: 'value',
          axisLine: { lineStyle: { color: '#12f0e0' } },
          axisLabel: { textStyle: { color: '#e6f7f6' }, formatter: '{value}%' },
          max: 100
        }],
        series: [{
          name: '模型内存占用',
          type: 'line',
          showSymbol: false,
          data: this.memoryData,
          color: '#12f0e0',
          areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [
            { offset: 0, color: 'rgba(18, 240, 224, 0.3)' },
            { offset: 1, color: 'rgba(18, 240, 224, 0.05)' }
          ]}}
        }]
      }
      chart.setOption(option)
      return chart
    },
    initDiskChart() {
      const chart = this.$echarts.init(document.getElementById('diskChart'))
      const numSlice = 30
      const xid = []
      for (let i = 0; i < numSlice; i++) {
        xid.push(((numSlice - i)) + '秒前')
      }
      xid[numSlice - 1] = '现在'

      const option = {
        animation: false,
        grid: { x: 40, y: 50, x2: 40, y2: 30 },
        title: {
          top: '3%',
          left: 'center',
          text: '量化精度损失 (%)',
          textStyle: { color: '#12f0e0' }
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: { animation: false }
        },
        xAxis: [{
          type: 'category',
          boundaryGap: false,
          data: xid,
          axisLine: { lineStyle: { color: '#12f0e0' } },
          axisLabel: { textStyle: { color: '#e6f7f6' }, interval: Math.max(Math.floor(xid.length/5) - 1, 0) }
        }],
        yAxis: [{
          type: 'value',
          axisLine: { lineStyle: { color: '#12f0e0' } },
          axisLabel: { textStyle: { color: '#e6f7f6' }, formatter: '{value}%' },
          max: 100
        }],
        series: [{
          name: '量化精度损失',
          type: 'line',
          showSymbol: false,
          data: this.diskData,
          color: '#12f0e0',
          areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [
            { offset: 0, color: 'rgba(18, 240, 224, 0.3)' },
            { offset: 1, color: 'rgba(18, 240, 224, 0.05)' }
          ]}}
        }]
      }
      chart.setOption(option)
      return chart
    },
    initNetworkChart() {
      const chart = this.$echarts.init(document.getElementById('networkChart'))
      const numSlice = 30
      const xid = []
      for (let i = 0; i < numSlice; i++) {
        xid.push(((numSlice - i)) + '秒前')
      }
      xid[numSlice - 1] = '现在'

      const option = {
        animation: false,
        grid: { x: 40, y: 50, x2: 40, y2: 30 },
        title: {
          top: '3%',
          left: 'center',
          text: '模型吞吐量 (inferences/s)',
          textStyle: { color: '#12f0e0' }
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: { animation: false }
        },
        legend: {
          data: ['接收', '发送'],
          textStyle: { color: '#e6f7f6' },
          top: '10%'
        },
        xAxis: [{
          type: 'category',
          boundaryGap: false,
          data: xid,
          axisLine: { lineStyle: { color: '#12f0e0' } },
          axisLabel: { textStyle: { color: '#e6f7f6' }, interval: Math.max(Math.floor(xid.length/5) - 1, 0) }
        }],
        yAxis: [{
          type: 'value',
          axisLine: { lineStyle: { color: '#12f0e0' } },
          axisLabel: { textStyle: { color: '#e6f7f6' }, formatter: '{value} inferences/s' }
        }],
        series: [
          {
            name: '接收',
            type: 'line',
            showSymbol: false,
            data: this.networkData.recv,
            color: '#12f0e0'
          },
          {
            name: '发送',
            type: 'line',
            showSymbol: false,
            data: this.networkData.send,
            color: '#ff6b6b'
          }
        ]
      }
      chart.setOption(option)
      return chart
    },
    async updateData() {
      try {
        const res = await this.$http.get('/monitor/status')

        // 更新CPU数据 -> 映射为“模型推理耗时 (ms)”（优先使用 cpu_time）
        if (res.cpu_data && res.cpu_data.cpu_time) {
          // 假设后端 cpu_time 单位为秒，转换为毫秒
          const val = parseFloat(res.cpu_data.cpu_time) * 1000
          this.cpuData.push(parseFloat(val.toFixed(2)))
          if (this.cpuData.length > 30) this.cpuData.shift()
        } else if (res.cpu_data && res.cpu_data.cpu_percent) {
          const avg = res.cpu_data.cpu_percent.reduce((a, b) => a + b, 0) / res.cpu_data.cpu_percent.length
          // 回退：使用 cpu 利用率的一个近似值（百分比 -> ms 级别占位）
          this.cpuData.push(parseFloat((avg * 10).toFixed(2)))
          if (this.cpuData.length > 30) this.cpuData.shift()
        }

        // 更新内存数据 -> 映射为“模型内存占用 (%)”
        if (res.memory_data && res.memory_data.basic_info) {
          this.memoryData.push(res.memory_data.basic_info.percent)
          if (this.memoryData.length > 30) this.memoryData.shift()
        }

        // 更新磁盘数据 -> 映射为“量化精度损失 (%)”（临时映射）
        if (res.disk_data && res.disk_data.disk_usage) {
          this.diskData.push(res.disk_data.disk_usage.percent)
          if (this.diskData.length > 30) this.diskData.shift()
        }

        // 更新网络数据 -> 映射为“模型吞吐量 (inferences/s)”（优先使用 model_metrics.throughput）
        if (res.model_metrics && (res.model_metrics.throughput || res.model_metrics.throughput === 0)) {
          const tp = parseFloat(res.model_metrics.throughput)
          // 同步推入接收/发送线用于显示（前端当前绘制两个系列），接收/发送均显示吞吐值
          this.networkData.recv.push(parseFloat(tp.toFixed(2)))
          this.networkData.send.push(parseFloat(tp.toFixed(2)))
          if (this.networkData.recv.length > 30) {
            this.networkData.recv.shift()
            this.networkData.send.shift()
          }
        } else if (res.net_data) {
          // 回退：使用网络字节作为占位，转换为 MB/s
          const recvMB = (res.net_data.bytes_recv || 0) / (1024 * 1024)
          const sendMB = (res.net_data.bytes_sent || 0) / (1024 * 1024)
          this.networkData.recv.push(parseFloat(recvMB.toFixed(2)))
          this.networkData.send.push(parseFloat(sendMB.toFixed(2)))
          if (this.networkData.recv.length > 30) {
            this.networkData.recv.shift()
            this.networkData.send.shift()
          }
        }

        // 更新图表
        this.initCPUChart()
        this.initMemoryChart()
        this.initDiskChart()
        this.initNetworkChart()
      } catch (error) {
        console.error('获取监控数据失败:', error)
      }
    }
  },
  mounted() {
    // 初始化数据
    for (let i = 0; i < 30; i++) {
      this.cpuData.push(0)
      this.memoryData.push(0)
      this.diskData.push(0)
      this.networkData.recv.push(0)
      this.networkData.send.push(0)
    }
    
    // 初始化图表
    this.$nextTick(() => {
      // 先初始化一次图表
      this.initCPUChart()
      this.initMemoryChart()
      this.initDiskChart()
      this.initNetworkChart()
      
      // 立即获取一次数据
      this.updateData()
      
      // 定时更新数据
      const timer = setInterval(() => this.updateData(), 2000)
      this.timers.push(timer)
      
      // 窗口大小调整
      const resizeHandler = () => {
        const cpuChart = this.$echarts.getInstanceByDom(document.getElementById('cpuChart'))
        const memoryChart = this.$echarts.getInstanceByDom(document.getElementById('memoryChart'))
        const diskChart = this.$echarts.getInstanceByDom(document.getElementById('diskChart'))
        const networkChart = this.$echarts.getInstanceByDom(document.getElementById('networkChart'))
        if (cpuChart) cpuChart.resize()
        if (memoryChart) memoryChart.resize()
        if (diskChart) diskChart.resize()
        if (networkChart) networkChart.resize()
      }
      window.addEventListener('resize', resizeHandler)
      this.resizeHandler = resizeHandler
    })
  },
  beforeDestroy() {
    this.timers.forEach(timer => clearInterval(timer))
    if (this.resizeHandler) {
      window.removeEventListener('resize', this.resizeHandler)
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

.main-content {
  height: 100%;
  padding: 15px;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
}

.charts-container {
  display: flex;
  flex-direction: column;
  gap: 15px;
  height: 100%;
}

.top-charts {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
  height: 48%;
}

.bottom-charts {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
  height: 48%;
}

.chart-box {
  background: rgba(8, 10, 12, 0.8);
  border: 1px solid rgba(18, 240, 224, 0.3);
  border-radius: 10px;
  padding: 10px;
  box-shadow: 0 10px 30px rgba(18, 240, 224, 0.1);
  backdrop-filter: blur(10px);
}
</style>
