<template>
  <div>
    <el-container>
      <el-aside width="20%" style="background-color: #252525;border-right-color: #252525;">
        <div class="compare" style="padding-bottom: 5%">
          <div style="margin-top: 5%;margin-left: 20%;padding-top:10%;width: 60%;color: #409EFF;font-size: 3vh">鸿蒙玲珑核模型</div>
          <div style="margin-top: 5%;margin-left: 14%;width: 50%;">
            <el-radio v-model="mdID" label="M1D1" @change="changeActive()">nyc_taxi数据集</el-radio>
          </div>
          <div style="margin-top: 5%;margin-left: 14%;width: 50%;">
            <el-radio v-model="mdID" label="M1D2" @change="changeActive()">ambient_temp数据集</el-radio>
          </div>
          <div style="margin-top: 5%;margin-left: 14%;width: 50%;">
            <el-radio v-model="mdID" label="M1D3" @change="changeActive()">request_latency数据集</el-radio>
          </div>
        </div>
        <div class="Evaluation" style="text-align: right;width: 80%;height: 20%;margin-top: 8%;margin-left: 8%">
          <div style="color: #eeeeee; font-weight: bold;text-align:center">指标</div>
          <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-left:6%;margin-right: 3%">Precision<br/><div class="font" id="Pre">{{Precison}}</div></div>
          <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-right: 2%">Recall<br/><div class="font" id="Re">{{Recall}}</div></div>
          <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left">F1_Score<br/><div class="font" id="F1">{{F1_Score}}</div></div>
        </div>
        <div style="margin-top: 8%;margin-left: 10%;width: 80%;height: 40%" class="charts" id="index1"></div>
        </el-aside>

      <el-container>
        <router-view></router-view>
      </el-container>
    </el-container>
  </div>
</template>

<script>
const nums = [0.592,0.176,0.655,0.533,0.613,0.574,0.591,0.625,0.517]
export default {
  name: 'Check',
  data () {
    var str = this.$route.name;
    return {
      mdID: str,
      Precison:0.93,
      Recall:0.93,
      F1_Score:0.76
    };
  },
  methods: {
    changeActive() {
      //console.log(this.mdID)
      //console.log(this.$route)
      window.location.href = '/#/check/' + this.mdID;
      if(this.mdID === 'M1D1'){
        document.getElementById('Pre').innerText = 0.93;
        document.getElementById('Re').innerText = 0.93;
        document.getElementById('F1').innerText = 0.76;
        document.getElementById('dataset1').style.color = "#409EFF";
        document.getElementById('dataset2').style.color = "#eeeeee";
        document.getElementById('dataset3').style.color = "#eeeeee";
      }
      if(this.mdID === 'M1D2'){
        document.getElementById('Pre').innerText = 0.95;
        document.getElementById('Re').innerText = 0.79;
        document.getElementById('F1').innerText = 0.86;
        document.getElementById('dataset1').style.color = "#eeeeee";
        document.getElementById('dataset2').style.color = "#409EFF";
        document.getElementById('dataset3').style.color = "#eeeeee";
      }
      if(this.mdID === 'M1D3'){
        document.getElementById('Pre').innerText = 0.9;
        document.getElementById('Re').innerText = 0.5;
        document.getElementById('F1').innerText = 0.64;
        document.getElementById('dataset1').style.color = "#eeeeee";
        document.getElementById('dataset2').style.color = "#eeeeee";
        document.getElementById('dataset3').style.color = "#409EFF";
      }
    },
    myEcharts(){
        let myEchart = this.$echarts.init(document.getElementById('index1'));
        var seriesLabel = {
          show: true
        }
        var option = {
          title: {
            text: '量化模型精度对比图',
            top:'4%',
            left:'20%',
            textStyle:{
              color:'#fff'
            }

          },
          tooltip: {
            trigger: 'axis',
            axisPointer: {
              type: 'shadow'
            }
          },
          legend: {
            data: ['鸿蒙玲珑核模型', 'INT8量化模型', 'INT4量化模型','FP16量化模型'],
            orient:'horizontal',
            top:'15%',
            left:'13%',
            textStyle:{
              fontSize:9,
              color: '#fff'
            }
          },
          grid: {
            top:"33%",
            left: "20%",
            right:"10%",
            bottom:"2%"
          },
          xAxis: {
            type: 'value',
            axisLabel: {
              formatter: '{value}',
              textStyle:{
                color: '#fff'
              }
            }
          },
          yAxis: {
            type: 'category',
            inverse: true,
            data: ['nyc_taxi\n数据集', 'ambient\ntemp\n数据集', 'request\nlatency\n数据集'],
            axisLabel:{
              textStyle:{
                color:'#fff'
              }
            },
            axisLine:{
              lineStyle:{
                color:'#fff'
              }
            }
          },
          series: [
            {
              name: '鸿蒙玲珑核模型',
              type: 'bar',
              data: [0.762,0.865,0.649],
              label: seriesLabel,
              itemStyle:{
                normal:{
                  textColor:'#fff'
                }
              }
            },
            {
              name: '时序差分模型',
              type: 'bar',
              label: seriesLabel,
              data: [0.592,0.176,0.655],
            },
            {
              name: '基于分类的模型',
              type: 'bar',
              label: seriesLabel,
              data: [0.533,0.613,0.574]
            },
            {
              name: '基于回归的模型',
              type: 'bar',
              label: seriesLabel,
              data: [0.591,0.625,0.517]
            }
          ]
        };
        myEchart.setOption(option);
        window.onresize = function(){
          myEchart.resize();
        }
      }
  },
    mounted() {
      this.myEcharts();
    },
    computed: {
      activeMenu() {
        const route = this.$route
        const { meta, path } = route
        // if set path, the sidebar will highlight the path you set
        return meta.secondMenu
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
  width: 100%
}
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
  .charts{
    width: 80%;
    height: 60%;
    margin-top: 33%;
    margin-left: 2%;
    margin-right: 3%;
    border: #333333;
    border-radius: 10px;
    box-shadow: 0 2px 5px black
  }
/deep/  .el-submenu__title{
  color: #eeeeee !important;
}
/deep/  .el-submenu__title:hover {
  background-color: #1c1c1c;
}
/deep/  .el-menu-item {
  color: #eeeeee;
}
/deep/ .el-menu{
  background-color: #252525;
}
/deep/ .el-menu-item:hover {
  outline: 0;
  background-color: #1c1c1c;
}

/deep/ .el-menu-item:focus{
  color: #409EFF;
  background-color: #1c1c1c;
}

.el-radio {
  color: #eeeeee;
  margin-top: 5%;
}

/deep/  .el-radio__label {
  font-size: 1.8vh;
}

.fontDataset{
  font-size: 2.8vh;
}

.Evaluation{
  position: relative;
  left:2%;
  margin-top: 1%;
  font-size: 2.1vw;
  font-size: 2.1vh;
  font-weight: 700;
  line-height: 250%;
  padding-bottom: 0.5%;
  padding-top: 0%;
  border-radius: 10px;
  box-shadow: 0 2px 5px black;
}
.el-button:focus, .el-button:hover {
  color: #409eff;
}

/deep/  .link:hover,.link:focus,
.server-table i.el-tooltip:hover {
  cursor: pointer;
  color: #409EFF;
}

.link{
  color: #eeeeee;
  text-decoration: none;
}
.button{
  height: 10%;
  width: 80%;
  margin-left: 10%;
  margin-right: 3%;
  color: #eeeeee;
  border: #333333;
  border-radius: 10px;
  box-shadow: 0 2px 5px black;
  font-size: 3vh;
}
.compare{
  width: 80%;
  margin-left: 10%;
  margin-right: 3%;
  border: #333333;
  border-radius: 10px;
  box-shadow: 0 2px 5px black;
}
.font{
  font-size: 3vw;
  font-size: 3vh;
  color: #409EFF;
}
</style>
