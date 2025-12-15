<template>
  <div class="config-manager">
    <div class="header">
      <h2>量化模型监控配置</h2>
      <el-button type="primary" @click="openAddDialog">
        <i class="el-icon-plus"></i> 添加配置
      </el-button>
    </div>

    <el-table :data="configs" border stripe style="width: 100%; margin-top: 20px;">
      <el-table-column prop="id" label="ID" width="80" align="center"></el-table-column>
      <el-table-column prop="name" label="配置名称"></el-table-column>
      <el-table-column prop="description" label="描述"></el-table-column>
      <el-table-column prop="threshold" label="阈值" align="center"></el-table-column>
      <el-table-column prop="created_at" label="创建时间" align="center"></el-table-column>
      <el-table-column label="操作" width="180" align="center">
        <template slot-scope="scope">
          <el-button @click="handleEdit(scope.row)" type="primary" size="small">编辑</el-button>
          <el-button @click="handleDelete(scope.row.id)" type="danger" size="small">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <el-dialog 
      :title="formTitle" 
      :visible.sync="dialogVisible" 
      width="500px"
      :close-on-click-modal="false"
    >
      <el-form :model="formData" :rules="formRules" ref="formRef" label-width="100px">
        <el-form-item label="配置名称" prop="name">
          <el-input v-model="formData.name" placeholder="请输入配置名称"></el-input>
        </el-form-item>
        <el-form-item label="描述" prop="description">
          <el-input 
            v-model="formData.description" 
            placeholder="请输入描述信息" 
            type="textarea" 
            rows="3"
          ></el-input>
        </el-form-item>
        <el-form-item label="阈值" prop="threshold">
          <el-input 
            v-model.number="formData.threshold" 
            placeholder="请输入阈值数值" 
            type="number"
            step="0.01"
          ></el-input>
        </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="submitForm">确定</el-button>
      </div>
    </el-dialog>
  </div>
</template>

<script>
export default {
  name: 'ConfigManager',
  data() {
    return {
      configs: [],
      dialogVisible: false,
      formTitle: '添加监控配置',
      formData: {
        id: null,
        name: '',
        description: '',
        threshold: null,
        created_at: ''
      },
      formRules: {
        name: [
          { required: true, message: '请输入配置名称', trigger: 'blur' },
          { min: 2, max: 50, message: '名称长度在2到50个字符之间', trigger: 'blur' }
        ],
        threshold: [
          { required: true, message: '请输入阈值', trigger: 'blur' },
          { type: 'number', message: '阈值必须是数字', trigger: 'blur' }
        ]
      }
    };
  },
  methods: {
    openAddDialog() {
      this.formTitle = '添加监控配置';
      this.formData = { id: null, name: '', description: '', threshold: null, created_at: '' };
      this.dialogVisible = true;
    },
    async fetchConfigs() {
      try {
        const res = await this.$http.get('/api/monitor-configs');
        this.configs = res;
      } catch (e) {
        this.$message.error('获取监控配置失败');
      }
    },
    handleEdit(row) {
      this.formTitle = '编辑监控配置';
      this.formData = { ...row };
      this.dialogVisible = true;
    },
    handleDelete(id) {
      this.$confirm('确定要删除该配置吗?', '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }).then(async () => {
        await this.$http({
          url: '/api/monitor-configs',
          method: 'DELETE',
          data: { id }
        });
        this.$message.success('删除成功');
        this.fetchConfigs();
      }).catch(() => {
        this.$message.info('已取消删除');
      });
    },
    submitForm() {
      this.$refs.formRef.validate(async (valid) => {
        if (valid) {
          if (this.formData.id) {
            await this.$http({
              url: '/api/monitor-configs',
              method: 'PUT',
              data: this.formData
            });
            this.$message.success('更新成功');
          } else {
            await this.$http({
              url: '/api/monitor-configs',
              method: 'POST',
              data: this.formData
            });
            this.$message.success('添加成功');
          }
          this.dialogVisible = false;
          this.formData = { id: null, name: '', description: '', threshold: null };
          this.fetchConfigs();
        }
      });
    }
  },
  mounted() {
    this.fetchConfigs();
  }
};
</script>

<style scoped>
.config-manager {
  max-width: 1200px;
  margin: 40px auto 0 auto;
  background: #fff;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0,0,0,0.08);
  padding: 32px 32px 40px 32px;
}
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}
.el-table {
  background-color: #fff;
  border-radius: 8px;
  overflow: hidden;
}
.el-table th, .el-table td {
  font-size: 15px;
}
.el-button {
  min-width: 70px;
}
.dialog-footer {
  text-align: right;
}
</style>