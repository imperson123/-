import { Row, Column, Text, Image, FlexAlign, TextAlign, FontSize, FontWeight, BackgroundColor, BorderRadius, Padding, Margin, MaxWidth } from '@ohos.ui';
import { Message } from './ChatStore';

// 消息气泡组件
interface MessageItemProps {
  message: Message;
}

// 鸿蒙设计规范颜色（与ChatPage保持一致）
const COLORS = {
  userBubble: '#007DFF',    // 用户消息气泡
  systemBubble: '#FFFFFF',  // 系统消息气泡
  userText: '#FFFFFF',      // 用户文本
  systemText: '#18181B',    // 系统文本
  timestamp: '#9CA3AF',     // 时间戳颜色
  avatarBg: '#E5E7EB'       // 头像背景
};

export function MessageItem(props: MessageItemProps) {
  const { message } = props;
  const isUser = message.sender === 'user';

  return (
    <Row 
      justifyContent={isUser ? FlexAlign.End : FlexAlign.Start}
      margin={{ bottom: 16 }}
    >
      {/* 头像 */}
      {!isUser && (
        <Image('https://gw.alicdn.com/imgextra/i2/O1CN01i5xM101rCIbJxR6Xh_!!6000000005933-2-tps-64-64.png')
          .width(40)
          .height(40)
          .borderRadius(20)
          .margin({ right: 8 })
      )}

      {/* 消息内容 */}
      <Column alignItems={isUser ? FlexAlign.End : FlexAlign.Start}>
        {/* 消息气泡 */}
        <Row 
          background={isUser ? COLORS.userBubble : COLORS.systemBubble}
          borderRadius={{
            topLeft: 18,
            topRight: 18,
            bottomLeft: isUser ? 18 : 4,
            bottomRight: isUser ? 4 : 18
          }}
          padding={{ left: 16, right: 16, top: 10, bottom: 10 }}
          shadow={{ radius: 2, color: '#00000014', offsetX: 0, offsetY: 2 }}
        >
          <Text(message.content)
            fontColor={isUser ? COLORS.userText : COLORS.systemText}
            fontSize={FontSize.Medium}
            maxWidth={MaxWidth('70%')}
            whiteSpace={WhiteSpace.PreWrap}
          />
        </Row>

        {/* 时间戳 */}
        <Text(message.timestamp)
          fontSize={FontSize.Small}
          fontColor={COLORS.timestamp}
          marginTop={4}
          textAlign={TextAlign.End}
        />
      </Column>

      {/* 用户头像（右侧） */}
      {isUser && (
        <Image('https://gw.alicdn.com/imgextra/i4/O1CN01bHdrZJ1Y8wXgjHlX6_!!6000000002659-2-tps-64-64.png')
          .width(40)
          .height(40)
          .borderRadius(20)
          .margin({ left: 8 })
      )}
    </Row>
  );
}