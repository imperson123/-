<template>
  <div class="metaverse-container" ref="container">
    <div class="overlay-content">
      <slot></slot>
    </div>
  </div>
</template>

<script>
// Three.js 导入 - 使用 require 方式（兼容 Webpack 3）
const THREE = require('three')

export default {
  name: 'MetaverseScene',
  data() {
    return {
      scene: null,
      camera: null,
      renderer: null,
      animationId: null,
      particles: null,
      cubes: []
    }
  },
  mounted() {
    this.initScene()
    this.animate()
    window.addEventListener('resize', this.onWindowResize)
  },
  beforeDestroy() {
    if (this.animationId) {
      cancelAnimationFrame(this.animationId)
    }
    window.removeEventListener('resize', this.onWindowResize)
    if (this.renderer) {
      this.renderer.dispose()
    }
  },
  methods: {
    initScene() {
      const container = this.$refs.container
      const width = container.clientWidth
      const height = container.clientHeight

      // 创建场景
      this.scene = new THREE.Scene()
      this.scene.background = new THREE.Color(0x0a0a0a)
      this.scene.fog = new THREE.Fog(0x0a0a0a, 10, 50)

      // 创建相机
      this.camera = new THREE.PerspectiveCamera(75, width / height, 0.1, 1000)
      this.camera.position.set(0, 5, 10)

      // 创建渲染器
      this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true })
      this.renderer.setSize(width, height)
      this.renderer.setPixelRatio(window.devicePixelRatio)
      container.appendChild(this.renderer.domElement)

      // 添加增强的光源系统（增强沉浸感）
      // 环境光
      const ambientLight = new THREE.AmbientLight(0x12f0e0, 0.4)
      this.scene.add(ambientLight)

      // 主方向光
      const directionalLight = new THREE.DirectionalLight(0x12f0e0, 0.9)
      directionalLight.position.set(5, 8, 5)
      directionalLight.castShadow = true
      this.scene.add(directionalLight)

      // 顶部点光源（模拟天花板灯）
      const ceilingLight = new THREE.PointLight(0x12f0e0, 1.2, 50)
      ceilingLight.position.set(0, 4.5, 0)
      this.scene.add(ceilingLight)

      // 底部点光源（模拟地面反射）
      const floorLight = new THREE.PointLight(0x12f0e0, 0.6, 30)
      floorLight.position.set(0, -4.5, 0)
      this.scene.add(floorLight)

      // 侧边补光
      const sideLight1 = new THREE.PointLight(0x12f0e0, 0.5, 40)
      sideLight1.position.set(-10, 0, 0)
      this.scene.add(sideLight1)

      const sideLight2 = new THREE.PointLight(0x12f0e0, 0.5, 40)
      sideLight2.position.set(10, 0, 0)
      this.scene.add(sideLight2)

      // 创建粒子系统
      this.createParticles()

      // 创建浮动立方体
      this.createFloatingCubes()

      // 创建地面和天花板
      this.createFloor()
      this.createCeiling()
      
      // 创建网格地面（辅助线）
      this.createGrid()
    },
    createParticles() {
      const particleCount = 1000
      const particles = new THREE.BufferGeometry()
      const positions = new Float32Array(particleCount * 3)

      for (let i = 0; i < particleCount * 3; i += 3) {
        positions[i] = (Math.random() - 0.5) * 100
        positions[i + 1] = (Math.random() - 0.5) * 100
        positions[i + 2] = (Math.random() - 0.5) * 100
      }

      particles.setAttribute('position', new THREE.BufferAttribute(positions, 3))

      const particleMaterial = new THREE.PointsMaterial({
        color: 0x12f0e0,
        size: 0.1,
        transparent: true,
        opacity: 0.6
      })

      this.particles = new THREE.Points(particles, particleMaterial)
      this.scene.add(this.particles)
    },
    createFloatingCubes() {
      const cubeCount = 20
      const geometry = new THREE.BoxGeometry(0.5, 0.5, 0.5)
      const material = new THREE.MeshPhongMaterial({
        color: 0x12f0e0,
        transparent: true,
        opacity: 0.7,
        emissive: 0x12f0e0,
        emissiveIntensity: 0.5
      })

      for (let i = 0; i < cubeCount; i++) {
        const cube = new THREE.Mesh(geometry, material)
        cube.position.set(
          (Math.random() - 0.5) * 20,
          (Math.random() - 0.5) * 20,
          (Math.random() - 0.5) * 20
        )
        cube.rotation.set(
          Math.random() * Math.PI,
          Math.random() * Math.PI,
          Math.random() * Math.PI
        )
        cube.userData = {
          speed: {
            x: (Math.random() - 0.5) * 0.02,
            y: (Math.random() - 0.5) * 0.02,
            z: (Math.random() - 0.5) * 0.02
          },
          rotation: {
            x: (Math.random() - 0.5) * 0.02,
            y: (Math.random() - 0.5) * 0.02,
            z: (Math.random() - 0.5) * 0.02
          }
        }
        this.cubes.push(cube)
        this.scene.add(cube)
      }
    },
    createFloor() {
      // 创建地面
      const floorGeometry = new THREE.PlaneGeometry(100, 100)
      const floorMaterial = new THREE.MeshPhongMaterial({
        color: 0x0a0a0a,
        transparent: true,
        opacity: 0.8,
        emissive: 0x12f0e0,
        emissiveIntensity: 0.1,
        side: THREE.DoubleSide
      })
      const floor = new THREE.Mesh(floorGeometry, floorMaterial)
      floor.rotation.x = -Math.PI / 2
      floor.position.y = -5
      floor.receiveShadow = true
      this.scene.add(floor)
      
      // 地面网格线
      const floorGrid = new THREE.GridHelper(100, 50, 0x12f0e0, 0x12f0e0)
      floorGrid.material.opacity = 0.3
      floorGrid.material.transparent = true
      floorGrid.position.y = -4.99
      this.scene.add(floorGrid)
    },
    createCeiling() {
      // 创建天花板
      const ceilingGeometry = new THREE.PlaneGeometry(100, 100)
      const ceilingMaterial = new THREE.MeshPhongMaterial({
        color: 0x0a0a0a,
        transparent: true,
        opacity: 0.6,
        emissive: 0x12f0e0,
        emissiveIntensity: 0.05,
        side: THREE.DoubleSide
      })
      const ceiling = new THREE.Mesh(ceilingGeometry, ceilingMaterial)
      ceiling.rotation.x = Math.PI / 2
      ceiling.position.y = 5
      this.scene.add(ceiling)
      
      // 天花板装饰网格
      const ceilingGrid = new THREE.GridHelper(100, 20, 0x12f0e0, 0x12f0e0)
      ceilingGrid.material.opacity = 0.15
      ceilingGrid.material.transparent = true
      ceilingGrid.position.y = 4.99
      this.scene.add(ceilingGrid)
    },
    createGrid() {
      // 保留原有的网格辅助线（可选，用于调试）
      const gridHelper = new THREE.GridHelper(50, 50, 0x12f0e0, 0x12f0e0)
      gridHelper.material.opacity = 0.1
      gridHelper.material.transparent = true
      gridHelper.position.y = -4.98
      this.scene.add(gridHelper)
    },
    animate() {
      this.animationId = requestAnimationFrame(this.animate)

      // 旋转粒子
      if (this.particles) {
        this.particles.rotation.y += 0.001
      }

      // 更新立方体（限制在房间内）
      this.cubes.forEach(cube => {
        cube.position.x += cube.userData.speed.x
        cube.position.y += cube.userData.speed.y
        cube.position.z += cube.userData.speed.z

        cube.rotation.x += cube.userData.rotation.x
        cube.rotation.y += cube.userData.rotation.y
        cube.rotation.z += cube.userData.rotation.z

        // 边界反弹（限制在地面和天花板之间）
        if (Math.abs(cube.position.x) > 8) cube.userData.speed.x *= -1
        if (cube.position.y > 4 || cube.position.y < -4) cube.userData.speed.y *= -1
        if (Math.abs(cube.position.z) > 8) cube.userData.speed.z *= -1
      })

      // 相机轻微移动（保持在房间内）
      const time = Date.now() * 0.0005
      this.camera.position.x = Math.sin(time) * 2
      this.camera.position.y = 0 // 保持在中间高度
      this.camera.position.z = 10 + Math.cos(time) * 2
      this.camera.lookAt(0, 0, 0)

      this.renderer.render(this.scene, this.camera)
    },
    onWindowResize() {
      const container = this.$refs.container
      const width = container.clientWidth
      const height = container.clientHeight

      this.camera.aspect = width / height
      this.camera.updateProjectionMatrix()
      this.renderer.setSize(width, height)
    }
  }
}
</script>

<style scoped>
.metaverse-container {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 0;
  overflow: hidden;
}

.overlay-content {
  position: relative;
  z-index: 10;
  width: 100%;
  height: 100%;
  pointer-events: none;
}

.overlay-content >>> * {
  pointer-events: auto;
}
</style>

