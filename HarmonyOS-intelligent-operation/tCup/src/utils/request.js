import axios from 'axios';

const request = axios.create({
  baseURL: 'http://localhost:5000', // 后端API地址
  timeout: 5000,
  withCredentials: true // 允许跨域携带cookie
});

// 请求拦截器（可选）
request.interceptors.request.use(
  config => {
    // 可添加请求头，如token
    return config;
  },
  error => {
    return Promise.reject(error);
  }
);

// 响应拦截器（可选）
request.interceptors.response.use(
  response => {
    return response.data;
  },
  error => {
    // 统一处理错误
    if (error.response && error.response.status === 401) {
      // 未登录，跳转到登录页
      window.location.href = '/#/login';
    }
    return Promise.reject(error);
  }
);

export default request;