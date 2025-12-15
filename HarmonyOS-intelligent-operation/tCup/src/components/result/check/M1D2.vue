<template>
  <div>
    <el-table :data="tableData" style="width: 100%">
      <el-table-column prop="timestamp" label="异常点时间段" width="200"></el-table-column>
      <el-table-column prop="prediction" label="预测值"></el-table-column>
      <el-table-column prop="real_value" label="真实值"></el-table-column>
    </el-table>
  </div>
</template>

<script>
export default {
  name: 'M1D2',
  data() {
    return {
      tableData: []
    }
  },
  mounted() {
    // 仅调用后端API，无任何本地数据
    this.$axios.get('/api/anomaly_data')
      .then(res => {
        this.tableData = res.data.map(item => ({
          timestamp: item.timestamp,
          prediction: item.prediction,
          real_value: item.real_value
        }));
      })
      .catch(err => {
        this.tableData = [
          {timestamp: 'API请求失败', prediction: '请检查后端', real_value: err.message}
        ];
      });
  }
}
</script>