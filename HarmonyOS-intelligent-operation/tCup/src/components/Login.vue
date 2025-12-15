<template>
  <div class="login-container">
    <div class="login-card">
      <h2 class="login-title">系统监控平台 - 登录</h2>
      
      <form @submit.prevent="handleLogin" class="login-form">
        <div class="form-group">
          <label for="username" class="form-label">用户名</label>
          <input
            type="text"
            id="username"
            v-model="username"
            class="form-input"
            placeholder="请输入用户名"
            required
          >
        </div>
        
        <div class="form-group">
          <label for="password" class="form-label">密码</label>
          <input
            type="password"
            id="password"
            v-model="password"
            class="form-input"
            placeholder="请输入密码"
            required
          >
        </div>
        
        <div v-if="errorMessage" class="error-message">{{ errorMessage }}</div>
        
        <button type="submit" class="login-button">登录</button>
      </form>
    </div>
  </div>
</template>

<script>
import axios from 'axios';

export default {
  name: 'Login',
  data() {
    return {
      username: '',
      password: '',
      errorMessage: ''
    };
  },
  methods: {
    async handleLogin() {
      // 清空之前的错误消息
      this.errorMessage = '';
      
      try {
        // 发送登录请求到后端API
        const response = await axios.post(
          'http://localhost:5000/api/login',
          {
            username: this.username,
            password: this.password
          },
          {
            // 允许携带跨域cookie，用于会话保持
            withCredentials: true,
            headers: {
              'Content-Type': 'application/json'
            }
          }
        );
        
        // 登录成功处理
        if (response.data.status === 'success') {
          // 存储登录状态（实际项目中可使用vuex或更安全的存储方式）
          localStorage.setItem('isLogin', 'true');
          // 跳转到概览页面
          this.$router.push('/overview');
        } else {
          this.errorMessage = response.data.message || '登录失败，请重试';
        }
      } catch (error) {
        // 错误处理
        if (error.response) {
          // 服务器返回错误
          this.errorMessage = error.response.data.message || 
            `登录失败 (${error.response.status})`;
        } else if (error.request) {
          // 无响应
          this.errorMessage = '无法连接到服务器，请检查后端服务是否运行';
        } else {
          // 其他错误
          this.errorMessage = '登录过程出错: ' + error.message;
        }
      }
    }
  },
  mounted() {
    // 页面加载时检查是否已登录，如已登录直接跳转
    if (localStorage.getItem('isLogin')) {
      this.$router.push('/overview');
    }
  }
};
</script>

<style scoped>
.login-container {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 100vh;
  background-color: #f5f5f5;
  padding: 20px;
}

.login-card {
  background: white;
  padding: 30px;
  border-radius: 8px;
  box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
  width: 100%;
  max-width: 400px;
}

.login-title {
  text-align: center;
  color: #333;
  margin-bottom: 25px;
  font-size: 24px;
}

.form-group {
  margin-bottom: 20px;
}

.form-label {
  display: block;
  margin-bottom: 8px;
  color: #555;
  font-weight: 500;
}

.form-input {
  width: 100%;
  padding: 12px 15px;
  border: 1px solid #ddd;
  border-radius: 4px;
  font-size: 16px;
  transition: border-color 0.2s;
}

.form-input:focus {
  border-color: #42b983;
  outline: none;
}

.login-button {
  width: 100%;
  padding: 12px;
  background-color: #42b983;
  color: white;
  border: none;
  border-radius: 4px;
  font-size: 16px;
  cursor: pointer;
  transition: background-color 0.2s;
}

.login-button:hover {
  background-color: #359469;
}

.error-message {
  color: #e74c3c;
  margin-bottom: 15px;
  padding: 10px;
  background-color: #f8d7da;
  border-radius: 4px;
  text-align: center;
}
</style>