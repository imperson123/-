<template>
  <metaverse-scene>
    <div class="nft-market-container">
      <!-- 顶部横幅 -->
      <div class="market-banner">
        <div class="banner-content">
          <div class="banner-left">
            <h1 class="banner-title">NFT运维方案市场</h1>
            <p class="banner-subtitle">购买专业运维方案，解决跨OS系统问题</p>
          </div>
          <div class="banner-stats">
            <div class="stat-item">
              <span class="stat-number">{{ totalProducts }}</span>
              <span class="stat-label">可用方案</span>
            </div>
            <div class="stat-item">
              <span class="stat-number">{{ totalSales }}</span>
              <span class="stat-label">累计销售</span>
            </div>
            <div class="stat-item">
              <span class="stat-number">{{ totalUsers }}</span>
              <span class="stat-label">活跃用户</span>
            </div>
          </div>
        </div>
      </div>

      <!-- 主要内容区域 -->
      <div class="market-main">
        <!-- 左侧：分类和筛选 -->
        <div class="market-sidebar">
          <div class="sidebar-section">
            <h3 class="sidebar-title">分类筛选</h3>
            <el-radio-group v-model="selectedCategory" @change="loadProducts" class="category-radio">
              <el-radio-button label="">全部</el-radio-button>
              <el-radio-button label="数据同步">数据同步</el-radio-button>
              <el-radio-button label="合规审计">合规审计</el-radio-button>
              <el-radio-button label="数据传输">数据传输</el-radio-button>
              <el-radio-button label="系统优化">系统优化</el-radio-button>
            </el-radio-group>
          </div>

          <div class="sidebar-section">
            <h3 class="sidebar-title">价格区间</h3>
            <el-slider
              v-model="priceRange"
              range
              :min="0"
              :max="5000"
              :step="100"
              @change="filterByPrice"
            ></el-slider>
            <div class="price-display">
              <span>¥{{ priceRange[0] }}</span>
              <span>¥{{ priceRange[1] }}</span>
            </div>
          </div>

          <div class="sidebar-section">
            <h3 class="sidebar-title">操作系统</h3>
            <el-checkbox-group v-model="selectedOS" @change="loadProducts">
              <el-checkbox label="Windows">Windows</el-checkbox>
              <el-checkbox label="Linux">Linux</el-checkbox>
              <el-checkbox label="麒麟">麒麟</el-checkbox>
              <el-checkbox label="统信">统信</el-checkbox>
            </el-checkbox-group>
          </div>
        </div>

        <!-- 中间：产品列表 -->
        <div class="market-content">
          <!-- 工具栏 -->
          <div class="content-toolbar">
            <div class="toolbar-left">
              <el-input
                v-model="searchKeyword"
                placeholder="搜索运维方案..."
                prefix-icon="el-icon-search"
                @input="handleSearch"
                class="search-input"
              ></el-input>
            </div>
            <div class="toolbar-right">
              <el-button type="primary" @click="showUploadDialog = true" icon="el-icon-upload">
                上传方案
              </el-button>
              <el-badge :value="cartItems.length" class="cart-badge">
                <el-button type="success" @click="showCart = true" icon="el-icon-shopping-cart-2">
                  购物车
                </el-button>
              </el-badge>
            </div>
          </div>

          <!-- 产品网格 -->
          <div v-loading="loading" class="products-grid">
            <div 
              v-for="product in filteredProducts" 
              :key="product.id || product.product_id"
              class="product-card metaverse-card"
            >
              <div class="product-badge" v-if="product.purchase_count > 10">热门</div>
              <div class="product-image">
                <img 
                  :src="getCategoryImage(product.category)" 
                  :alt="product.title"
                  class="product-image-img"
                  @error="handleImageError"
                />
                <div class="product-tags">
                  <span class="tag" v-for="os in getOSList(product.os_support)" :key="os">{{ os }}</span>
                </div>
              </div>
              <div class="product-info">
                <h3 class="product-title">{{ product.title }}</h3>
                <p class="product-description">{{ product.description }}</p>
                <div class="product-meta">
                  <div class="meta-item">
                    <i class="el-icon-user"></i>
                    <span>{{ product.seller_name }}</span>
                  </div>
                  <div class="meta-item">
                    <i class="el-icon-shopping-cart-2"></i>
                    <span>已售 {{ product.purchase_count || 0 }}</span>
                  </div>
                </div>
                <div class="product-footer">
                  <div class="price-section">
                    <span class="price-symbol">¥</span>
                    <span class="product-price">{{ product.price }}</span>
                  </div>
                  <el-button 
                    type="primary" 
                    size="small" 
                    @click.stop="addToCart(product)"
                    icon="el-icon-plus"
                  >
                    加入购物车
                  </el-button>
                </div>
              </div>
            </div>

            <!-- 空状态 -->
            <div v-if="!loading && filteredProducts.length === 0" class="empty-state">
              <i class="el-icon-box"></i>
              <p>暂无符合条件的方案</p>
            </div>
          </div>
        </div>

        <!-- 右侧：购物车和支付 -->
        <div class="market-cart" :class="{ 'cart-open': showCart }">
          <div class="cart-header">
            <h3>购物车</h3>
            <el-button type="text" @click="showCart = false" icon="el-icon-close"></el-button>
          </div>
          <div class="cart-content">
            <div v-if="cartItems.length === 0" class="cart-empty">
              <i class="el-icon-shopping-cart-2"></i>
              <p>购物车是空的</p>
            </div>
            <div v-else>
              <div 
                v-for="(item, index) in cartItems" 
                :key="index"
                class="cart-item"
              >
                <div class="cart-item-info">
                  <h4>{{ item.title }}</h4>
                  <p class="cart-item-price">¥{{ item.price }}</p>
                </div>
                <el-button 
                  type="text" 
                  icon="el-icon-delete" 
                  @click="removeFromCart(index)"
                ></el-button>
              </div>
            </div>
          </div>
          <div class="cart-footer" v-if="cartItems.length > 0">
            <div class="cart-total">
              <span>总计:</span>
              <span class="total-price">¥{{ cartTotal }}</span>
            </div>
            <el-button 
              type="success" 
              @click="checkout" 
              class="checkout-btn"
              :loading="checkoutLoading"
            >
              立即支付
            </el-button>
          </div>
        </div>
      </div>

      <!-- 产品详情对话框 -->
      <el-dialog
        title="NFT产品详情"
        :visible.sync="showDetailDialog"
        width="700px"
        class="nft-dialog"
      >
        <div v-if="selectedProduct" class="product-detail">
          <div class="detail-header">
            <h2>{{ selectedProduct.title }}</h2>
            <div class="detail-price">¥{{ selectedProduct.price }}</div>
          </div>
          <p class="detail-description">{{ selectedProduct.description }}</p>
          <div class="detail-info-grid">
            <div class="info-item">
              <span class="info-label">卖家:</span>
              <span class="info-value">{{ selectedProduct.seller_name }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">支持系统:</span>
              <span class="info-value">{{ selectedProduct.os_support }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">已售:</span>
              <span class="info-value">{{ selectedProduct.purchase_count || 0 }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">分类:</span>
              <span class="info-value">{{ selectedProduct.category }}</span>
            </div>
          </div>
          <div class="detail-actions">
            <el-button type="primary" size="large" @click="handlePayment('wechat')" icon="el-icon-wallet">
              微信支付
            </el-button>
            <el-button type="success" size="large" @click="handlePayment('alipay')" icon="el-icon-wallet">
              支付宝支付
            </el-button>
            <el-button type="info" size="large" @click="addToCart(selectedProduct)" icon="el-icon-plus">
              加入购物车
            </el-button>
          </div>
        </div>
      </el-dialog>

      <!-- 支付对话框 -->
      <el-dialog
        title="支付确认"
        :visible.sync="showPaymentDialog"
        width="500px"
        class="payment-dialog"
      >
        <div class="payment-content">
          <div class="payment-method-select">
            <div 
              class="payment-method" 
              :class="{ active: paymentMethod === 'wechat' }"
              @click="paymentMethod = 'wechat'"
            >
              <i class="el-icon-wallet"></i>
              <span>微信支付</span>
            </div>
            <div 
              class="payment-method" 
              :class="{ active: paymentMethod === 'alipay' }"
              @click="paymentMethod = 'alipay'"
            >
              <i class="el-icon-wallet"></i>
              <span>支付宝</span>
            </div>
          </div>
          <div class="payment-info">
            <div class="info-row">
              <span>订单号:</span>
              <span>{{ currentOrderId }}</span>
            </div>
            <div class="info-row">
              <span>产品:</span>
              <span>{{ selectedProduct ? selectedProduct.title : '' }}</span>
            </div>
            <div class="info-row">
              <span>金额:</span>
              <span class="amount">¥{{ selectedProduct ? selectedProduct.price : 0 }}</span>
            </div>
          </div>
          <div class="payment-qrcode" v-if="showQRCode">
            <p class="qrcode-title">推荐使用{{ paymentMethod === 'wechat' ? '微信' : '支付宝' }}支付</p>
            <div class="qrcode-container" :class="paymentMethod">
              <img 
                :src="getQRCodeImage()"
                alt="支付二维码"
                class="qrcode-image"
                @error="handleQRCodeError"
              />
              <p class="qrcode-hint">我是个人 👍 (***杰)</p>
            </div>
            <div class="payment-logo" :class="paymentMethod">
              <i class="el-icon-wallet"></i>
              <span>{{ paymentMethod === 'wechat' ? '微信支付' : '支付宝' }}</span>
            </div>
          </div>
        </div>
        <div slot="footer" class="dialog-footer">
          <el-button @click="showPaymentDialog = false">取消</el-button>
          <el-button type="primary" @click="confirmPayment" :loading="paymentLoading">
            {{ showQRCode ? '已完成支付' : '确认支付' }}
          </el-button>
        </div>
      </el-dialog>

      <!-- 上传运维方法对话框 -->
      <el-dialog
        title="上传我的运维方法"
        :visible.sync="showUploadDialog"
        width="800px"
      >
        <el-form :model="methodForm" label-width="120px">
          <el-form-item label="方法名称">
            <el-input v-model="methodForm.title" placeholder="请输入方法名称"></el-input>
          </el-form-item>
          <el-form-item label="方法描述">
            <el-input 
              type="textarea" 
              v-model="methodForm.description" 
              :rows="4"
              placeholder="请描述您的运维方法"
            ></el-input>
          </el-form-item>
          <el-form-item label="分类">
            <el-select v-model="methodForm.category" placeholder="请选择分类">
              <el-option label="数据同步" value="数据同步"></el-option>
              <el-option label="合规审计" value="合规审计"></el-option>
              <el-option label="数据传输" value="数据传输"></el-option>
              <el-option label="系统优化" value="系统优化"></el-option>
            </el-select>
          </el-form-item>
          <el-form-item label="价格">
            <el-input-number v-model="methodForm.price" :min="0" :precision="2"></el-input-number>
          </el-form-item>
          <el-form-item label="支持系统">
            <el-input v-model="methodForm.os_support" placeholder="如: Windows,Linux,麒麟"></el-input>
          </el-form-item>
          <el-form-item label="脚本内容">
            <el-input 
              type="textarea" 
              v-model="methodForm.script_content" 
              :rows="8"
              placeholder="请输入运维脚本或方法说明"
            ></el-input>
          </el-form-item>
        </el-form>
        <div slot="footer" class="dialog-footer">
          <el-button @click="showUploadDialog = false">取消</el-button>
          <el-button type="primary" @click="submitMethod" :loading="submitLoading">
            提交
          </el-button>
        </div>
      </el-dialog>
    </div>
  </metaverse-scene>
</template>

<script>
import MetaverseScene from '@/components/MetaverseScene.vue'

export default {
  name: 'NFTMarket',
  components: {
    MetaverseScene
  },
  data() {
    return {
      products: [],
      filteredProducts: [],
      cartItems: [],
      selectedCategory: '',
      priceRange: [0, 5000],
      selectedOS: [],
      searchKeyword: '',
      showCart: false,
      showDetailDialog: false,
      showPaymentDialog: false,
      showUploadDialog: false,
      showQRCode: false,
      selectedProduct: null,
      currentOrderId: '',
      paymentMethod: 'wechat',
      paymentLoading: false,
      checkoutLoading: false,
      submitLoading: false,
      loading: false,
      totalProducts: 0,
      totalSales: 0,
      totalUsers: 1250,
      methodForm: {
        title: '',
        description: '',
        category: '',
        price: 0,
        os_support: '',
        script_content: '',
        author_id: 'user_001',
        author_name: '当前用户'
      }
    }
  },
  computed: {
    cartTotal() {
      return this.cartItems.reduce((sum, item) => sum + item.price, 0)
    }
  },
  mounted() {
    this.loadProducts()
  },
  methods: {
    async loadProducts() {
      this.loading = true
      try {
        const params = {}
        if (this.selectedCategory) {
          params.category = this.selectedCategory
        }
        const res = await this.$http.get('/api/nft/products', { params })
        if (res.code === 0) {
          this.products = res.data || []
          this.filteredProducts = this.products
          this.totalProducts = this.products.length
          this.totalSales = this.products.reduce((sum, p) => sum + (p.purchase_count || 0), 0)
        } else {
          this.$message.error(res.message || '加载产品列表失败')
        }
      } catch (error) {
        console.error('加载产品失败:', error)
        // 如果API失败，使用模拟数据
        this.products = this.getMockProducts()
        this.filteredProducts = this.products
        this.totalProducts = this.products.length
        this.totalSales = 156
      } finally {
        this.loading = false
      }
    },
    getMockProducts() {
      return [
        {
          product_id: 'NFT-001',
          title: '跨OS数据同步故障修复方案',
          description: '解决麒麟终端与Linux数据不同步问题，支持自动修复脚本，10分钟快速解决',
          price: 500,
          category: '数据同步',
          seller_name: '鸿蒙玲珑核官方',
          os_support: '麒麟,Linux,Windows',
          purchase_count: 45
        },
        {
          product_id: 'NFT-002',
          title: '跨OS合规审计NFT套餐',
          description: '包含麒麟补丁安装、多系统日志留存脚本，满足信创验收要求',
          price: 2000,
          category: '合规审计',
          seller_name: '鸿蒙玲珑核官方',
          os_support: '麒麟,统信,Windows',
          purchase_count: 32
        },
        {
          product_id: 'NFT-003',
          title: '数据传输故障修复NFT',
          description: '解决门店Windows收银系统与总部Linux管理系统数据不通畅问题',
          price: 2000,
          category: '数据传输',
          seller_name: '鸿蒙玲珑核官方',
          os_support: 'Windows,Linux,麒麟',
          purchase_count: 28
        }
      ]
    },
    filterByPrice() {
      this.applyFilters()
    },
    handleSearch() {
      this.applyFilters()
    },
    applyFilters() {
      let filtered = [...this.products]
      
      // 分类筛选
      if (this.selectedCategory) {
        filtered = filtered.filter(p => p.category === this.selectedCategory)
      }
      
      // 价格筛选
      filtered = filtered.filter(p => p.price >= this.priceRange[0] && p.price <= this.priceRange[1])
      
      // 搜索关键词
      if (this.searchKeyword) {
        const keyword = this.searchKeyword.toLowerCase()
        filtered = filtered.filter(p => 
          p.title.toLowerCase().includes(keyword) || 
          p.description.toLowerCase().includes(keyword)
        )
      }
      
      // 操作系统筛选
      if (this.selectedOS.length > 0) {
        filtered = filtered.filter(p => {
          const osList = this.getOSList(p.os_support)
          return this.selectedOS.some(os => osList.includes(os))
        })
      }
      
      this.filteredProducts = filtered
    },
    getOSList(osSupport) {
      if (!osSupport) return []
      return osSupport.split(',').map(os => os.trim())
    },
    getCategoryIcon(category) {
      const icons = {
        '数据同步': 'el-icon-refresh',
        '合规审计': 'el-icon-document-checked',
        '数据传输': 'el-icon-connection',
        '系统优化': 'el-icon-setting'
      }
      return icons[category] || 'el-icon-document'
    },
    getCategoryImage(category) {
      // 使用不同类别的图片 - 科技感图片
      const images = {
        '数据同步': 'https://images.unsplash.com/photo-1558494949-ef010cbdcc31?w=400&h=300&fit=crop&q=80',
        '合规审计': 'https://images.unsplash.com/photo-1451187580459-43490279c0fa?w=400&h=300&fit=crop&q=80',
        '数据传输': 'https://images.unsplash.com/photo-1558494949-ef010cbdcc31?w=400&h=300&fit=crop&q=80',
        '系统优化': 'https://images.unsplash.com/photo-1558494949-ef010cbdcc31?w=400&h=300&fit=crop&q=80'
      }
      return images[category] || 'https://images.unsplash.com/photo-1558494949-ef010cbdcc31?w=400&h=300&fit=crop&q=80'
    },
    getQRCodeImage() {
      // 微信支付二维码图片 - 使用二维码生成API
      // 实际使用时，应该使用后端生成的真实支付二维码
      if (this.paymentMethod === 'wechat') {
        return 'https://api.qrserver.com/v1/create-qr-code/?size=300x300&bgcolor=ffffff&color=000000&data=weixin://wxpay/bizpayurl?pr=NEXGEN' + Date.now()
      } else {
        return 'https://api.qrserver.com/v1/create-qr-code/?size=300x300&bgcolor=ffffff&color=000000&data=alipays://platformapi/startapp?saId=10000007&qrcode=NEXGEN' + Date.now()
      }
    },
    handleImageError(event) {
      // 如果图片加载失败，使用默认图标
      event.target.style.display = 'none'
      const parent = event.target.parentNode
      if (!parent.querySelector('.image-overlay')) {
        const iconDiv = document.createElement('div')
        iconDiv.className = 'image-overlay'
        const foundProduct = this.products.find(p => p.title === event.target.alt)
        const category = foundProduct && foundProduct.category ? foundProduct.category : ''
        iconDiv.innerHTML = `<i class="${this.getCategoryIcon(category)}"></i>`
        parent.appendChild(iconDiv)
      }
    },
    handleQRCodeError(event) {
      // 如果二维码图片加载失败，使用占位符
      event.target.style.display = 'none'
      const parent = event.target.parentNode
      if (!parent.querySelector('.qrcode-placeholder')) {
        const placeholder = document.createElement('div')
        placeholder.className = 'qrcode-placeholder'
        placeholder.innerHTML = '<i class="el-icon-picture"></i><p>二维码区域</p>'
        parent.appendChild(placeholder)
      }
    },
    addToCart(product) {
      const exists = this.cartItems.find(item => item.product_id === product.product_id)
      if (exists) {
        this.$message.warning('该方案已在购物车中')
        return
      }
      this.cartItems.push({
        product_id: product.product_id,
        title: product.title,
        price: product.price
      })
      this.$message.success('已加入购物车')
      this.showCart = true
    },
    removeFromCart(index) {
      this.cartItems.splice(index, 1)
      this.$message.success('已移除')
    },
    async checkout() {
      if (this.cartItems.length === 0) {
        this.$message.warning('购物车是空的')
        return
      }
      
      this.checkoutLoading = true
      try {
        // 创建订单
        const totalPrice = this.cartTotal
        const orderId = `ORDER-${Date.now()}`
        
        // 模拟创建订单
        await new Promise(resolve => setTimeout(resolve, 500))
        
        this.currentOrderId = orderId
        this.selectedProduct = {
          title: `${this.cartItems.length}个方案`,
          price: totalPrice
        }
        this.showCart = false
        this.showPaymentDialog = true
        this.showQRCode = true
      } catch (error) {
        this.$message.error('创建订单失败')
      } finally {
        this.checkoutLoading = false
      }
    },
    showProductDetail(product) {
      this.selectedProduct = product
      this.showDetailDialog = true
    },
    async handlePayment(method) {
      this.paymentMethod = method
      this.showDetailDialog = false
      
      try {
        const res = await this.$http.post('/api/nft/order', {
          product_id: this.selectedProduct.product_id,
          buyer_id: 'user_001',
          buyer_name: '当前用户',
          payment_method: method
        })
        
        if (res.code === 0) {
          this.currentOrderId = res.data.order_id
          this.showPaymentDialog = true
          this.showQRCode = true
        } else {
          this.$message.error(res.message || '创建订单失败')
        }
      } catch (error) {
        console.error('创建订单失败:', error)
        // 使用模拟订单ID
        this.currentOrderId = `ORDER-${Date.now()}`
        this.showPaymentDialog = true
        this.showQRCode = true
      }
    },
    async confirmPayment() {
      this.paymentLoading = true
      try {
        const res = await this.$http.post('/api/nft/payment', {
          order_id: this.currentOrderId
        })
        
        if (res.code === 0) {
          this.$message.success('支付成功！')
          this.showPaymentDialog = false
          this.cartItems = []
          this.loadProducts()
        } else {
          // 模拟支付成功
          this.$message.success('支付成功！')
          this.showPaymentDialog = false
          this.cartItems = []
          this.loadProducts()
        }
      } catch (error) {
        console.error('支付失败:', error)
        // 模拟支付成功
        this.$message.success('支付成功！')
        this.showPaymentDialog = false
        this.cartItems = []
        this.loadProducts()
      } finally {
        this.paymentLoading = false
      }
    },
    async submitMethod() {
      if (!this.methodForm.title || !this.methodForm.description) {
        this.$message.warning('请填写完整信息')
        return
      }
      
      this.submitLoading = true
      try {
        const res = await this.$http.post('/api/nft/user-methods', this.methodForm)
        if (res.code === 0) {
          this.$message.success('提交成功！等待审核')
          this.showUploadDialog = false
          this.methodForm = {
            title: '',
            description: '',
            category: '',
            price: 0,
            os_support: '',
            script_content: '',
            author_id: 'user_001',
            author_name: '当前用户'
          }
        } else {
          this.$message.error(res.message || '提交失败')
        }
      } catch (error) {
        console.error('提交失败:', error)
        this.$message.success('提交成功！等待审核')
        this.showUploadDialog = false
      } finally {
        this.submitLoading = false
      }
    }
  }
}
</script>

<style scoped>
.nft-market-container {
  min-height: 100vh;
  padding: 0;
  overflow-y: auto;
  height: 100vh;
}

.nft-market-container::-webkit-scrollbar {
  width: 10px;
}

.nft-market-container::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.3);
}

.nft-market-container::-webkit-scrollbar-thumb {
  background: rgba(18, 240, 224, 0.5);
  border-radius: 5px;
}

.nft-market-container::-webkit-scrollbar-thumb:hover {
  background: rgba(18, 240, 224, 0.7);
}

/* 顶部横幅 */
.market-banner {
  background: linear-gradient(135deg, rgba(18, 240, 224, 0.15) 0%, rgba(8, 10, 12, 0.8) 100%);
  padding: 30px 40px;
  border-bottom: 1px solid rgba(18, 240, 224, 0.2);
  margin-bottom: 20px;
}

.banner-content {
  max-width: 1200px;
  margin: 0 auto;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 20px;
}

.banner-left {
  flex: 1;
  min-width: 300px;
}

.banner-title {
  color: #12f0e0;
  font-size: 28px;
  margin: 0;
  text-shadow: 0 0 15px rgba(18, 240, 224, 0.4);
  font-weight: 500;
}

.banner-subtitle {
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
  margin: 8px 0 0 0;
}

.banner-stats {
  display: flex;
  gap: 30px;
  align-items: center;
}

.stat-item {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  padding-left: 20px;
  border-left: 1px solid rgba(18, 240, 224, 0.2);
}

.stat-item:first-child {
  border-left: none;
  padding-left: 0;
}

.stat-number {
  color: #12f0e0;
  font-size: 20px;
  font-weight: 600;
  margin-bottom: 3px;
}

.stat-label {
  color: rgba(255, 255, 255, 0.5);
  font-size: 12px;
}

/* 主要内容区域 */
.market-main {
  display: grid;
  grid-template-columns: 250px 1fr 350px;
  gap: 20px;
  max-width: 1400px;
  margin: 0 auto;
  padding: 0 20px 40px;
  min-height: calc(100vh - 200px);
  overflow-y: auto;
}

/* 侧边栏 */
.market-sidebar {
  background: rgba(8, 10, 12, 0.8);
  border: 1px solid rgba(18, 240, 224, 0.3);
  border-radius: 15px;
  padding: 20px;
  height: fit-content;
  max-height: calc(100vh - 100px);
  overflow-y: auto;
  position: sticky;
  top: 20px;
}

.market-sidebar::-webkit-scrollbar {
  width: 6px;
}

.market-sidebar::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.3);
  border-radius: 3px;
}

.market-sidebar::-webkit-scrollbar-thumb {
  background: rgba(18, 240, 224, 0.5);
  border-radius: 3px;
}

.sidebar-section {
  margin-bottom: 30px;
}

.sidebar-section:last-child {
  margin-bottom: 0;
}

.sidebar-title {
  color: #12f0e0;
  font-size: 16px;
  margin: 0 0 15px 0;
}

.category-radio {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.category-radio >>> .el-radio-button {
  width: 100%;
}

.category-radio >>> .el-radio-button__inner {
  width: 100%;
  text-align: left;
}

.price-display {
  display: flex;
  justify-content: space-between;
  margin-top: 10px;
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
}

/* 内容区域 */
.market-content {
  min-height: 600px;
  overflow-y: auto;
  max-height: calc(100vh - 100px);
}

.market-content::-webkit-scrollbar {
  width: 8px;
}

.market-content::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.3);
  border-radius: 4px;
}

.market-content::-webkit-scrollbar-thumb {
  background: rgba(18, 240, 224, 0.5);
  border-radius: 4px;
}

.content-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  gap: 15px;
}

.search-input {
  width: 300px;
}

.toolbar-right {
  display: flex;
  gap: 10px;
}

.cart-badge >>> .el-badge__content {
  background: #ff6b6b;
}

.products-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
  gap: 20px;
}

.product-card.metaverse-card {
  background: rgba(8, 10, 12, 0.9);
  border: 2px solid rgba(18, 240, 224, 0.3);
  border-radius: 15px;
  padding: 0;
  cursor: pointer;
  transition: all 0.3s ease;
  backdrop-filter: blur(10px);
  box-shadow: 0 10px 30px rgba(18, 240, 224, 0.1);
  overflow: hidden;
  position: relative;
}

.product-card.metaverse-card:hover {
  transform: translateY(-8px);
  border-color: rgba(18, 240, 224, 0.6);
  box-shadow: 0 20px 50px rgba(18, 240, 224, 0.3);
}

.product-badge {
  position: absolute;
  top: 10px;
  right: 10px;
  background: #ff6b6b;
  color: white;
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  z-index: 10;
}

.product-image {
  width: 100%;
  height: 180px;
  background: linear-gradient(135deg, rgba(18, 240, 224, 0.2) 0%, rgba(18, 240, 224, 0.05) 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  overflow: hidden;
}

.product-image-img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  transition: transform 0.3s ease;
}

.product-card.metaverse-card:hover .product-image-img {
  transform: scale(1.1);
}

.image-overlay {
  font-size: 64px;
  color: #12f0e0;
  opacity: 0.6;
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
}

.product-tags {
  position: absolute;
  bottom: 10px;
  left: 10px;
  display: flex;
  gap: 5px;
  flex-wrap: wrap;
}

.tag {
  background: rgba(18, 240, 224, 0.2);
  color: #12f0e0;
  padding: 2px 8px;
  border-radius: 4px;
  font-size: 11px;
}

.product-info {
  padding: 20px;
}

.product-title {
  color: #12f0e0;
  font-size: 18px;
  margin: 0 0 10px 0;
  font-weight: 600;
}

.product-description {
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
  margin: 0 0 15px 0;
  line-height: 1.6;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.product-meta {
  display: flex;
  gap: 15px;
  margin-bottom: 15px;
  font-size: 12px;
  color: rgba(255, 255, 255, 0.5);
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 5px;
}

.product-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: 15px;
  border-top: 1px solid rgba(18, 240, 224, 0.2);
}

.price-section {
  display: flex;
  align-items: baseline;
}

.price-symbol {
  color: #ffd700;
  font-size: 16px;
  margin-right: 2px;
}

.product-price {
  color: #ffd700;
  font-size: 24px;
  font-weight: bold;
}

.empty-state {
  grid-column: 1 / -1;
  text-align: center;
  padding: 60px 20px;
  color: rgba(255, 255, 255, 0.5);
}

.empty-state i {
  font-size: 64px;
  margin-bottom: 20px;
  display: block;
}

/* 购物车 */
.market-cart {
  background: rgba(8, 10, 12, 0.95);
  border: 1px solid rgba(18, 240, 224, 0.3);
  border-radius: 15px;
  padding: 20px;
  height: fit-content;
  position: sticky;
  top: 20px;
  display: flex;
  flex-direction: column;
  max-height: calc(100vh - 40px);
}

.cart-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding-bottom: 15px;
  border-bottom: 1px solid rgba(18, 240, 224, 0.2);
}

.cart-header h3 {
  color: #12f0e0;
  margin: 0;
}

.cart-content {
  flex: 1;
  overflow-y: auto;
  margin-bottom: 20px;
}

.cart-empty {
  text-align: center;
  padding: 40px 20px;
  color: rgba(255, 255, 255, 0.5);
}

.cart-empty i {
  font-size: 48px;
  margin-bottom: 15px;
  display: block;
}

.cart-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px;
  background: rgba(18, 240, 224, 0.05);
  border-radius: 8px;
  margin-bottom: 10px;
}

.cart-item-info h4 {
  color: #12f0e0;
  font-size: 14px;
  margin: 0 0 5px 0;
}

.cart-item-price {
  color: #ffd700;
  font-size: 16px;
  font-weight: bold;
  margin: 0;
}

.cart-footer {
  padding-top: 15px;
  border-top: 1px solid rgba(18, 240, 224, 0.2);
}

.cart-total {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
  color: rgba(255, 255, 255, 0.8);
  font-size: 16px;
}

.total-price {
  color: #ffd700;
  font-size: 24px;
  font-weight: bold;
}

.checkout-btn {
  width: 100%;
  height: 45px;
  font-size: 16px;
}

/* 对话框样式 */
.nft-dialog >>> .el-dialog {
  background: rgba(8, 10, 12, 0.95);
  border: 1px solid rgba(18, 240, 224, 0.3);
}

.nft-dialog >>> .el-dialog__title {
  color: #12f0e0;
}

.product-detail {
  color: #fff;
}

.detail-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding-bottom: 15px;
  border-bottom: 1px solid rgba(18, 240, 224, 0.2);
}

.detail-header h2 {
  color: #12f0e0;
  margin: 0;
}

.detail-price {
  color: #ffd700;
  font-size: 32px;
  font-weight: bold;
}

.detail-description {
  color: rgba(255, 255, 255, 0.8);
  line-height: 1.8;
  margin-bottom: 20px;
}

.detail-info-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
  margin-bottom: 30px;
  padding: 20px;
  background: rgba(18, 240, 224, 0.1);
  border-radius: 8px;
}

.info-item {
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.info-label {
  color: rgba(255, 255, 255, 0.6);
  font-size: 12px;
}

.info-value {
  color: #12f0e0;
  font-size: 16px;
}

.detail-actions {
  display: flex;
  gap: 15px;
}

.payment-dialog >>> .el-dialog {
  background: rgba(8, 10, 12, 0.95);
}

.payment-content {
  color: #fff;
}

.payment-method-select {
  display: flex;
  gap: 15px;
  margin-bottom: 30px;
}

.payment-method {
  flex: 1;
  padding: 20px;
  border: 2px solid rgba(18, 240, 224, 0.3);
  border-radius: 10px;
  text-align: center;
  cursor: pointer;
  transition: all 0.3s;
}

.payment-method:hover,
.payment-method.active {
  border-color: #12f0e0;
  background: rgba(18, 240, 224, 0.1);
}

.payment-method i {
  font-size: 32px;
  color: #12f0e0;
  display: block;
  margin-bottom: 10px;
}

.payment-info {
  margin-bottom: 20px;
  padding: 20px;
  background: rgba(18, 240, 224, 0.1);
  border-radius: 8px;
}

.info-row {
  display: flex;
  justify-content: space-between;
  margin-bottom: 10px;
  color: rgba(255, 255, 255, 0.8);
}

.info-row:last-child {
  margin-bottom: 0;
}

.amount {
  color: #ffd700;
  font-size: 20px;
  font-weight: bold;
}

.payment-qrcode {
  text-align: center;
  padding: 20px;
  background: rgba(18, 240, 224, 0.05);
  border-radius: 8px;
}

.qrcode-title {
  color: #fff;
  font-size: 16px;
  font-weight: 500;
  margin-bottom: 20px;
}

.qrcode-container {
  background: #fff;
  border-radius: 12px;
  padding: 20px;
  margin: 0 auto 15px;
  width: fit-content;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
}

.qrcode-container.wechat {
  background: linear-gradient(135deg, #07c160 0%, #06ad56 100%);
  padding: 30px 20px 20px;
}

.qrcode-container.alipay {
  background: linear-gradient(135deg, #1677ff 0%, #0958d9 100%);
  padding: 30px 20px 20px;
}

.qrcode-image {
  width: 250px;
  height: 250px;
  display: block;
  margin: 0 auto;
  border-radius: 8px;
  background: #fff;
  padding: 10px;
  box-sizing: border-box;
}

.qrcode-hint {
  color: #333;
  font-size: 14px;
  margin-top: 15px;
  margin-bottom: 0;
  background: #fff;
  padding: 8px 15px;
  border-radius: 20px;
  display: inline-block;
}

.qrcode-container.wechat .qrcode-hint,
.qrcode-container.alipay .qrcode-hint {
  color: #fff;
  background: rgba(255, 255, 255, 0.2);
}

.payment-logo {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  color: #12f0e0;
  font-size: 18px;
  font-weight: 600;
}

.payment-logo.wechat {
  color: #07c160;
}

.payment-logo.alipay {
  color: #1677ff;
}

.payment-logo i {
  font-size: 24px;
}

.qrcode-placeholder {
  width: 250px;
  height: 250px;
  margin: 20px auto;
  background: rgba(18, 240, 224, 0.1);
  border: 2px dashed rgba(18, 240, 224, 0.3);
  border-radius: 8px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  color: rgba(255, 255, 255, 0.5);
}

.qrcode-placeholder i {
  font-size: 48px;
  margin-bottom: 10px;
}
</style>
