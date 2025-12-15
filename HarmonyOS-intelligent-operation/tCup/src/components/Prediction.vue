<template>
  <div>
    <el-container style="background-color: #252525">
      <el-aside width = 20%>
        <el-button class = "button" @click="toM1D0()" style="background-color:#252525;color: #409EFF" autofocus id="model0">鸿蒙玲珑核模型</el-button>
        <div class="compare" style="padding-bottom: 5%">
          <div style="margin-top: 5%;margin-left: 35%;width: 50%;color: #eeeeee">量化模型（INT8/INT4/FP16）</div>
          <div style="margin-top: 5%;margin-left: 14%;width: 50%;">
            <el-radio v-model="modelID" label="M1D1" @change="changeActive()">INT8量化模型</el-radio>
          </div>
          <div style="margin-top: 5%;margin-left: 14%;width: 50%;">
            <el-radio v-model="modelID" label="M1D2" @change="changeActive()">INT4量化模型</el-radio>
          </div>
          <div style="margin-top: 5%;margin-left: 14%;width: 50%;">
            <el-radio v-model="modelID" label="M1D3" @change="changeActive()">FP16量化模型</el-radio>
          </div>
        </div>
        <div style="margin-top: 5%;margin-left: 10%;width: 80%;height: 18%">
          <el-carousel indicator-position="outside" height="17vh" interval="10000">
            <el-carousel-item :key="1">
                <div class="Evaluation" style="text-align: right;width: 100%;height: 100%">
                <div style="color: #eeeeee; font-weight: bold;text-align:center">模型量化耗时</div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-left:17%;margin-right: 5%" >精度损失<br/><div class="font">{{MAPE1}}</div></div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-right: 5%">余弦相似度<br/><div class="font">{{MAE1}}</div></div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left">PSNR<br/><div class="font">{{RMSE1}}</div></div>
              </div>
            </el-carousel-item>
            <el-carousel-item :key="2">
                  <div class="Evaluation" style="text-align: right;width: 100%;height: 100%">
                  <div style="color: #eeeeee; font-weight: bold;text-align:center">量化模型算力占用</div>
                  <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-left:16%;margin-right: 4%">精度损失<br/><div class="font">{{MAPE2}}</div></div>

                  <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-right: 4%">余弦相似度<br/><div class="font">{{MAE2}}</div></div>
                  <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left">PSNR<br/><div class="font">{{RMSE2}}</div></div>
                </div>
            </el-carousel-item>
            <el-carousel-item :key="3">
                <div class="Evaluation" style="text-align: right;width: 100%;height: 100%">
                <div style="color: #eeeeee; font-weight: bold;text-align:center">模型推理时延</div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-left:17%;margin-right: 5%">精度损失<br/><div class="font">{{MAPE3}}</div></div>

                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-right: 5%">余弦相似度<br/><div class="font">{{MAE3}}</div></div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left">PSNR<br/><div class="font">{{RMSE3}}</div></div>
              </div>
            </el-carousel-item>
            <el-carousel-item :key="4">
                <div class="Evaluation" style="text-align: right;width: 100%;height: 100%">
                <div style="color: #eeeeee; font-weight: bold;text-align:center">量化模型加载次数</div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-left:17%;margin-right: 5%">精度损失<br/><div class="font">{{MAPE4}}</div></div>

                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-right: 5%">余弦相似度<br/><div class="font">{{MAE4}}</div></div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left">PSNR<br/><div class="font">{{RMSE4}}</div></div>
              </div>
            </el-carousel-item>
            <el-carousel-item :key="5">
                <div class="Evaluation" style="text-align: right;width: 100%;height: 100%">
                <div style="color: #eeeeee; font-weight: bold;text-align:center">量化校准耗时</div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-left:17%;margin-right: 5%">精度损失<br/><div class="font">{{MAPE5}}</div></div>

                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-right: 5%">余弦相似度<br/><div class="font">{{MAE5}}</div></div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left">PSNR<br/><div class="font">{{RMSE5}}</div></div>
              </div>
            </el-carousel-item>
            <el-carousel-item :key="6">
                <div class="Evaluation" style="text-align: right;width: 100%;height: 100%">
                <div style="color: #eeeeee; font-weight: bold;text-align:center">模型推理吞吐量</div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-left:15%;margin-right: 2%">精度损失<br/><div class="font">{{MAPE6}}</div></div>

                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left;margin-right: 2%">余弦相似度<br/><div class="font">{{MAE6}}</div></div>
                <div style="color: #eeeeee; font-weight: bold;text-align:center;float:left">PSNR<br/><div class="font">{{RMSE6}}</div></div>
              </div>
            </el-carousel-item>
          </el-carousel>
        </div>
        <div style="margin-top: 13%;margin-left: 10%;width: 80%;height: 40%" class="charts" id="index1"></div>

      </el-aside>

      <el-main>
        <router-view></router-view>
      </el-main>
    </el-container>
  </div>
</template>

<script>
 const index = ['模型量化耗时','量化模型算力占用','模型推理时延','量化模型加载次数','量化校准耗时','模型推理吞吐量']
  export default {
    name: 'Prediction',
    data () {
      var str = this.$route.name;
      return {
        modelID: str,
        MAPE1: 1.46,
        MAE1: 1.78,
        RMSE1: 3.56,
        MAPE2: 1.46,
        MAE2: 1.78,
        RMSE2: 3.56,
        MAPE3: 1.46,
        MAE3: 1.78,
        RMSE3: 3.56,
        MAPE4: 1.46,
        MAE4: 1.78,
        RMSE4: 3.56,
        MAPE5: 1.46,
        MAE5: 1.78,
        RMSE5: 3.56,
        MAPE6: 1.46,
        MAE6: 1.78,
        RMSE6: 3.56,
        ddata: require("../data/prediction/ARIMA/ambient_temp_prediction_metrics.json"),
        edata: require("../data/prediction/ARIMA/cpu_utilization_prediction_metrics.json"),
        fdata: require("../data/prediction/ARIMA/machine_temp_prediction_metrics.json"),
        gdata: require("../data/prediction/ARIMA/nyc_taxi_prediction_metrics.json"),
        hdata: require("../data/prediction/ARIMA/request_latency_prediction_metrics.json"),
        idata: require("../data/prediction/ARIMA/request_count_prediction_metrics.json"),
        lenDdata: this.getJsonLength(require("../data/prediction/ARIMA/ambient_temp_prediction_metrics.json")),
        lenEdata: this.getJsonLength(require("../data/prediction/ARIMA/cpu_utilization_prediction_metrics.json")),
        lenFdata: this.getJsonLength(require("../data/prediction/ARIMA/machine_temp_prediction_metrics.json")),
        lenGdata: this.getJsonLength(require("../data/prediction/ARIMA/nyc_taxi_prediction_metrics.json")),
        lenHdata: this.getJsonLength(require("../data/prediction/ARIMA/request_latency_prediction_metrics.json")),
        lenIdata: this.getJsonLength(require("../data/prediction/ARIMA/request_count_prediction_metrics.json")),
        iid: 0,
        iie: 0,
        iif: 0,
        iig: 0,
        iih: 0,
        iii: 0,
        haydnD:1.46,
        haydnE:1.46,
        haydnF:1.46,
        haydnG:1.46,
        haydnH:1.46,
        haydnI:1.46,
        haydnIndex:0,
        haydnIndex2:0,
        haydnIndex3:0,
        haydnIndex4:0,
        haydnIndex5:0,
        haydnIndex6:0,

        arimaD:1.46,
        arimaE:1.46,
        arimaF:1.46,
        arimaG:1.46,
        arimaH:1.46,
        arimaI:1.46,
        tcnD:1.46,
        tcnE:1.46,
        tcnF:1.46,
        tcnG:1.46,
        tcnH:1.46,
        tcnI:1.46,
        rnnD:1.46,
        rnnE:1.46,
        rnnF:1.46,
        rnnG:1.46,
        rnnH:1.46,
        rnnI:1.46,
      };
    },
    methods: {
      changeActive() {
        window.location.href = '/#/prediction/' + this.modelID;
        this.selectData();
        //console.log(this.modelID);
        document.getElementById('model0').style.color = "#eeeeee";
      },
      toM1D0(){
        window.location.href = '/#/prediction/M1D0';
        document.getElementById('model0').style.color = "#409EFF";
        this.modelID = ''
      },
      getJsonLength(jsonData) {
        var jsonLength = 0;
        for(var item in jsonData){
          jsonLength++;
        }
        return jsonLength;
      },
      selectData() {
        if( this.modelID === 'M1D1') {
          this.ddata = require("../data/prediction/ARIMA/ambient_temp_prediction_metrics.json");
          this.edata = require("../data/prediction/ARIMA/cpu_utilization_prediction_metrics.json");
          this.fdata = require("../data/prediction/ARIMA/machine_temp_prediction_metrics.json");
          this.gdata = require("../data/prediction/ARIMA/nyc_taxi_prediction_metrics.json");
          this.hdata = require("../data/prediction/ARIMA/request_latency_prediction_metrics.json");
          this.idata = require("../data/prediction/ARIMA/request_count_prediction_metrics.json");
        }
        else if( this.modelID === 'M1D2') {
          this.ddata = require("../data/prediction/TCN/ambient_temp_prediction_metrics.json");
          this.edata = require("../data/prediction/TCN/cpu_utilization_prediction_metrics.json");
          this.fdata = require("../data/prediction/TCN/machine_temp_prediction_metrics.json");
          this.gdata = require("../data/prediction/TCN/nyc_taxi_prediction_metrics.json");
          this.hdata = require("../data/prediction/TCN/request_latency_prediction_metrics.json");
          this.idata = require("../data/prediction/TCN/request_count_prediction_metrics.json");
        }
        else if(this.modelID == 'M1D0'){
          this.ddata = require("../data/prediction/HAYDN/ambient_temp_prediction_metrics.json");
          this.edata = require("../data/prediction/HAYDN/cpu_utilization_prediction_metrics.json");
          this.fdata = require("../data/prediction/HAYDN/machine_temp_prediction_metrics.json");
          this.gdata = require("../data/prediction/HAYDN/nyc_taxi_prediction_metrics.json");
          this.hdata = require("../data/prediction/HAYDN/request_latency_prediction_metrics.json");
          this.idata = require("../data/prediction/HAYDN/request_count_prediction_metrics.json");
        }
        else {
          this.ddata = require("../data/prediction/RNN/ambient_temp_prediction_metrics.json");
          this.edata = require("../data/prediction/RNN/cpu_utilization_prediction_metrics.json");
          this.fdata = require("../data/prediction/RNN/machine_temp_prediction_metrics.json");
          this.gdata = require("../data/prediction/RNN/nyc_taxi_prediction_metrics.json");
          this.hdata = require("../data/prediction/RNN/request_latency_prediction_metrics.json");
          this.idata = require("../data/prediction/RNN/request_count_prediction_metrics.json");
        }

        //console.log(this.ddata);

        this.lenDdata = this.getJsonLength(this.ddata);
        this.lenEdata = this.getJsonLength(this.edata);
        this.lenFdata = this.getJsonLength(this.fdata);
        this.lenGdata = this.getJsonLength(this.gdata);
        this.lenHdata = this.getJsonLength(this.hdata);
        this.lenIdata = this.getJsonLength(this.idata);

        this.iid = 0;
        this.iie = 0;
        this.iif = 0;
        this.iig = 0;
        this.iih = 0;
        this.iii = 0;
      },
      showMetrics() {
        let _this = this;
        this.timer = setInterval(function() {

          //console.log(_this.iid,_this.iie,_this.iif,_this.iig,_this.iih,_this.iii)
          //console.log(_this.modelID)
          //console.log(_this.iid)
          var mape1, mae1, rmse1;
          //console.log(iid,iie,iif,iig,iih);
          mape1 = parseFloat(_this.ddata[_this.iid].MAPE);
          mae1 = parseFloat(_this.ddata[_this.iid].MAE);
          rmse1 = parseFloat(_this.ddata[_this.iid].RMSE);
          _this.MAPE1 = mape1.toFixed(2);
          _this.MAE1 = mae1.toFixed(2);
          _this.RMSE1 = rmse1.toFixed(2);
          var mape2, mae2, rmse2;
          //console.log(iid,iie,iif,iig,iih);
          mape2 = parseFloat(_this.edata[_this.iie].MAPE);
          mae2 = parseFloat(_this.edata[_this.iie].MAE);
          rmse2 = parseFloat(_this.edata[_this.iie].RMSE);
          _this.MAPE2 = mape2.toFixed(2);
          _this.MAE2 = mae2.toFixed(2);
          _this.RMSE2 = rmse2.toFixed(2);
          var mape3, mae3, rmse3;
          //console.log(iid,iie,iif,iig,iih);
          mape3 = parseFloat(_this.fdata[_this.iif].MAPE);
          mae3 = parseFloat(_this.fdata[_this.iif].MAE);
          rmse3 = parseFloat(_this.fdata[_this.iif].RMSE);
          _this.MAPE3 = mape3.toFixed(2);
          _this.MAE3 = mae3.toFixed(2);
          _this.RMSE3 = rmse3.toFixed(2);
          var mape4, mae4, rmse4;
          //console.log(iid,iie,iif,iig,iih);
          mape4 = parseFloat(_this.gdata[_this.iig].MAPE);
          mae4 = parseFloat(_this.gdata[_this.iig].MAE);
          rmse4 = parseFloat(_this.gdata[_this.iig].RMSE);
          _this.MAPE4 = mape4.toFixed(2);
          _this.MAE4 = mae4.toFixed(2);
          _this.RMSE4 = rmse4.toFixed(2);
          var mape5, mae5, rmse5;
          //console.log(iid,iie,iif,iig,iih);
          mape5 = parseFloat(_this.hdata[_this.iih].MAPE);
          mae5 = parseFloat(_this.hdata[_this.iih].MAE);
          rmse5 = parseFloat(_this.hdata[_this.iih].RMSE);
          _this.MAPE5 = mape5.toFixed(2);
          _this.MAE5 = mae5.toFixed(2);
          _this.RMSE5 = rmse5.toFixed(2);
          var mape6, mae6, rmse6;
          //console.log(iid,iie,iif,iig,iih);
          mape6 = parseFloat(_this.idata[_this.iii].MAPE);
          mae6 = parseFloat(_this.idata[_this.iii].MAE);
          rmse6 = parseFloat(_this.idata[_this.iii].RMSE);
          _this.MAPE6 = mape6.toFixed(2);
          _this.MAE6 = mae6.toFixed(2);
          _this.RMSE6 = rmse6.toFixed(2);
          (_this.iid) += 1;
          (_this.iid) %= (_this.lenDdata);
          (_this.iie) += 1;
          (_this.iie) %= (_this.lenEdata);
          (_this.iif) += 1;
          (_this.iif) %= (_this.lenFdata);
          (_this.iig) += 1;
          (_this.iig) %= (_this.lenGdata);
          (_this.iih) += 1;
          (_this.iih) %= (_this.lenHdata);
          (_this.iii) += 1;
          (_this.iii) %= (_this.lenIdata);
        },1000)
      },
      myEcharts(){
        let myEchart = this.$echarts.init(document.getElementById('index1'));
        let _this = this;
        this.haydnD = require("../data/prediction/RNN/ambient_temp_prediction_metrics.json");
        this.haydnE= require("../data/prediction/RNN/cpu_utilization_prediction_metrics.json");
        this.haydnF = require("../data/prediction/RNN/machine_temp_prediction_metrics.json");
        this.haydnG = require("../data/prediction/RNN/nyc_taxi_prediction_metrics.json");
        this.haydnH = require("../data/prediction/RNN/request_latency_prediction_metrics.json");
        this.haydnI = require("../data/prediction/RNN/request_count_prediction_metrics.json");

        this.arimaD = require("../data/prediction/ARIMA/ambient_temp_prediction_metrics.json");
        this.arimaE= require("../data/prediction/ARIMA/cpu_utilization_prediction_metrics.json");
        this.arimaF = require("../data/prediction/ARIMA/machine_temp_prediction_metrics.json");
        this.arimaG = require("../data/prediction/ARIMA/nyc_taxi_prediction_metrics.json");
        this.arimaH = require("../data/prediction/ARIMA/request_latency_prediction_metrics.json");
        this.arimaI = require("../data/prediction/ARIMA/request_count_prediction_metrics.json");

        this.tcnD = require("../data/prediction/TCN/ambient_temp_prediction_metrics.json");
        this.tcnE= require("../data/prediction/TCN/cpu_utilization_prediction_metrics.json");
        this.tcnF = require("../data/prediction/TCN/machine_temp_prediction_metrics.json");
        this.tcnG = require("../data/prediction/TCN/nyc_taxi_prediction_metrics.json");
        this.tcnH = require("../data/prediction/TCN/request_latency_prediction_metrics.json");
        this.tcnI = require("../data/prediction/TCN/request_count_prediction_metrics.json");

        this.rnnD = require("../data/prediction/HAYDN/ambient_temp_prediction_metrics.json");
        this.rnnE= require("../data/prediction/HAYDN/cpu_utilization_prediction_metrics.json");
        this.rnnF = require("../data/prediction/HAYDN/machine_temp_prediction_metrics.json");
        this.rnnG = require("../data/prediction/HAYDN/nyc_taxi_prediction_metrics.json");
        this.rnnH = require("../data/prediction/HAYDN/request_latency_prediction_metrics.json");
        this.rnnI = require("../data/prediction/HAYDN/request_count_prediction_metrics.json");
        this.timer = setInterval(function () {
          var option = {
            title: {
              text: '量化模型精度对比图',
              top: '3%',
              left: '20%',
              textStyle: {
                color: '#fff'
              }
            },
            legend: {
              data: ['鸿蒙玲珑核模型', 'INT8量化模型', 'INT4量化模型', 'FP16量化模型'],
              orient: 'horizontal',
              top: '80%',
              left: '10%',
              textStyle: {
                fontSize: 9,
                color: '#fff'
              }
            },
            radar: {
              // shape: 'circle',
              indicator: [
                {name: '模型量化耗时'},
                {name: '量化模型算力占用'},
                {name: '模型推理时延'},
                {name: '量化模型加载次数'},
                {name: '量化校准耗时'},
                {name: '模型推理吞吐量'}
              ],
              radius: 70,
              center: ['48%', '45%']
            },
            series: [{
              name: '量化模型精度对比图',
              type: 'radar',
              data: [
                {
                  value: [_this.haydnD[_this.haydnIndex].MAPE, _this.haydnE[_this.haydnIndex2].MAPE, _this.haydnF[_this.haydnIndex3].MAPE, _this.haydnG[_this.haydnIndex4].MAPE, _this.haydnH[_this.haydnIndex5].MAPE, _this.haydnI[_this.haydnIndex6].MAPE],
                  name: '鸿蒙玲珑核模型'
                },
                {
                  value: [_this.arimaD[_this.haydnIndex].MAPE, _this.arimaE[_this.haydnIndex].MAPE, _this.arimaF[_this.haydnIndex].MAPE, _this.arimaG[_this.haydnIndex].MAPE, _this.arimaH[_this.haydnIndex].MAPE, _this.arimaI[_this.haydnIndex].MAPE],
                  name: 'ARIMA差分时序模型'
                },
                {
                  value: [_this.tcnD[_this.haydnIndex].MAPE, _this.tcnE[_this.haydnIndex].MAPE, _this.tcnF[_this.haydnIndex].MAPE, _this.tcnG[_this.haydnIndex].MAPE, _this.tcnH[_this.haydnIndex].MAPE, _this.tcnI[_this.haydnIndex].MAPE],
                  name: '基于TCN的时序模型'
                },
                {
                  value: [_this.rnnD[_this.haydnIndex].MAPE, _this.rnnE[_this.haydnIndex].MAPE, _this.rnnF[_this.haydnIndex].MAPE, _this.rnnG[_this.haydnIndex].MAPE, _this.rnnH[_this.haydnIndex].MAPE, _this.rnnI[_this.haydnIndex].MAPE],
                  name: '基于RNN的时序模型'
                {
                  value: [_this.haydnD[_this.haydnIndex].MAPE, _this.haydnE[_this.haydnIndex2].MAPE, _this.haydnF[_this.haydnIndex3].MAPE, _this.haydnG[_this.haydnIndex4].MAPE, _this.haydnH[_this.haydnIndex5].MAPE, _this.haydnI[_this.haydnIndex6].MAPE],
                  name: '鸿蒙玲珑核模型'
                },
          myEchart.setOption(option);
          (_this.haydnIndex) += 1;
          (_this.haydnIndex) %= _this.getJsonLength(_this.haydnD);
          //console.log(_this.haydnIndex);
          (_this.haydnIndex2) += 1;
          (_this.haydnIndex2) %= _this.getJsonLength(_this.haydnE);
          (_this.haydnIndex3) += 1;
          (_this.haydnIndex3) %= _this.getJsonLength(_this.haydnF);
          (_this.haydnIndex4) += 1;
          (_this.haydnIndex4) %= _this.getJsonLength(_this.haydnG);
          (_this.haydnIndex5) += 1;
          (_this.haydnIndex5) %= _this.getJsonLength(_this.haydnH);
          (_this.haydnIndex6) += 1;
          (_this.haydnIndex6) %= _this.getJsonLength(_this.haydnI);
        },1000)
        window.addEventListener("resize",()=>{
          myEchart.resize();
        })
                {
                  value: [_this.arimaD[_this.haydnIndex].MAPE, _this.arimaE[_this.haydnIndex].MAPE, _this.arimaF[_this.haydnIndex].MAPE, _this.arimaG[_this.haydnIndex].MAPE, _this.arimaH[_this.haydnIndex].MAPE, _this.arimaI[_this.haydnIndex].MAPE],
                  name: 'INT8量化模型'
                },
      this.showMetrics();//})
      this.myEcharts();
      Notification.closeAll();
    },
    beforeDestroy() {
      if (this.timer) {
        clearInterval(this.timer); // 在Vue实例销毁前，清除我们的定时器
                {
                  value: [_this.tcnD[_this.haydnIndex].MAPE, _this.tcnE[_this.haydnIndex].MAPE, _this.tcnF[_this.haydnIndex].MAPE, _this.tcnG[_this.haydnIndex].MAPE, _this.tcnH[_this.haydnIndex].MAPE, _this.tcnI[_this.haydnIndex].MAPE],
                  name: 'INT4量化模型'
                },
        const route = this.$route
        const { meta, path } = route
        // if set path, the sidebar will highlight the path you set
        return meta.secondMenu
                {
                  value: [_this.rnnD[_this.haydnIndex].MAPE, _this.rnnE[_this.haydnIndex].MAPE, _this.rnnF[_this.haydnIndex].MAPE, _this.rnnG[_this.haydnIndex].MAPE, _this.rnnH[_this.haydnIndex].MAPE, _this.rnnI[_this.haydnIndex].MAPE],
                  name: 'FP16量化模型'
                }

<style scoped>
.el-carousel__item h3 {
  color: #475669;
  font-size: 18px;
  opacity: 0.75;
  line-height: 300px;
  margin: 0;
}

.el-carousel__item{
  width: 100%;
  height: 100%;
}

.el-carousel__item:nth-child(2n) {
  background-color: #1f1f1f;
}

.el-carousel__item:nth-child(2n+1) {
  background-color: #333333;
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
  }
  .el-radio{
    color: #eeeeee;
  }
  .Evaluation{
    position: fixed;
    left:0%;
    margin-top: 0%;
    font-size: 2vw;
    font-size: 2vh;
    font-weight: 700;
    line-height: 250%;
    height: 38%;
    padding:1%;
    border-radius: 10px;
    box-shadow: 0 2px 5px black;
    width: 11%;
  }
  .font{
    font-size: 2.5vw;
    font-size: 2.5vh;
    color: #409EFF;
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
  .el-radio.is-bordered{
    border-radius: 10px;
    box-shadow: 0 2px 5px black;
    border: #333333;
  }
  .charts{
    width: 80%;
    height: 50%;
    margin-top: 33%;
    margin-left: 2%;
    margin-right: 3%;
    border: #333333;
    border-radius: 10px;
    box-shadow: 0 2px 5px black
  }
.compare{
  width: 80%;
  margin-left: 10%;
  margin-right: 3%;
  border: #333333;
  border-radius: 10px;
  box-shadow: 0 2px 5px black;
}

/deep/  .el-radio__label{
    font-size: 1.7vh;
  padding-bottom: 1%;
  margin-left: 8%;
  }
/deep/ .el-radio.is-bordered {
  padding: 8px 20px 8px 10px;
}

</style>
