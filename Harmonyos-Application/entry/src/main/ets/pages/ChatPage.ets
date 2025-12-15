import { Column, Row, Text, TextInput, ScrollView, Image, Button, Swiper, ForEach, State, LazyForEach, CacheMode, FlexAlign, TextAlign, Edge, FontSize, FontWeight, BackgroundColor, BorderRadius, Padding, Margin, Shadow, ZIndex } from '@ohos.ui';
import { MessageItem } from './MessageItem';
import { ChatStore, Message } from './ChatStore';
import hilog from '@ohos.hilog';

// 鸿蒙设计规范颜色
const COLORS = {
  primary: '#007DFF',       // 主题蓝
  background: '#F5F7FA',    // 页面背景
  userBubble: '#007DFF',    // 用户消息气泡
  systemBubble: '#FFFFFF',  // 系统消息气泡
  textPrimary: '#18181B',   // 主要文本
  textSecondary: '#666666', // 次要文本
  inputBg: '#FFFFFF',       // 输入框背景
  border: '#E5E7EB'         // 边框颜色
};

// 全局状态管理实例
const chatStore = new ChatStore();

@Entry
@Component
struct ChatPage {
  @State inputText: string = '';
  @State isSending: boolean = false;
  private scrollController: ScrollController = new ScrollController();

  // 发送消息
  sendMessage() {
    if (!this.inputText.trim() || this.isSending) return;

    this.isSending = true;
    const userMsg: Message = {
      id: Date.now().toString(),
      content: this.inputText,
      sender: 'user',
      timestamp: new Date().toLocaleTimeString()
    };

    // 添加用户消息
    chatStore.addMessage(userMsg);
    this.inputText = '';
    this.scrollToBottom();

    // 模拟系统回复（2秒后）
    setTimeout(() => {
      const systemMsg: Message = {
        id: (Date.now() + 1).toString(),
        content: `这是系统自动回复：您刚才说的是"${userMsg.content}"\n\n（实际应用中此处会调用AI模型生成回复）`,
        sender: 'system',
        timestamp: new Date().toLocaleTimeString()
      };
      chatStore.addMessage(systemMsg);
      this.scrollToBottom();
      this.isSending = false;
    }, 1000);
  }

  // 滚动到底部
  scrollToBottom() {
    setTimeout(() => {
      this.scrollController.scrollTo({
        position: this.scrollController.currentOffset().totalHeight,
        animation: { duration: 200 }
      });
    }, 100);
  }

  // 清空对话
  clearChat() {
    chatStore.clearMessages();
  }

  build() {
    Column() {
      // 顶部导航栏
      Row() {
        Text('模型对话')
          .fontSize(FontSize.Title)
          .fontWeight(FontWeight.Bold)
          .textAlign(TextAlign.Center)
      }
      .width('100%')
      .height(56)
      .justifyContent(FlexAlign.Center)
      .backgroundColor(COLORS.background)
      .shadow({ radius: 2, color: '#0000001A' })
      .zIndex(1)

      // 消息列表
      ScrollView(this.scrollController) {
        Column() {
          // 欢迎消息（首次进入）
          if (chatStore.messages.length === 0) {
            Column() {
              Image('https://gw.alicdn.com/imgextra/i4/O1CN010eJ3sH1c5xHtHhU7g_!!6000000003550-2-tps-200-200.png')
                .width(120)
                .height(120)
                .margin({ bottom: 20 })
              
              Text('欢迎使用模型对话')
                .fontSize(FontSize.Large)
                .fontWeight(FontWeight.Bold)
                .margin({ bottom: 8 })
              
              Text('请在下方输入消息，开始对话吧~')
                .fontSize(FontSize.Small)
                .fontColor(COLORS.textSecondary)
            }
            .width('100%')
            .height('100%')
            .justifyContent(FlexAlign.Center)
            .alignItems(FlexAlign.Center)
          } else {
            // 消息列表
            LazyForEach(
              chatStore.messages,
              (msg: Message) => MessageItem({ message: msg }),
              (msg) => msg.id
            )
            .padding({ left: 16, right: 16, top: 8, bottom: 8 })
          }
        }
        .width('100%')
      }
      .backgroundColor(COLORS.background)
      .flexGrow(1)

      // 输入区域
      Column() {
        Row() {
          TextInput({
            placeholder: '请输入消息...',
            text: this.inputText
          })
            .onChange((value) => this.inputText = value)
            .onSubmit(() => this.sendMessage())
            .width('80%')
            .height(56)
            .backgroundColor(COLORS.inputBg)
            .borderRadius(28)
            .padding({ left: 16, right: 16 })
            .border({ width: 1, color: COLORS.border })
            .fontSize(FontSize.Medium)

          Button(this.isSending ? '发送中...' : '发送')
            .onClick(() => this.sendMessage())
            .width('18%')
            .height(56)
            .backgroundColor(COLORS.primary)
            .fontColor('#FFFFFF')
            .borderRadius(28)
            .fontSize(FontSize.Medium)
            .disabled(this.isSending || !this.inputText.trim())
        }
        .padding({ left: 16, right: 16, bottom: 20 })
      }
      .backgroundColor('#FFFFFF')
      .shadow({ radius: 4, color: '#0000001A', offsetY: -2 })
    }
    .width('100%')
    .height('100%')
    .backgroundColor(COLORS.background)
    .onPageShow(() => {
      hilog.info(0x0000, 'ChatPage', '模型对话界面加载完成');
    })
  }
}