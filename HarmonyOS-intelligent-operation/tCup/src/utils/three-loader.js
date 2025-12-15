/**
 * Three.js 加载器 - 兼容 Vue 2 + Webpack 3
 * 解决 webpack 无法正确解析 three 模块的问题
 */

// 尝试多种导入方式
let THREE

try {
  // 方式1: ES6 import
  THREE = require('three')
} catch (e) {
  try {
    // 方式2: 直接引用
    THREE = require('three/build/three.min.js')
  } catch (e2) {
    console.error('无法加载 Three.js:', e2)
    // 提供一个空对象避免崩溃
    THREE = {}
  }
}

export default THREE

