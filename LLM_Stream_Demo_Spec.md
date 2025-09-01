# LLM 流式显示 Demo（基于 faceshiftDemo 扩展）

本设计基于当前 Qt 程序（faceshiftDemo）实现一个演示程序：将大模型输出以逐字流式显示到表情区域下方，同时展示用户语音识别（ASR）结果。与大模型之间通过 Socket 连接交互，Socket 固定配置、自动启动，无需手动配置面板。

## 1. 目标与范围
- 将 LLM 的回答以逐字符流式更新的方式显示在表情显示区下方。
- 显示窗口仅保留最近 1～2 行文本（自动换行/滚动策略）。
- 同步展示用户 ASR 的识别文本（可显示为“User:”一行，实时刷新）。
- 与大模型的通信通过固定的 Socket（TCP）连接完成，不提供手动配置 UI。
- 与现有“表情图像序列插值”功能并存，不影响表情动画逻辑。

## 2. 运行环境
- 平台：Windows 10/11
- Qt：5.13.2（MinGW 7.3.0 64-bit）
- 构建工具：qmake + mingw32-make

## 3. UI 设计
- 保留现有表情展示区（faceLabel，图像序列驱动）。
- 在 faceLabel 下方新增两行文本区域：
  - 第 1 行：LLM 流式输出区（逐字显示，始终只保留 1～2 行）。
  - 第 2 行：ASR 文本区（显示最近一次识别结果）。
- 建议控件：
  - LLM 流区：QLabel 或 QPlainTextEdit（只读、无边框、自动换行），固定高度以控制行数。
  - ASR 区：QLabel（灰色小字，单行）。
- 不再显示 Socket 手动配置按钮区域；程序启动即自动监听固定端口。

## 4. Socket 策略（固定配置）
- 角色：程序作为 TCP 服务器（与现有实现一致）。
- 监听：0.0.0.0:8888（固定端口；自动启动；无 UI 按钮）。
- 连接：单客户优先，允许断开/重连。
- 数据格式：逐行 JSON（一条消息一行，以“\n”结尾）。

### 4.1 消息协议
- LLM 流式 Token（partial）：
  ```json
  {"type":"llm_stream","content":"你","final":false}
  ```
- LLM 最终完成（可选）：
  ```json
  {"type":"llm_stream","content":"","final":true}
  ```
- ASR 实时/最终结果：
  ```json
  {"type":"asr","text":"请提醒我按时吃药","final":true}
  ```
- 备注：
  - 为提高鲁棒性，content 可以为多字符 Token；UI 侧仍按“逐字符定时推进”显示。
  - 若需要携带会话/分段编号，可扩展字段：session_id、segment_id。

## 5. 流式显示实现
- 采用“生产者-消费者 + 定时器”模式：
  - 生产者：Socket 线程收到 llm_stream 的 content，将字符串拆分为字符队列（QQueue<QChar>）并追加到待显示缓冲。
  - 消费者：QTimer（例如 20～40ms）从队列取 1～N 个字符（N=1 或根据速率动态），追加到显示文本。
- 行数控制：
  - 文本加入后，进行换行与裁剪，仅保留最近 1～2 行。
  - 可用 QFontMetrics 估算单行字符数：maxChars = floor(labelWidth / averageCharWidth)。若超出则插入“\n”。
  - 若行数 > 2，移除最前行，确保只保留最后 2 行。
- 线程安全：
  - Socket 回调与 UI 更新通过 Qt::QueuedConnection/信号槽分发到主线程；禁止直接跨线程操作 UI。

## 6. 关键类与信号槽（建议）
- 类/成员：
  - TypingDisplay（内部类/组件）：
    - 成员：QLabel* llmLabel、QLabel* asrLabel、QTimer* typingTimer、QQueue<QChar> tokenQueue、QString currentText。
    - 方法：appendTokens(const QString& t)、setAsrText(const QString& t)、tick()（定时取出字符并更新）。
  - Socket：沿用现有 QTcpServer + QTcpSocket 结构，去除 UI 控制按钮，启动时自动监听 8888。
- 信号槽（示例）：
  - void onDataReceived()：解析每一行 JSON，按 type 分发。
  - void onLlmTokens(const QString& tokens, bool isFinal) Q_SLOTS：入队 tokens，isFinal 仅用于状态管理。
  - void onAsrText(const QString& text, bool isFinal) Q_SLOTS：刷新 asrLabel 文本。

## 7. 代码要点（片段示例）
- 自动监听（构造函数末或 initializeSocketServer 后）：
  - startSocketServer(8888); // 无 UI，启动即监听
- 读取与分发：
  - 每次 readyRead 读取缓冲，将“\n”分割为完整 JSON 行，再用 QJsonDocument::fromJson 解析。
  - if type == "llm_stream" -> emit onLlmTokens(content, final)
  - if type == "asr" -> emit onAsrText(text, final)
- TypingDisplay::tick：
  - 若 tokenQueue 非空，取前 1 个字符追加到 currentText。
  - 调整 currentText：按 QFontMetrics 限制宽度并强制换行，最终仅保留最近 1～2 行；setText(currentText)。

## 8. 兼容性与现有功能
- 不改变图像序列插值与播放逻辑：
  - faceLabel 仍由图像序列驱动，emoji 已全部禁用。
- Socket UI 面板移除/隐藏：
  - 固定端口、自动监听，保持与 Python 客户端兼容（无需用户干预）。
- EmotionOutput JSON 支持继续保留（可并行使用）。

## 9. 测试方案
1) 启动程序，确认窗口下方出现两行文本区域（LLM、ASR）。
2) 通过 Python 客户端发送：
   - 多条 llm_stream partial（content 各含若干汉字），应逐字显示，持续滚动，仅保留 1～2 行。
   - 发送 asr 消息，第二行应更新为“User: 文本”。
3) 调整发送速率与 Token 长度，观察定时器平滑显示效果（20～40ms/字符）。
4) 在播放插值动画时发送流式文本，确认互不干扰，UI 流畅。

## 10. 部署
- 继续使用 deploy 文件夹 + deploy.zip 的打包方式。
- 程序启动即监听 8888 端口，无需任何 Socket 设置按钮。

## 11. 后续优化（可选）
- TypingDisplay 动态速率：根据缓冲长度自适应提速/降速。
- Markdown 富文本渲染（加粗/斜体/代码等）。
- 多会话隔离（按 session_id 显示不同用户/对话）。
- 将 ASR/LLM 状态（录音中/思考中）用小图标或动画提示。

## 12. 基于 NerController.java 的文本接收方法（HTTP POST + Flux<String> 流式）

本节给出与当前后端 NerController.java 完全匹配的前端（Qt）文本接收方案。NerController 在 POST /ner 上返回 Flux<String>（HTTP chunked 流式纯文本分片），Qt 端使用 QNetworkAccessManager 发起 POST，并通过 QNetworkReply::readyRead 增量读取，实时送入 UI 的逐字显示队列。

- 服务端接口要点（从 NerController 推断）
  - URL：POST http://<server>:8080/ner?memoryId=<MEMORY_ID>
  - 请求体：
    - application/json：VoiceInputRequest（含 transcription、mentionedPersons、speaker）
    - 或 multipart/form-data：字段名 imageFile（可与文本并用）
  - 响应：Flux<String>（UTF-8 文本分片），HTTP chunked，客户端需在 readyRead 中增量处理

- Qt 端总体思路
  - QNetworkAccessManager 发 POST -> 得到 QNetworkReply
  - 连接 readyRead，在回调中 readAll() -> QString::fromUtf8() -> emit llmTokens(text, false)
  - 在 finished() 中 emit llmTokens("", true) 标记完成，清理 reply
  - 与 TypingDisplay（逐字显示）对接：llmTokens 信号仅负责“入队字符”，QTimer 以 20–40ms 出队 1 个字符并更新 UI，仅保留 1–2 行

- 需要的类成员/信号/槽（建议放入 Widget）
  - 成员：
    - QNetworkAccessManager* nerNam = nullptr;
    - QNetworkReply* nerReply = nullptr;
    - QByteArray nerBuffer; // 可选：若需粘包/边界控制
  - 信号（供 UI/TypingDisplay 使用）：
    - Q_SIGNALS: void llmTokens(const QString& tokens, bool isFinal);
    - Q_SIGNALS: void asrText(const QString& text, bool isFinal); // 若你的 ASR 来源独立，可保留此信号
  - 槽：
    - Q_SLOTS: void startNerStreamJson(const QUrl& baseUrl, const QString& memoryId, const QJsonObject& voiceInput);
    - Q_SLOTS: void startNerStreamMultipart(const QUrl& baseUrl, const QString& memoryId, const QJsonObject& voiceInput, const QString& imagePath);
    - Q_SLOTS: void onNerReadyRead();
    - Q_SLOTS: void onNerFinished();
    - Q_SLOTS: void onNerError(QNetworkReply::NetworkError);

- 核心流程（伪代码）
  1) JSON 请求（仅文本）：
  ```cpp
  // 构造 URL
  QUrl url = baseUrl; // 例如 http://127.0.0.1:8080
  url.setPath("/ner");
  QUrlQuery q; q.addQueryItem("memoryId", memoryId); url.setQuery(q);

  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
  req.setRawHeader("Accept", "text/plain");
  req.setRawHeader("Connection", "keep-alive");

  QByteArray body = QJsonDocument(voiceInput).toJson(QJsonDocument::Compact);
  nerReply = nerNam->post(req, body);

  connect(nerReply, &QNetworkReply::readyRead, this, &Widget::onNerReadyRead);
  connect(nerReply, &QNetworkReply::finished, this, &Widget::onNerFinished);
  connect(nerReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onNerError(QNetworkReply::NetworkError)));
  ```

  2) Multipart（带图片）：
  ```cpp
  QHttpMultiPart* multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  // 文本（可选：按后端约定）
  QHttpPart txt;
  txt.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"voiceInput\""));
  txt.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain; charset=utf-8"));
  txt.setBody(QJsonDocument(voiceInput).toJson(QJsonDocument::Compact));
  multi->append(txt);

  // 图片
  QFile* file = new QFile(imagePath, multi);
  if (file->open(QIODevice::ReadOnly)) {
      QHttpPart img;
      img.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"imageFile\"; filename=\"" + QFileInfo(*file).fileName() + "\""));
      img.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
      img.setBodyDevice(file);
      multi->append(img);
  }

  // POST
  QUrl url = baseUrl; url.setPath("/ner");
  QUrlQuery q; q.addQueryItem("memoryId", memoryId); url.setQuery(q);
  QNetworkRequest req(url);
  nerReply = nerNam->post(req, multi);
  multi->setParent(nerReply);

  connect(nerReply, &QNetworkReply::readyRead, this, &Widget::onNerReadyRead);
  connect(nerReply, &QNetworkReply::finished, this, &Widget::onNerFinished);
  connect(nerReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onNerError(QNetworkReply::NetworkError)));
  ```

  3) 增量读取与分发：
  ```cpp
  void Widget::onNerReadyRead() {
      QByteArray chunk = nerReply->readAll();
      if (!chunk.isEmpty()) emit llmTokens(QString::fromUtf8(chunk), false);
  }
  void Widget::onNerFinished() {
      emit llmTokens(QString(), true); // 结束标记（可选）
      nerReply->deleteLater(); nerReply = nullptr;
  }
  void Widget::onNerError(QNetworkReply::NetworkError) {
      // 记录日志，必要时提示/重试
  }
  ```

- 与 UI 逐字显示对接
  - 连接：connect(this, &Widget::llmTokens, typingDisplay, &TypingDisplay::appendTokens, Qt::QueuedConnection);
  - TypingDisplay::appendTokens 仅做“字符入队”；tick() 用 QTimer 以 20–40ms 逐字更新 QLabel 文本；保持最多 1–2 行（参考第 5 节）

- 与第 4 节（TCP Socket JSON 行协议）的关系
  - 两种输入渠道可并存：
    - 后端为 Java NerController 时，默认启用“HTTP 流式”方案（本节）
    - 若需要直接与本地/其他客户端通过 Socket 行协议交互，可启用第 4 节方案
  - 建议用运行时开关或编译宏控制，避免并发接入造成重复显示

- 编码与健壮性
  - 服务端编码 UTF-8；客户端统一 QString::fromUtf8()
  - 新请求前若已有 nerReply，先 abort() 并清理，避免并发
  - faceshiftDemo.pro 确保：QT += network

- 调用示例
  ```cpp
  // 仅文本
  QJsonObject voice;
  voice["transcription"] = "请提醒我按时吃药";
  startNerStreamJson(QUrl("http://127.0.0.1:8080"), "memory-001", voice);

  // 文本 + 图片
  QJsonObject voice2; voice2["transcription"] = "识别这张药品照片并提醒";
  startNerStreamMultipart(QUrl("http://127.0.0.1:8080"), "memory-002", voice2, "D:/images/drug.jpg");
  ```

- 可选：ASR 并行显示
  - 若 ASR 来自独立通道，继续通过 asrText 信号更新第二行
  - 若后端将 LLM/ASR 混合在一个流内，建议服务端改为“逐行 JSON”，客户端解析后分别 emit 到 llmTokens/asrText