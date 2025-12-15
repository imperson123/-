# 前端聊天界面（最小示例）

这是一个最小的静态前端 + Node 代理服务示例，用来与大模型 API（例如 OpenAI）对话。

快速开始（Windows PowerShell）：

```powershell
cd "c:\Users\ZLHJ2\OneDrive\Desktop\Harmonyos-Application\frontend"
npm install
$env:OPENAI_API_KEY = "<你的 API key>"   # 临时设置环境变量（PowerShell）
npm start
```

然后打开浏览器访问 `http://localhost:3000`，在文本框输入并发送即可。

注意：
- `server.js` 使用环境变量 `OPENAI_API_KEY`（或 `API_KEY`）和可选的 `MODEL` 来配置要调用的模型。
- 此示例仅用于演示，生产环境需做好认证、速率限制和安全保护。

如需将该前端嵌入到 HarmonyOS 应用中，可将 `public/` 中的页面打包为资源或在应用中使用 WebView 加载本地/远程页面。