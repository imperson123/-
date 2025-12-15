<template>
  <div class="el-container">
    <div class="el-aside">
      <!-- 左侧导航栏保持不变 -->
    </div>
    <div class="el-main">
      <!-- 只保留顶部磁盘趋势图 -->
      <div class="TopBox">
        <div id="graph1" style="width: 100%; height: 100%;"></div>
      </div>
    </div>
  </div>
</template>

<script>
import Vue from "vue";
export default {
  name: 'disk',
  data() {
    return {
      diskUsage: {},
      timer: null
    }
  },
  methods: {
    myEcharts(myChart_L1, dataset) {
      // 磁盘占用率趋势图
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
          text: '磁盘占用率',
          textStyle: { color: '#FFF' },
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: { animation: false },
          appendToBody:true,
          formatter: function (params) {
            return params[0].name + "磁盘的占用率："+params[0].value+"%";
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
            interval: xid.length/5 - 1,
            showMaxLabel: true
          }
        }],
        yAxis: [{
          type: 'value',
          axisLine: { lineStyle: { type: 'solid', color: '#eeeeee' } },
          axisLabel: { textStyle: { color: '#eeeeee' }, formatter:'{value}%' },
          max: 100
        }],
        series: [{
          name: '磁盘占用率',
          type: 'line',
          showSymbol: false,
          data: dataset,
          color: '#409EFF'
        }]
      }
      myChart_L1.setOption(option);
    },
    getData() {
      // 只保留顶部趋势图的初始化
      let lineGraph = this.$echarts.init(document.getElementById('graph1'));
      let _this = this;
      var numset1 = [];
      var numSlice = 30;
      var lenSlice = 1;
      
      for( var i = 0; i < numSlice*lenSlice; i++) numset1.push(0);
      
      async function chartsInit() {
        try {
          const res = await _this.$http.get('/monitor/status');
          _this.diskUsage = res.disk_data.disk_usage;
        } catch (error) {
          console.error(error);
        }
        for( var i = 0; i < lenSlice; i++) {
          numset1.push(parseFloat(_this.diskUsage.percent || 0));
        }
        numset1.splice(0, lenSlice);
        _this.myEcharts(lineGraph, numset1);
      }
      chartsInit();
      this.timer = setInterval(chartsInit, 1000);
      window.onresize = function(){
        lineGraph.resize();
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
  height: 95%; /* 占满页面高度 */
  border: 1px solid rgb(68,68,68);
  border-radius: 10px;
  box-shadow: 0 2px 5px black;
}
</style>