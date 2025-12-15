<template>
  <div class="el-container">
    <div class="el-aside">
      <!-- 左侧导航栏保持不变 -->
    </div>
    <div class="el-main">
      <!-- 顶部主趋势图 -->
      <div class="TopBox">
        <div id="graph1" style="width: 100%; height: 100%;"></div>
      </div>

      <!-- 底部三个辅助图表 -->
      <div class="BottomBox">
        <div class="BottomBox_item">
          <div id="graph2" style="width: 100%; height: 100%;"></div>
        </div>
        <div class="BottomBox_item">
          <div id="graph3" style="width: 100%; height: 100%;"></div>
        </div>
        <div class="BottomBox_item">
          <div id="graph4" style="width: 100%; height: 100%;"></div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import Vue from "vue";
export default {
  name: 'ethernet',
  data() {
    return {
      netData: {
        recv_rate: [],
        send_rate: [],
        packets: {
          total: 0,
          normal: 0,
          error: 0,
          lost: 0
        },
        loss_rate: []
      },
      timer: null
    }
  },
  methods: {
    // 顶部：以太网传输速率趋势图
    myEcharts(mainChart) {
      // 生成30秒趋势数据
      const xData = Array.from({length: 30}, (_, i) => `${30-i}秒前`);
      xData.push('现在');
      
      // 模拟实时速率数据
      const recvData = Array.from({length: 31}, () => Math.floor(Math.random() * 60 + 10));
      const sendData = Array.from({length: 31}, () => Math.floor(Math.random() * 30 + 5));
      
      const option = {
        animation: false,
        grid: { x: 40, y: 50, x2: 40, y2: 30 },
        title: {
          top: '3%',
          left: 'center',
          text: '以太网传输速率',
          textStyle: { color: '#FFF', fontSize: 16 }
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: { type: 'shadow' },
          formatter: params => {
            return `${params[0].name}<br/>
                    接收速率: ${params[0].value}KB/s<br/>
                    发送速率: ${params[1].value}KB/s`;
          }
        },
        legend: {
          right: 30,
          top: 30,
          textStyle: { color: '#FFF' },
          data: ['接收速率', '发送速率']
        },
        xAxis: {
          type: 'category',
          boundaryGap: false,
          data: xData,
          axisLine: { lineStyle: { color: '#444' } },
          axisLabel: {
            textStyle: { color: '#aaa' },
            interval: 5
          }
        },
        yAxis: {
          type: 'value',
          max: 80,
          axisLine: { lineStyle: { color: '#444' } },
          splitLine: { lineStyle: { color: '#333' } },
          axisLabel: { textStyle: { color: '#aaa' }, formatter: '{value}KB/s' }
        },
        series: [
          {
            name: '接收速率',
            type: 'line',
            showSymbol: false,
            data: recvData,
            color: '#409EFF',
            areaStyle: { color: 'rgba(64, 152, 255, 0.1)' }
          },
          {
            name: '发送速率',
            type: 'line',
            showSymbol: false,
            data: sendData,
            color: '#FFC107',
            lineStyle: { type: 'dashed' },
            areaStyle: { color: 'rgba(255, 193, 7, 0.05)' }
          }
        ]
      };
      mainChart.setOption(option);
      return { recvData, sendData };
    },
    
    // 底部左侧：速率对比柱状图
    myGraph2(chart) {
      const option = {
        grid: { x: 30, y: 40, x2: 10, y2: 20 },
        title: {
          top: '5%',
          left: 'center',
          text: '实时速率对比',
          textStyle: { color: '#FFF', fontSize: 14 }
        },
        tooltip: { trigger: 'axis', formatter: '{b}: {c}KB/s' },
        xAxis: {
          type: 'category',
          data: ['接收速率', '发送速率'],
          axisLine: { lineStyle: { color: '#444' } },
          axisLabel: { textStyle: { color: '#aaa' } }
        },
        yAxis: {
          type: 'value',
          axisLine: { lineStyle: { color: '#444' } },
          splitLine: { lineStyle: { color: '#333' } },
          axisLabel: { textStyle: { color: '#aaa' }, formatter: '{value}KB/s' }
        },
        series: [{
          type: 'bar',
          data: [
            this.netData.recv_rate[this.netData.recv_rate.length-1] || 0,
            this.netData.send_rate[this.netData.send_rate.length-1] || 0
          ],
          itemStyle: {
            color: function(params) {
              const colorList = ['#409EFF', '#FFC107'];
              return colorList[params.dataIndex];
            }
          },
          barWidth: 40
        }]
      };
      chart.setOption(option);
    },
    
    // 底部中间：丢包率趋势图
    myGraph3(chart) {
      const xData = Array.from({length: 12}, (_, i) => `${i*5}秒前`);
      const lossData = Array.from({length: 12}, () => parseFloat((Math.random() * 0.5).toFixed(2)));
      
      const option = {
        grid: { x: 30, y: 40, x2: 10, y2: 20 },
        title: {
          top: '5%',
          left: 'center',
          text: '丢包率趋势',
          textStyle: { color: '#FFF', fontSize: 14 }
        },
        tooltip: { trigger: 'axis', formatter: '{b}: {c}%' },
        xAxis: {
          type: 'category',
          data: xData,
          axisLine: { lineStyle: { color: '#444' } },
          axisLabel: { textStyle: { color: '#aaa', fontSize: 10 } }
        },
        yAxis: {
          type: 'value',
          max: 1,
          axisLine: { lineStyle: { color: '#444' } },
          splitLine: { lineStyle: { color: '#333' } },
          axisLabel: { textStyle: { color: '#aaa' }, formatter: '{value}%' }
        },
        series: [{
          type: 'line',
          showSymbol: true,
          symbol: 'circle',
          symbolSize: 6,
          data: lossData,
          color: '#FF5252',
          lineStyle: { width: 2 }
        }]
      };
      chart.setOption(option);
    },
    
    // 底部右侧：包类型分布饼图
    myGraph4(chart) {
      const packetData = [
        { value: this.netData.packets.normal || 85, name: '正常包' },
        { value: this.netData.packets.error || 5, name: '错误包' },
        { value: this.netData.packets.lost || 10, name: '丢失包' }
      ];
      
      const option = {
        grid: { x: 20, y: 30, x2: 20, y2: 20 },
        title: {
          top: '5%',
          left: 'center',
          text: '包类型分布',
          textStyle: { color: '#FFF', fontSize: 14 }
        },
        tooltip: { trigger: 'item', formatter: '{b}: {c} ({d}%)' },
        legend: {
          orient: 'vertical',
          right: 10,
          top: '40%',
          textStyle: { color: '#aaa', fontSize: 12 },
          data: ['正常包', '错误包', '丢失包']
        },
        series: [{
          type: 'pie',
          radius: ['40%', '70%'],
          center: ['35%', '55%'],
          avoidLabelOverlap: false,
          itemStyle: {
            borderRadius: 8,
            borderColor: '#222',
            borderWidth: 2
          },
          label: { show: false },
          emphasis: {
            label: { show: true, fontSize: 16 }
          },
          labelLine: { show: false },
          data: packetData,
          color: ['#4CAF50', '#FF9800', '#F44336']
        }]
      };
      chart.setOption(option);
    },
    
    initCharts() {
      // 初始化所有图表
      const mainChart = this.$echarts.init(document.getElementById('graph1'));
      const chart2 = this.$echarts.init(document.getElementById('graph2'));
      const chart3 = this.$echarts.init(document.getElementById('graph3'));
      const chart4 = this.$echarts.init(document.getElementById('graph4'));
      
      // 渲染图表
      const rates = this.myEcharts(mainChart);
      this.netData.recv_rate = rates.recvData;
      this.netData.send_rate = rates.sendData;
      this.myGraph2(chart2);
      this.myGraph3(chart3);
      this.myGraph4(chart4);
      
      // 窗口大小调整
      window.addEventListener('resize', () => {
        mainChart.resize();
        chart2.resize();
        chart3.resize();
        chart4.resize();
      });
      
      // 定时更新
      this.timer = setInterval(() => this.initCharts(), 5000);
    }
  },
  mounted() {
    this.initCharts();
  },
  beforeDestroy() {
    clearInterval(this.timer);
  }
}
</script>

<style scoped>
.el-container {
  padding: 0;
  margin: 0;
  height: calc(100vh - 142px);
  background: #1a1a1a;
}
.el-main {
  padding: 15px;
  height: 100%;
}
.TopBox {
  width: 100%;
  height: 55%;
  margin-bottom: 15px;
  border-radius: 10px;
  background: #222;
  box-shadow: 0 3px 10px rgba(0,0,0,0.3);
}
.BottomBox {
  display: flex;
  gap: 15px;
  height: calc(45% - 15px);
}
.BottomBox_item {
  flex: 1;
  border-radius: 10px;
  background: #222;
  box-shadow: 0 3px 10px rgba(0,0,0,0.3);
}
</style>