<template>
  <div style="width: 100%;background-color: #252525">
    <div id="main" style="margin-top: 2%;width: 55%;height: 90%; border:#333333;box-shadow: 0 2px 5px black;float: left"></div>
    <div style="margin-top: 1%;height: 50%;">
      <el-table
        :data="tableNums"
        height="750"
        border
        style="width: 32%;float: right;right: 5%;top: 30px;background-color: #252525;text-align: center;box-shadow: 0 2px 5px black;"
        max-height="80%"
        :header-cell-style="{background:'#252525',color:'#eeeeee'}"
        :row-style="{background:'#252525',color:'#eeeeee'}">
        <el-table-column
          prop="timestamp"
          label="异常点时间戳"
          :fit="true">
        </el-table-column>
        <el-table-column
          prop="forecast"
          label="预测值">
        </el-table-column>
        <el-table-column
          prop="value"
          label="真实值">
        </el-table-column>
      </el-table>
    </div>
    <div id="main2" style="margin-right:5%;width: 32%;height: 40%; border:#333333;box-shadow: 0 2px 5px black;float: right"></div>
  </div>
</template>

<script>
export default {
  name: 'M1D1',
  data() {
    return {
      tableNums: []
    }
  },
  methods:{
    myEcharts(){
      //this.cgsJg=echarts.init(this.$refs.cgsJ);(自己联系的项目中)

      let myChart_L1 = this.$echarts.init(document.getElementById('main'));

      var ii = 0;
      var xid = [];
      var numset1 = [];
      var numset2 = [];

      var flag = 0;
      const numWindow = 30;
      const lenSlice = 36;
      const numOverlap = 30;
      const numReal = 30;

      function showOption( dataset1, dataset2, title, pieces) {
        var option = {
          animation: false,
          grid: {
            y2: 30  //图表下方空白部分高度
          },
          title: {
            top: '2%',
            left: 'center',
            text: title,
            textStyle: {
              color: '#FFF',
            }
          },
          tooltip: {
            trigger: 'axis',
            axisPointer: {
              animation: false
            }
          },
          legend: {
            type: 'scroll',
            orient: 'vertical',
            right: 1,
            bottom: 20,
            top: '2%',
            data: ['预测值', '真实值', '异常值'],
            textStyle: {
              color: '#eeeeee'
            }
          },
          visualMap: {
            show: false,
            type: 'piecewise',
            max: 0,
            dimension: 0,
            pieces: pieces,
            outOfRange: {
              color: '#f9e264'
            },
            seriesIndex:1
          },
          xAxis: {
            //show: false,
            type: 'category',
            data: xid,
            axisLine: {
              lineStyle: {
                type: 'solid',
                color: '#eeeeee',//左边线的颜色
              }
            },
            axisLabel: {
              textStyle: {
                color: '#eeeeee',//坐标值得具体的颜色
              },
              interval: xid.length/5-1,
              showMaxLabel: true
            }
          },
          yAxis: {
            type: 'value',
            scale: true,
            axisLine: {
              lineStyle: {
                type: 'solid',
                color: '#eeeeee',//左边线的颜色
              }
            },
            axisLabel: {
              textStyle: {
                color: '#eeeeee',//坐标值得具体的颜色
              }
            }
          },
          series: [
            {
              name: '真实值',
              type: 'line',
              showSymbol: false,
              color: '#409EFF',
              data: dataset1,
              //areaStyle: {}
            },
            {
              name: '预测值',
              type: 'line',
              showSymbol: false,
              color: '#f9e264',
              data: dataset2,
              //areaStyle: {}
            },
            {
              name: '异常值',
              type: 'line',
              showSymbol: false,
              color: '#FF0000',
              data: [null],
            }]
        };return option}

      //var ddata = require("../../../data/check/cpu_utilization.json");
      //var ddata = require("../../../data/check/ambient_temp.json");
      var ddata = require("../../../data/check/HAYDN/request_latency.json");

      function getJsonLength(jsonData){
        var jsonLength = 0;
        for(var item in jsonData){
          jsonLength++;
        }
        return jsonLength;
      }
      var lenDataset = getJsonLength(ddata);
      //console.log(lenDataset);
      var offset = parseInt(Math.random()*Math.min(lenDataset/2, lenDataset - numWindow * lenSlice - 2));

      for( var i = 0; i < numWindow*lenSlice; i++) {
        xid.push(parseFloat(((numWindow*lenSlice-i)/lenSlice).toFixed(3)).toString()+'秒前');
        numset1.push(parseFloat(ddata[offset+i].value).toFixed(3));
        numset2.push(parseFloat(ddata[offset+i].forecast).toFixed(3));
      }
      xid.splice(numWindow*lenSlice-1,1,'现在')

      var indexReal = offset + numWindow*lenSlice;
      var _this = this;

      var pieces = [];
      var countFirst= 0;
      var mmax = lenSlice*numWindow;
      for( let i = 0; i < mmax; i++) {
        var offsetData = indexReal-mmax;
        var curIndexData = offsetData + i;
        if( ddata[curIndexData].is_anomaly === 1) {
          pieces.push({gte: i - 2, lte: i + 2, color: 'red'});
          //console.log(pieces);
          let curData = {timestamp:ddata[curIndexData].timestamp,forecast:parseFloat(ddata[curIndexData].forecast).toFixed(3),value:parseFloat(ddata[curIndexData].value).toFixed(3)};
          _this.tableNums.splice(0,0,curData);
          //_this.tableNums.push(curData);
          countFirst++;
        }
      }
      if (countFirst !== 0){
        _this.open1(countFirst);
      }

      if( flag==0) {
        myChart_L1.setOption(showOption(numset1, numset2,'HarmonyOS-Linglong-Kernel模型-request_latency数据集', pieces));
      }

      this.timer = window.setInterval(function () {
        console.log("timer1");
        let i;
        let countSecond = 0;
        flag = 1;
        numset1.splice(0,lenSlice);
        numset2.splice((numReal-numOverlap)*lenSlice,lenSlice);
        for(i = 0; i < lenSlice; i++) {
          var curIndexData = indexReal+i;
          numset1.splice((numReal-1)*lenSlice+i,0,parseFloat(ddata[curIndexData%lenDataset].value).toFixed(3));
          numset2.splice((numWindow-1)*lenSlice+i,0, parseFloat(ddata[curIndexData%lenDataset].forecast).toFixed(3));
          if (ddata[curIndexData%lenDataset].is_anomaly === 1){
            let curData = {timestamp:ddata[curIndexData%lenDataset].timestamp,forecast:parseFloat(ddata[curIndexData%lenDataset].forecast).toFixed(3),value:parseFloat(ddata[curIndexData%lenDataset].value).toFixed(3)};
            _this.tableNums.splice(0,0,curData);
            countSecond++;
          }
        }
        if (countSecond!==0){
          _this.open2(ddata[curIndexData%lenDataset].timestamp,countSecond);
        }

        var pieces = [];
        var mmax = lenSlice*numWindow;
        for( let i = 0; i < mmax; i++) {
          var offsetData = indexReal+lenSlice-mmax;
          var curIndexData = offsetData + i;
          if( curIndexData >= lenDataset) curIndexData -= lenDataset;
          else if( curIndexData < 0) curIndexData += lenDataset;
          if( ddata[curIndexData].is_anomaly === 1) {
            pieces.push({gte: i - 2, lte: i + 2, color: 'red'});
            //console.log(pieces);
          }
        }

        indexReal += lenSlice;
        indexReal %= lenDataset;
        //console.log(indexReal);
        ii++;
        // 使用刚指定的配置项和数据显示图表。
        myChart_L1.setOption(showOption(numset1, numset2,'HAYDN模型-request_latency数据集',pieces));
      },1000)
      window.addEventListener("resize",()=>{
        myChart_L1.resize();
      })
    },
    myEcharts2(){
      //this.cgsJg=echarts.init(this.$refs.cgsJ);(自己联系的项目中)

      let myChart_L1 = this.$echarts.init(document.getElementById('main2'));

      var ii = 0;
      var xid = [];
      var numset1 = [];
      var numset2 = [];
      var numset3 = [];
      var numset4 = [];
      var numset5 = [];
      var numset6 = [];
      var numset7 = [];
      var numset8 = [];

      var flag = 0;
      const numWindow = 30;
      const lenSlice = 36;
      const numOverlap = 30;
      const numReal = 30;

      function showOption( dataset1, dataset2, dataset3,dataset4,dataset5,dataset6,dataset7,dataset8,title, pieces) {
        var option = {
          animation: false,
          grid: {
            y2: 30  //图表下方空白部分高度
          },
          title: {
            top: '2%',
            left: 'center',
            text: title,
            textStyle: {
              color: '#FFF',
            }
          },
          tooltip: {
            trigger: 'axis',
            axisPointer: {
              animation: false
            }
          },
          legend: {
            type: 'scroll',
            top: '10%',
            left:'10%',
            data: ['真实值','HarmonyOS-Linglong-Kernel模型预测值','HarmonyOS-Linglong-Kernel模型异常值','INT8量化模型预测值','INT8量化模型异常值','INT4量化模型预测值','INT4量化模型异常值','FP16量化模型预测值','FP16量化模型异常值'],
            textStyle: {
              color: '#eeeeee'
            }
          },
          visualMap: {
            show: false,
            type: 'piecewise',
            max: 0,
            dimension: 0,
            pieces: pieces,
            outOfRange: {
              color: '#f9e264'
            },
            seriesIndex:1
          },
          xAxis: {
            //show: false,
            type: 'category',
            data: xid,
            axisLine: {
              lineStyle: {
                type: 'solid',
                color: '#eeeeee',//左边线的颜色
              }
            },
            axisLabel: {
              textStyle: {
                color: '#eeeeee',//坐标值得具体的颜色
              },
              interval: xid.length/5-1,
              showMaxLabel: true
            }
          },
          yAxis: {
            type: 'value',
            scale: true,
            axisLine: {
              lineStyle: {
                type: 'solid',
                color: '#eeeeee',//左边线的颜色
              }
            },
            axisLabel: {
              textStyle: {
                color: '#eeeeee',//坐标值得具体的颜色
              }
            }
          },
          series: [
            {
              name: '真实值',
              type: 'line',
              showSymbol: false,
              color: '#73f964',
              data: dataset1,
              //areaStyle: {}
            },
            {
              name: 'HarmonyOS-Linglong-Kernel模型预测值',
              type: 'line',
              showSymbol: false,
              color: '#f9e264',
              data: dataset2,
              //areaStyle: {}
            },
            {
              name: 'HarmonyOS-Linglong-Kernel模型异常值',
              type: 'line',
              showSymbol: false,
              color: '#FF0000',
              data: [null],
            },
            {
              name: '基于ARIMA差分时序模型预测值',
              type: 'line',
              showSymbol: false,
              color: '#0e3adb',
              data: dataset4,
              //areaStyle: {}
            },
            {
              name: '基于ARIMA差分时序模型异常值',
              type: 'line',
              showSymbol: false,
              color: '#FF0000',
              data: [null],
            },
            {
              name: '基于分类的模型预测值',
              type: 'line',
              showSymbol: false,
              color: '#ee8a69',
              data: dataset6,
            },
            {
              name: '基于分类的模型异常值',
              type: 'line',
              showSymbol: false,
              color: '#FF0000',
              data: [null],
            },
            {
              name: '基于回归的模型预测值',
              type: 'line',
              showSymbol: false,
              color: '#ac69ee',
              data: dataset8,
            },
            {
              name: '基于回归的模型异常值',
              type: 'line',
              showSymbol: false,
              color: '#FF0000',
              data: [null],
            }]
        };return option}

      //var ddata = require("../../../data/check/cpu_utilization.json");
      //var ddata = require("../../../data/check/ambient_temp.json");
      var ddata = require("../../../data/check/HAYDN/request_latency.json");
      var adata = require("../../../data/check/ARIMA/request_latency.json");
      var cdata = require("../../../data/check/CLASS/request_latency.json");
      var rdata = require("../../../data/check/CLASS/request_latency.json");

      function getJsonLength(jsonData){
        var jsonLength = 0;
        for(var item in jsonData){
          jsonLength++;
        }
        return jsonLength;
      }
      var lenDataset = getJsonLength(ddata);
      //console.log(lenDataset);
      var offset = parseInt(Math.random()*Math.min(lenDataset/2, lenDataset - numWindow * lenSlice - 2));

      for( var i = 0; i < numWindow*lenSlice; i++) {
        xid.push(parseFloat(((numWindow*lenSlice-i)/lenSlice).toFixed(3)).toString()+'秒前');
        numset1.push(parseFloat(ddata[offset+i].value).toFixed(3));
        numset2.push(parseFloat(ddata[offset+i].forecast).toFixed(3));
        numset3.push(parseFloat(adata[offset+i].value).toFixed(3));
        numset4.push(parseFloat(adata[offset+i].forecast).toFixed(3));
        numset5.push(parseFloat(cdata[offset+i].value).toFixed(3));
        numset6.push(parseFloat(cdata[offset+i].value).toFixed(3));
        numset7.push(parseFloat(rdata[offset+i].value).toFixed(3));
        numset8.push(parseFloat(rdata[offset+i].value).toFixed(3));
      }
      xid.splice(numWindow*lenSlice-1,1,'现在')

      var indexReal = offset + numWindow*lenSlice;
      var _this = this;

      var pieces = [];
      var countFirst= 0;
      var mmax = lenSlice*numWindow;
      for( let i = 0; i < mmax; i++) {
        var offsetData = indexReal-mmax;
        var curIndexData = offsetData + i;
        if( ddata[curIndexData].is_anomaly === 1) {
          pieces.push({gte: i - 2, lte: i + 2, color: 'red'});
          //console.log(pieces);
          let curData = {timestamp:ddata[curIndexData].timestamp,forecast:parseFloat(ddata[curIndexData].forecast).toFixed(3),value:parseFloat(ddata[curIndexData].value).toFixed(3)};
          _this.tableNums.splice(0,0,curData);
          //_this.tableNums.push(curData);
          countFirst++;
        }
      }
      if (countFirst !== 0){
        //_this.open1(countFirst);
      }

      if( flag==0) {
        myChart_L1.setOption(showOption(numset1, numset2,numset3,numset4,numset5,numset6,numset7,numset8,'nyc_taxi数据集多模型检测对比', pieces));
      }

      this.timer2 = setInterval(function () {
        console.log("timer2");
        let i;
        let countSecond = 0;
        flag = 1;
        numset1.splice(0,lenSlice);
        numset2.splice((numReal-numOverlap)*lenSlice,lenSlice);
        numset3.splice(0,lenSlice);
        numset4.splice((numReal-numOverlap)*lenSlice,lenSlice);
        numset5.splice(0,lenSlice);
        numset6.splice((numReal-numOverlap)*lenSlice,lenSlice);
        numset7.splice(0,lenSlice);
        numset8.splice((numReal-numOverlap)*lenSlice,lenSlice);
        for(i = 0; i < lenSlice; i++) {
          var curIndexData = indexReal+i;
          numset1.splice((numReal-1)*lenSlice+i,0,parseFloat(ddata[curIndexData%lenDataset].value).toFixed(3));
          numset2.splice((numWindow-1)*lenSlice+i,0, parseFloat(ddata[curIndexData%lenDataset].forecast).toFixed(3));
          numset3.splice((numReal-1)*lenSlice+i,0,parseFloat(adata[curIndexData%lenDataset].value).toFixed(3));
          numset4.splice((numWindow-1)*lenSlice+i,0, parseFloat(adata[curIndexData%lenDataset].forecast).toFixed(3));
          numset5.splice((numReal-1)*lenSlice+i,0,parseFloat(cdata[curIndexData%lenDataset].value).toFixed(3));
          numset6.splice((numWindow-1)*lenSlice+i,0, parseFloat(cdata[curIndexData%lenDataset].value).toFixed(3));
          numset7.splice((numReal-1)*lenSlice+i,0,parseFloat(rdata[curIndexData%lenDataset].value).toFixed(3));
          numset8.splice((numWindow-1)*lenSlice+i,0, parseFloat(rdata[curIndexData%lenDataset].value).toFixed(3));
          if (ddata[curIndexData%lenDataset].is_anomaly === 1){
            let curData = {timestamp:ddata[curIndexData%lenDataset].timestamp,forecast:parseFloat(ddata[curIndexData%lenDataset].forecast).toFixed(3),value:parseFloat(ddata[curIndexData%lenDataset].value).toFixed(3)};
            _this.tableNums.splice(0,0,curData);
            countSecond++;
          }
        }
        if (countSecond!==0){
          //_this.open2(ddata[curIndexData%lenDataset].timestamp,countSecond);
        }

        var pieces = [];
        var mmax = lenSlice*numWindow;
        for( let i = 0; i < mmax; i++) {
          var offsetData = indexReal+lenSlice-mmax;
          var curIndexData = offsetData + i;
          if( curIndexData >= lenDataset) curIndexData -= lenDataset;
          else if( curIndexData < 0) curIndexData += lenDataset;
          if( ddata[curIndexData].is_anomaly === 1) {
            pieces.push({gte: i - 2, lte: i + 2, color: 'red'});
            //console.log(pieces);
          }
        }

        indexReal += lenSlice;
        indexReal %= lenDataset;
        //console.log(indexReal);
        ii++;
        // 使用刚指定的配置项和数据显示图表。
        myChart_L1.setOption(showOption(numset1, numset2,numset3,numset4,numset5,numset6,numset7,numset8,'nyc_taxi数据集多模型检测对比',pieces));
      },1000)
      window.addEventListener("resize",()=>{
        myChart_L1.resize();
      })
    },
    open1(count){
      this.$notify({
        title:'异常',
        message:'初始加载时出现'+count+'次指标值异常！',
        type:'warning',
        position:"top-right",
        duration:1000
      });
    },
    open2(timestamps,count){
      this.$notify({
        title:'异常',
        message:'时间戳'+timestamps+'左右出现'+count+'次指标值异常！',
        type:'warning',
        position:"top-right",
        duration:1000
      });
    }
  },
  mounted() {
    //this.$nextTick(function() {
    this.myEcharts();//})
    this.myEcharts2();
  },
  beforeDestroy () {
    window.clearInterval(this.timer);
    this.timer = null;
    window.clearInterval(this.timer2);
    this.timer2 = null;
  }
}
</script>

<style>
#main {
  top:0%;
  left:5%;
  right:5%;
}

.el-table--enable-row-hover .el-table__body tr:hover>td {
  background-color: #409EFF;
  color: white;
}

</style>
