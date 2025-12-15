<template>
  <div class="user-chat-container">
    <div class="chat-header">
      <h3 class="chat-title">社区对话</h3>
      <div class="header-actions">
        <el-button size="mini" @click="showCommunity = !showCommunity" icon="el-icon-s-custom">
          {{ showCommunity ? '系统' : '社区' }}
        </el-button>
        <el-button size="mini" @click="clearChat" icon="el-icon-delete">清空</el-button>
      </div>
    </div>
    
    <div class="chat-messages" ref="messagesContainer">
      <div 
        v-for="(msg, index) in messages" 
        :key="index"
        :class="['message-item', msg.type]"
      >
        <div class="message-avatar" :style="{ background: getAvatarColor(msg.userId) }">
          <i :class="getAvatarIcon(msg.type)"></i>
        </div>
        <div class="message-content">
          <div class="message-name">
            {{ msg.name }}
            <span v-if="msg.type === 'community'" class="user-badge">用户{{ msg.userId }}</span>
          </div>
          <div class="message-text">{{ msg.text }}</div>
          <div class="message-time">{{ msg.time }}</div>
        </div>
      </div>
    </div>
    
    <div class="chat-input-area">
      <el-input
        v-model="inputMessage"
        :placeholder="showCommunity ? '在社区中发言...' : '输入消息...'"
        @keyup.enter.native="sendMessage"
        class="chat-input"
      >
        <el-button slot="append" @click="sendMessage" icon="el-icon-position">发送</el-button>
      </el-input>
    </div>
  </div>
</template>

<script>
export default {
  name: 'UserChat',
  data() {
    return {
      inputMessage: '',
      showCommunity: false,
      currentUserId: Math.floor(Math.random() * 1000),
      messages: [
        {
          type: 'system',
          name: '系统助手',
          text: '欢迎使用鸿蒙玲珑核（鸿蒙量化模型运维）平台！我可以帮助您解决系统问题。',
          time: this.formatTime(new Date()),
          userId: 0
        },
        {
          type: 'community',
          name: '运维工程师_张',
          text: '刚用量化报告功能，修复了一个跨OS数据同步问题，效果不错！',
          time: this.formatTime(new Date(Date.now() - 300000)),
          userId: 101
        },
        {
          type: 'community',
          name: '系统管理员_李',
          text: '自主运维功能确实省了不少时间，自动修复率很高。',
          time: this.formatTime(new Date(Date.now() - 180000)),
          userId: 102
        }
      ]
    }
  },
  mounted() {
    this.scrollToBottom()
  },
  methods: {
    sendMessage() {
      if (!this.inputMessage.trim()) return
      
      const messageType = this.showCommunity ? 'community' : 'user'
      const userName = this.showCommunity ? `用户_${this.currentUserId}` : '我'
      
      // 添加用户消息
      this.messages.push({
        type: messageType,
        name: userName,
        text: this.inputMessage,
        time: this.formatTime(new Date()),
        userId: this.currentUserId
      })
      
      this.inputMessage = ''
      this.$nextTick(() => {
        this.scrollToBottom()
      })
      
      // 模拟回复
      setTimeout(() => {
        if (this.showCommunity) {
          // 社区模式：模拟其他用户回复
          const communityReplies = [
            '我也遇到过类似问题，可以试试看错误日志功能。',
            '量化报告确实很有用，特别是合规存证方面。',
            '轻量化部署真的很方便，多OS支持很棒！',
            '自主运维帮我节省了很多时间。'
          ]
          const randomUserId = 100 + Math.floor(Math.random() * 50)
          this.messages.push({
            type: 'community',
            name: `用户_${randomUserId}`,
            text: communityReplies[Math.floor(Math.random() * communityReplies.length)],
            time: this.formatTime(new Date()),
            userId: randomUserId
          })
        } else {
          // 系统模式：系统助手回复
          this.messages.push({
            type: 'system',
            name: '系统助手',
            text: this.getAutoReply(),
            time: this.formatTime(new Date()),
            userId: 0
          })
        }
        this.$nextTick(() => {
          this.scrollToBottom()
        })
      }, 800)
    },
    getAutoReply() {
      const replies = [
        '收到您的消息，正在处理中...',
        '已记录您的问题，系统正在分析。',
        '建议您查看错误日志获取更多信息。',
        '可以尝试使用自主运维功能自动修复。',
        '如需帮助，请访问模型库购买专业修复方案。'
      ]
      return replies[Math.floor(Math.random() * replies.length)]
    },
    clearChat() {
      this.messages = [{
        type: 'system',
        name: '系统助手',
        text: '对话已清空，有什么可以帮您的吗？',
        time: this.formatTime(new Date()),
        userId: 0
      }]
    },
    getAvatarColor(userId) {
      const colors = [
        'rgba(18, 240, 224, 0.3)',
        'rgba(255, 100, 100, 0.3)',
        'rgba(100, 255, 100, 0.3)',
        'rgba(255, 200, 100, 0.3)',
        'rgba(200, 100, 255, 0.3)'
      ]
      return colors[userId % colors.length]
    },
    getAvatarIcon(type) {
      if (type === 'community') return 'el-icon-user-solid'
      if (type === 'user') return 'el-icon-user'
      return 'el-icon-service'
    },
    scrollToBottom() {
      const container = this.$refs.messagesContainer
      if (container) {
        container.scrollTop = container.scrollHeight
      }
    },
    formatTime(date) {
      const hours = date.getHours().toString().padStart(2, '0')
      const minutes = date.getMinutes().toString().padStart(2, '0')
      return `${hours}:${minutes}`
    }
  }
}
</script>

<style scoped>
.user-chat-container {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: rgba(8, 10, 12, 0.8);
  border: 1px solid rgba(18, 240, 224, 0.3);
  border-radius: 10px;
  overflow: hidden;
}

.chat-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 20px;
  border-bottom: 1px solid rgba(18, 240, 224, 0.2);
  background: rgba(0, 0, 0, 0.3);
}

.header-actions {
  display: flex;
  gap: 8px;
}

.chat-title {
  color: #12f0e0;
  margin: 0;
  font-size: 18px;
  text-shadow: 0 0 10px rgba(18, 240, 224, 0.5);
}

.chat-messages {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 15px;
}

/* 自定义滚动条 */
.chat-messages::-webkit-scrollbar {
  width: 6px;
}

.chat-messages::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.3);
}

.chat-messages::-webkit-scrollbar-thumb {
  background: rgba(18, 240, 224, 0.5);
  border-radius: 3px;
}

.message-item {
  display: flex;
  gap: 12px;
  animation: fadeIn 0.3s ease;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.message-item.user {
  flex-direction: row-reverse;
}

.message-item.community {
  flex-direction: row;
}

.user-badge {
  font-size: 10px;
  color: rgba(18, 240, 224, 0.6);
  margin-left: 5px;
  font-weight: normal;
}

.message-avatar {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.message-item.user .message-avatar {
  background: rgba(18, 240, 224, 0.2);
  color: #12f0e0;
}

.message-item.system .message-avatar {
  background: rgba(255, 255, 255, 0.1);
  color: rgba(18, 240, 224, 0.8);
}

.message-content {
  flex: 1;
  max-width: 70%;
}

.message-item.user .message-content {
  text-align: right;
}

.message-name {
  color: #12f0e0;
  font-size: 12px;
  margin-bottom: 5px;
  font-weight: bold;
}

.message-text {
  background: rgba(0, 0, 0, 0.4);
  padding: 10px 15px;
  border-radius: 10px;
  color: #e6f7f6;
  line-height: 1.5;
  word-wrap: break-word;
  border: 1px solid rgba(18, 240, 224, 0.2);
}

.message-item.user .message-text {
  background: rgba(18, 240, 224, 0.15);
  border-color: rgba(18, 240, 224, 0.4);
}

.message-time {
  color: rgba(18, 240, 224, 0.5);
  font-size: 11px;
  margin-top: 5px;
}

.chat-input-area {
  padding: 15px 20px;
  border-top: 1px solid rgba(18, 240, 224, 0.2);
  background: rgba(0, 0, 0, 0.3);
}

.chat-input {
  width: 100%;
}

.chat-input >>> .el-input__inner {
  background: rgba(0, 0, 0, 0.5);
  border-color: rgba(18, 240, 224, 0.3);
  color: #e6f7f6;
}

.chat-input >>> .el-input-group__append {
  background: rgba(18, 240, 224, 0.2);
  border-color: rgba(18, 240, 224, 0.3);
  color: #12f0e0;
}
</style>

