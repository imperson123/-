import { AppStorage } from '@ohos.data.AppStorage';
import { Observer } from '@ohos.util.Observer';

// 消息类型定义
export interface Message {
  id: string;          // 唯一ID
  content: string;     // 消息内容
  sender: 'user' | 'system'; // 发送者
  timestamp: string;   // 时间戳
}

// 聊天状态管理类
export class ChatStore {
  private messages: Message[] = [];
  private observer: Observer<Message[]> | null = null;

  constructor() {
    // 从本地存储加载历史记录（简化版）
    this.loadHistory();
  }

  // 加载历史消息
  private loadHistory() {
    // 实际应用中应从数据库或Preferences加载
    const saved = AppStorage.Get('chat_history') as Message[];
    if (saved && Array.isArray(saved)) {
      this.messages = saved;
    }
  }

  // 保存消息到本地存储
  private saveHistory() {
    // 实际应用中应使用数据库或Preferences
    AppStorage.Set('chat_history', this.messages);
  }

  // 添加消息
  addMessage(message: Message) {
    this.messages.push(message);
    this.saveHistory();
    this.notifyUpdate();
  }

  // 清空消息
  clearMessages() {
    this.messages = [];
    this.saveHistory();
    this.notifyUpdate();
  }

  // 获取消息列表（供LazyForEach使用）
  get messages() {
    return this.messages;
  }

  // 注册观察者（UI更新）
  registerObserver(observer: Observer<Message[]>) {
    this.observer = observer;
  }

  // 通知UI更新
  private notifyUpdate() {
    if (this.observer) {
      this.observer(this.messages.slice()); // 返回副本避免直接修改
    }
  }
}