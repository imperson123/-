<template>
  <div class="el-container">
    <div class="el-aside">
      <!-- 左侧导航栏保持不变 -->
    </div>
    <div class="el-main">
      <!-- 顶部CPU利用率趋势图 -->
      <div class="TopBox">
        <div id="graph1" style="width: 100%; height: 100%;"></div>
      </div>

      <!-- 仅保留左侧CPU核心占用率图表，删除右侧空白卡片 -->
      <div class="BottomBox_left">
        <div id="graph2" style="width: 100%; height: 100%;"></div>
      </div>
    </div>
  </div>
</template>

<script>
import Vue from "vue";
export default {
  name: 'cpu',
  data() {
    return {
      CPU_percent: [],
      timer: null
    }
  },
  methods: {
    myEcharts(myChart_L1, dataset) {
      // 顶部CPU利用率趋势图
      var xid = [];
      var numSlice = 30;
      var lenSlice = 1;
      for (var i = 0; i < numSlice * lenSlice; i++) {
        xid.push(((numSlice * lenSlice - i) / lenSlice).toString() + '秒前');
      }
      xid.splice(numSlice * lenSlice - 1, 1, '现在');
      var option = {
        animation: false,
        grid: { x: 40, y: 50, x2: 40, y2: 30 },
        title: {
          top: '3%',
          left: 'center',
          text: 'CPU利用率',
          textStyle: { color: '#FFF' },
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: { animation: false },
          appendToBody:true,
          formatter: function (params) {
            return params[0].name + " CPU的利用率："+params[0].value+"%";
          }
        },
        xAxis: [{
          show: true,
          type: 'category',
          boundaryGap: false,
          data: xid,
          axisLine: { lineStyle: { type: 'solid', color: '#eeeeee' } },
          axisLabel: {
            textStyle: { color: '#eeeeee' },
            interval: Math.max(Math.floor(xid.length/5) - 1, 0),
            showMaxLabel: true
          }
        }],
        yAxis: [{
          type: 'value',
          axisLine: { lineStyle: { type: 'solid', color: '#eeeeee' }, },
          axisLabel: { textStyle: { color: '#eeeeee' }, formatter:'{value}%' },
          max: 100
        }],
        series: [{
          name: 'CPU利用率',
          type: 'line',
          showSymbol: false,
          data: dataset,
          color: '#409EFF'
        }]
      }
      myChart_L1.setOption(option);
    },
    myGraph2(myGraph) {
      // CPU核心占用率柱状图
      var option = {
        color: '#FF5252',
        grid: { x: 50, y: 50, x2: 50, y2: 40 },
        tooltip: {
          trigger: 'axis',
          axisPointer: { type: 'shadow' },
          appendToBody:true,
          formatter: function(params) {
            return "核心" + params.dataIndex + " 占用率：" + params.value + "%";
          }
        },
        title: {
          top: '5%',
          left: 'center',
          text: 'CPU核心占用率',
          textStyle: { color: '#FFF' },
        },
        xAxis: {
          type: 'category',
          data: ['0', '1', '2', '3', '4', '5', '6', '7'],
          axisLine: { lineStyle: { type: 'solid', color: '#eeeeee' } }
        },
        yAxis: {
          type: 'value',
          max: 100,
          axisLabel: { textStyle: { color: '#eeeeee' }, formatter:'{value}%' },
          axisLine: { lineStyle: { type: 'solid', color: '#eeeeee' } }
        },
        series: [{
          name: '核心占用率',
          type: 'bar',
          data: (this.CPU_percent || []).map(p => {
            const n = Number(p) || 0; return parseFloat(n.toFixed(1));
          }),
          barWidth: 30
        }]
      };
      myGraph.setOption(option);
    },
    getData() {
      // 初始化图表并定时请求后端数据
      let lineGraph = this.$echarts.init(document.getElementById('graph1'));
      let barGraph = this.$echarts.init(document.getElementById('graph2'));
      let _this = this;
      var numset1 = [];
      var numSlice = 30;
      var lenSlice = 1;
      for( var i = 0; i < numSlice*lenSlice; i++) numset1.push(0);

      async function chartsInit() {
        try {
          console.log('[cpu.vue] chartsInit start');
          const res = await _this.$http.get('/monitor/status');
          console.log('[cpu.vue] /monitor/status raw 返回：', res);
          // 支持多种后端返回结构：res.cpu | res.cpu_data.cpu_percent | res.data.cpu
          let cpuArray = [];
          if (res) {
            if (Array.isArray(res.cpu)) cpuArray = res.cpu;
            else if (res.cpu_data && Array.isArray(res.cpu_data.cpu_percent)) cpuArray = res.cpu_data.cpu_percent;
            else if (res.data && Array.isArray(res.data.cpu)) cpuArray = res.data.cpu;
          }
          console.log('[cpu.vue] 解析后的 cpuArray 长度：', cpuArray ? cpuArray.length : 0, cpuArray);
          // ensure numeric values
          _this.CPU_percent = (cpuArray || []).map(v => Number(v) || 0);
          // compute avg safely
          for( var i = 0; i < lenSlice; i++) {
            let avg = 0;
            if (_this.CPU_percent.length > 0) {
              avg = _this.CPU_percent.reduce((a, b) => a + b, 0) / _this.CPU_percent.length;
            }
            numset1.push(parseFloat(avg.toFixed(2)));
          }
          numset1.splice(0, lenSlice);
          _this.myEcharts(lineGraph, numset1);
          _this.myGraph2(barGraph);
        } catch (error) {
          console.error('[cpu.vue] /monitor/status 请求失败', error);
        }
      }
      chartsInit();
      this.timer = setInterval(chartsInit, 2000);
      window.onresize = function(){
        lineGraph.resize();
        barGraph.resize();
      };
    }
  },
  mounted() {
    this.getData();
  },
  beforeDestroy() {
    if (this.timer) clearInterval(this.timer);
  }
}
</script>

<style scoped>
.el-container {
  padding: 0;
  margin: 0;
  height: calc(100vh - 142px);
}
.el-aside {
  padding: 0;
  margin: 0;
  color: #333;
  height: 100%;
}
.el-main {
  padding: 0;
  margin: 0;
  height: 100%;
  width: 100%
}
.TopBox {
  width: 90%;
  margin: 1% auto;
  height: 55%;
  border: 1px solid rgb(68,68,68);
  border-radius: 10px;
  box-shadow: 0 2px 5px black;
}
.BottomBox_left {
  width: 45%;
  margin: 1% auto;
  height: 35%;
  border: 1px solid rgb(68,68,68);
  border-radius: 10px;
  box-shadow: 0 2px 5px black;
}
</style>