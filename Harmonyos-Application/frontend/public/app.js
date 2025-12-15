(function(){
  const messagesEl = document.getElementById('messages');
  const textarea = document.getElementById('chat_bot');
  const sendBtn = document.querySelector('.btn-submit');

  function appendMessage(role, contentNode) {
    const div = document.createElement('div');
    div.className = 'msg ' + (role === 'user' ? 'user' : 'assistant');
    const bubble = document.createElement('div');
    bubble.className = 'bubble';
    if (typeof contentNode === 'string') bubble.textContent = contentNode;
    else bubble.appendChild(contentNode);
    div.appendChild(bubble);
    messagesEl.appendChild(div);
    messagesEl.scrollTop = messagesEl.scrollHeight;
    return bubble;
  }

  async function sendMessage() {
    const text = textarea.value.trim();
    if (!text) return;

    // Normalize (remove whitespace) for greeting detection
    const normalized = text.replace(/\s+/g, '');
    const greetingRe = /^(你好|你好的|您好)$/i;
    if (greetingRe.test(normalized)) {
      // append user message and show delayed local assistant reply (exact requested text)
      appendMessage('user', text);
      textarea.value = '';

      // show glitch loader then replace with reply after 4s
      const localReply = '你好，我是鸿蒙玲珑核，有什么可以帮助你的';
      const glitchWrap = document.createElement('div');
      glitchWrap.className = 'glitch-wrap';
      const glitchEl = document.createElement('div');
      glitchEl.className = 'glitch';
      glitchEl.setAttribute('data-glitch', 'Loading...');
      glitchEl.textContent = 'Loading...';
      glitchWrap.appendChild(glitchEl);
      const assistantBubble = appendMessage('assistant', glitchWrap);
      await new Promise((resolve) => setTimeout(resolve, 4000));
      assistantBubble.textContent = localReply;
      return;
    }

    // Detect capability questions (e.g. "你能干什么", "你会做什么", "能做什么") and reply locally with a rich AI-style description
    const capabilityRe = /(你能干什么|你能做什么|你会做什么|能做什么|可以做什么|能帮我做什么)/i;
    if (capabilityRe.test(text)) {
      appendMessage('user', text);
      textarea.value = '';
      const capabilityReply = '我可以回答你的任何问题，提供编程与调试建议、文案与文章生成、翻译、结构化数据解析、接口与配置示例、以及针对具体问题给出可执行的步骤和参考代码。\n\n' +
        '此外，我配备了实时运维系统能力：能够帮助你分析日志、定位异常、生成排查步骤和建议告警策略，并在会话中提供监控指标和故障排查思路。\n\n' +
        '如果你有具体任务（例如：写一个接口示例、优化一个 SQL 查询、排查某个错误日志），告诉我细节，我会一步步给出可操作的建议和示例。';

      // show glitch loader then replace with reply after 4s
      const glitchWrap = document.createElement('div');
      glitchWrap.className = 'glitch-wrap';
      const glitchEl = document.createElement('div');
      glitchEl.className = 'glitch';
      glitchEl.setAttribute('data-glitch', 'Loading...');
      glitchEl.textContent = 'Loading...';
      glitchWrap.appendChild(glitchEl);
      const assistantBubble = appendMessage('assistant', glitchWrap);
      await new Promise((resolve) => setTimeout(resolve, 4000));
      assistantBubble.textContent = capabilityReply;
      return;
    }

    // append user message
    appendMessage('user', text);
    textarea.value = '';

    // create assistant bubble with glitch loader
    const glitchWrap = document.createElement('div');
    glitchWrap.className = 'glitch-wrap';
    const glitchEl = document.createElement('div');
    glitchEl.className = 'glitch';
    glitchEl.setAttribute('data-glitch', 'Loading...');
    glitchEl.textContent = 'Loading...';
    glitchWrap.appendChild(glitchEl);
    const assistantBubble = appendMessage('assistant', glitchWrap);

    // ensure at least 4 seconds loading before showing reply
    await new Promise((resolve) => setTimeout(resolve, 4000));

    try {
      const resp = await fetch('/api/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message: text })
      });
      const data = await resp.json();
      if (data.error) {
        assistantBubble.textContent = '错误: ' + (data.error.message || JSON.stringify(data.error));
      } else {
        assistantBubble.textContent = data.reply || '';
      }
    } catch (err) {
      assistantBubble.textContent = '网络错误：' + (err.message || err);
    }
  }

  // click send
  sendBtn.addEventListener('click', sendMessage);

  // Enter to send (textarea): Enter without Shift will send
  textarea.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      sendMessage();
    }
  });
})();
