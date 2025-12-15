<template>
  <div>
    <el-container>
      <el-header height="2%"></el-header>
      <el-main>
        <div class="LeftBox" id="graph1"></div>
        <div class="RightBox" id="graph2"></div>
        <div class="LeftBox" id="graph3"></div>
        <div class="RightBox" id="graph4"></div>
        <div class="LeftBox" id="graph5"></div>
        <div class="RightBox" id="graph6"></div>
      </el-main>
    </el-container>
  </div>
</template>

<script>
  export default {
    name: 'M1D3',
    methods:{
      myEcharts(){
        //this.cgsJg=echarts.init(this.$refs.cgsJ);(自己联系的项目中)

        let myChart_L1 = this.$echarts.init(document.getElementById('graph1'));
        let myChart_R1 = this.$echarts.init(document.getElementById('graph2'));
        let myChart_L2 = this.$echarts.init(document.getElementById('graph3'));
        let myChart_R2 = this.$echarts.init(document.getElementById('graph4'));
        let myChart_L3 = this.$echarts.init(document.getElementById('graph5'));
        let myChart_R3 = this.$echarts.init(document.getElementById('graph6'));

        var xid = [];
        var flag = 0;
        const numWindow = 30;
        const lenSlice = 36;
        const numOverlap = 6;
        const numReal = 24;
        const numPred = 6;

        for (var i = 0; i < numWindow * lenSlice; i++) {
          var tmp = (numReal * lenSlice - i) / lenSlice;
          if( tmp > 0) xid.push(parseFloat(tmp.toFixed(3)).toString() + '秒前');
          else if( tmp == 0) xid.push('现在');
          else xid.push(parseFloat((-tmp).toFixed(3)).toString() + '秒后');
        }
        xid.splice(numWindow*lenSlice-1,1,numPred.toString() + '秒后')

        function showOption( dataset1, dataset2, title) {
          var option = {
            animation: false,
            grid: {
              y2: 30  //图表下方空白部分高度
            },
            title: {
              top: '5%',
              left: 'center',
              text: title,
              textStyle: {
                color: '#FFF',
              },
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
              data: ['真实值', '预测值'],
              textStyle: {
                color: '#eeeeee'
              }
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
                areaStyle: {}
              },
              {
                name: '预测值',
                type: 'line',
                showSymbol: false,
                color: '#f9e264',
                data: dataset2,
                areaStyle: {}
              }]
          };return option}

        var ddata = require("../../../data/prediction/RNN/ambient_temp_prediction.json");
        var edata = require("../../../data/prediction/RNN/cpu_utilization_prediction.json");
        var fdata = require("../../../data/prediction/RNN/machine_temp_prediction.json");
        var gdata = require("../../../data/prediction/RNN/nyc_taxi_prediction.json");
        var hdata = require("../../../data/prediction/RNN/request_latency_prediction.json");
        var idata = require("../../../data/prediction/RNN/request_count_prediction.json");

        function getJsonLength(jsonData){
          var jsonLength = 0;
          for(var item in jsonData){
            jsonLength++;
          }
          return jsonLength;
        }
        var lenDdata = getJsonLength(ddata);
        var lenEdata = getJsonLength(edata);
        var lenFdata = getJsonLength(fdata);
        var lenGdata = getJsonLength(gdata);
        var lenHdata = getJsonLength(hdata);
        var lenIdata = getJsonLength(idata);

        function numsetInit( Data, lenData) {
          var dataset1 = [], dataset2 = [];
          //var offset = numOverlap*lenSlice;
          var offset = parseInt(Math.random()*Math.min(lenData/2, lenData - numWindow * lenSlice - 2));
          //console.log( lenData, offset);
          for (var i = 0; i < numWindow * lenSlice; i++) {
            if (i < (numReal - numOverlap) * lenSlice) {
              dataset1.push(parseFloat(Data[offset+i].value).toFixed(3));
              dataset2.push(null);
            } else if (i > (numReal - numOverlap) * lenSlice - 1 && i < numReal * lenSlice) {
              dataset1.push(parseFloat(Data[offset+i].value).toFixed(3));
              dataset2.push(parseFloat(Data[offset+i].forecast).toFixed(3));
            } else {
              dataset1.push(null);
              dataset2.push(parseFloat(Data[offset+i].forecast).toFixed(3));
            }
          }
          return {dataset1:dataset1,dataset2:dataset2,offset:offset+numReal*lenSlice};
        }

        var numpair1 = numsetInit( ddata, lenDdata);
        var numpair2 = numsetInit( edata, lenEdata);
        var numpair3 = numsetInit( fdata, lenFdata);
        var numpair4 = numsetInit( gdata, lenGdata);
        var numpair5 = numsetInit( hdata, lenHdata);
        var numpair6 = numsetInit( idata, lenIdata);
        var numset1 = numpair1.dataset1;
        var numset2 = numpair1.dataset2;
        var indexReald = numpair1.offset;
        var numset3 = numpair2.dataset1;
        var numset4 = numpair2.dataset2;
        var indexReale = numpair2.offset;
        var numset5 = numpair3.dataset1;
        var numset6 = numpair3.dataset2;
        var indexRealf = numpair3.offset;
        var numset7 = numpair4.dataset1;
        var numset8 = numpair4.dataset2;
        var indexRealg = numpair4.offset;
        var numset9 = numpair5.dataset1;
        var numset10 = numpair5.dataset2;
        var indexRealh = numpair5.offset;
        var numset11 = numpair6.dataset1;
        var numset12 = numpair6.dataset2;
        var indexReali = numpair6.offset;

        if( flag==0) {
          myChart_L1.setOption(showOption(numset1, numset2,'模型量化耗时'));
          myChart_R1.setOption(showOption(numset3, numset4,'量化模型算力占用'));
          myChart_L2.setOption(showOption(numset5, numset6,'模型推理时延'));
          myChart_R2.setOption(showOption(numset7, numset8,'量化模型加载次数'));
          myChart_L3.setOption(showOption(numset9, numset10,'量化校准耗时'));
          myChart_R3.setOption(showOption(numset11, numset12,'模型推理吞吐量'));
        }

        this.timer = setInterval(function () {
          //console.log(indexReald,indexReale,indexRealf,indexRealg,indexRealh,indexReali,)
          flag = 1;
          numset1.splice(0,lenSlice);
          numset3.splice(0,lenSlice);
          numset5.splice(0,lenSlice);
          numset7.splice(0,lenSlice);
          numset9.splice(0,lenSlice);
          numset11.splice(0,lenSlice);
          for( var i = 0; i < lenSlice; i++) {
            numset1.splice((numReal-1)*lenSlice+i,0,parseFloat(ddata[(indexReald+i)%lenDdata].value).toFixed(3));
            numset3.splice((numReal-1)*lenSlice+i,0,parseFloat(edata[(indexReale+i)%lenEdata].value).toFixed(3));
            numset5.splice((numReal-1)*lenSlice+i,0,parseFloat(fdata[(indexRealf+i)%lenFdata].value).toFixed(3));
            numset7.splice((numReal-1)*lenSlice+i,0,parseFloat(gdata[(indexRealg+i)%lenGdata].value).toFixed(3));
            numset9.splice((numReal-1)*lenSlice+i,0,parseFloat(hdata[(indexRealh+i)%lenHdata].value).toFixed(3));
            numset11.splice((numReal-1)*lenSlice+i,0,parseFloat(idata[(indexReali+i)%lenIdata].value).toFixed(3));
          }

          numset2.splice((numReal-numOverlap)*lenSlice,lenSlice);
          numset4.splice((numReal-numOverlap)*lenSlice,lenSlice);
          numset6.splice((numReal-numOverlap)*lenSlice,lenSlice);
          numset8.splice((numReal-numOverlap)*lenSlice,lenSlice);
          numset10.splice((numReal-numOverlap)*lenSlice,lenSlice);
          numset12.splice((numReal-numOverlap)*lenSlice,lenSlice);
          for( var i = 0; i < lenSlice; i++) {
            numset2.splice((numWindow-1)*lenSlice+i,0, parseFloat(ddata[(indexReald+numPred*lenSlice+i)%lenDdata].forecast).toFixed(3));
            numset4.splice((numWindow-1)*lenSlice+i,0, parseFloat(edata[(indexReale+numPred*lenSlice+i)%lenEdata].forecast).toFixed(3));
            numset6.splice((numWindow-1)*lenSlice+i,0, parseFloat(fdata[(indexRealf+numPred*lenSlice+i)%lenFdata].forecast).toFixed(3));
            numset8.splice((numWindow-1)*lenSlice+i,0, parseFloat(gdata[(indexRealg+numPred*lenSlice+i)%lenGdata].forecast).toFixed(3));
            numset10.splice((numWindow-1)*lenSlice+i,0, parseFloat(hdata[(indexRealh+numPred*lenSlice+i)%lenHdata].forecast).toFixed(3));
            numset12.splice((numWindow-1)*lenSlice+i,0, parseFloat(idata[(indexReali+numPred*lenSlice+i)%lenIdata].forecast).toFixed(3));
          }

          indexReald += lenSlice;
          indexReald %= lenDdata;
          indexReale += lenSlice;
          indexReale %= lenEdata;
          indexRealf += lenSlice;
          indexRealf %= lenFdata;
          indexRealg += lenSlice;
          indexRealg %= lenGdata;
          indexRealh += lenSlice;
          indexRealh %= lenHdata;
          indexReali += lenSlice;
          indexReali %= lenIdata;
          //console.log(indexReald,indexReale,indexRealf,indexRealg,indexRealh);
          // 使用刚指定的配置项和数据显示图表。
          myChart_L1.setOption(showOption(numset1, numset2,'模型量化耗时'));
          myChart_R1.setOption(showOption(numset3, numset4,'量化模型算力占用'));
          myChart_L2.setOption(showOption(numset5, numset6,'模型推理时延'));
          myChart_R2.setOption(showOption(numset7, numset8,'量化模型加载次数'));
          myChart_L3.setOption(showOption(numset9, numset10,'量化校准耗时'));
          myChart_R3.setOption(showOption(numset11, numset12,'模型推理吞吐量'));
        },1000)
        window.onresize = function(){
          myChart_L1.resize();
          myChart_R1.resize();
          myChart_L2.resize();
          myChart_R2.resize();
          myChart_L3.resize();
          myChart_R3.resize();
        }
      }
    },
    mounted() {
      //this.$nextTick(function() {
      this.myEcharts();//})
      Notification.closeAll();
    },
    beforeDestroy() {
      if (this.timer) {
        clearInterval(this.timer); // 在Vue实例销毁前，清除我们的定时器
      }
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
  }
  .LeftBox {
    width: 48%;
    float: left;
    margin-right: 1%;
    margin-bottom: 1%;
    height: 31%;
    border: #333333;
    border-radius: 10px;
    box-shadow: 0 2px 5px black
  }
  .RightBox{
    width: 48%;
    float: right;
    margin-right: 1%;
    margin-bottom: 1%;
    height: 31%;

    border: #333333;
    border-radius: 10px;
    box-shadow: 0 2px 5px black
  }
</style>
