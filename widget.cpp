#include "widget.h"
#include "ui_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QFont>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QCoreApplication>
// 新增：HTTP流式与多部分上传所需头文件
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QFile>
#include <QUrlQuery>
#include <QHostAddress>
#include <QSizePolicy>
#include <QScrollBar>
#include <QTextCursor>
#include <QRandomGenerator> // 新增：用于随机眨眼
#include "interfacewidget.h"
#include <functional>
#include <QDir>

namespace {
inline QString faceRes(const QString &file) {
    return QDir(QCoreApplication::applicationDirPath()).filePath("../../faceshiftDemo/qt_face/" + file);
}

inline int randomBlinkIntervalMs() {
    return QRandomGenerator::global()->bounded(4000, 7000 + 1);
}
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , currentExpression(ExpressionType::Normal)
    , isAnimating(false)
    , fromExpression(ExpressionType::Happy)
    , toExpression(ExpressionType::Sad)

    , imageAnimationTimer(new QTimer(this))
    , currentImageFrame(0)
    , interpolationBasePath("face")
    , useImageSequences(false)
    , expressionDurationTimer(new QTimer(this))
    , previousExpression(ExpressionType::Normal)
    , tcpServer(nullptr)
    , serverPort(8888)
    , isServerRunning(false)
    , imageAnimationIntervalMs(50)
    , currentMode(Mode::Expression)
    , interfaceWidget(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("智能用药提醒机器人表情系统");
    resize(1280, 800); // 适配800x1280屏幕
    setupFaceDisplay();
    
    // 连接表情持续时间定时器
    connect(expressionDurationTimer, &QTimer::timeout, this, &Widget::onExpressionDurationTimeout);
    expressionDurationTimer->setSingleShot(true);

    // ======== 睡眠模式 ========
    idleTimer = new QTimer(this);
    idleTimer->setInterval(10000); // 10秒
    idleTimer->setSingleShot(true);
    connect(idleTimer, &QTimer::timeout, this, &Widget::onIdleTimeout);
    idleTimer->start();
    
    // 初始化Socket服务器
    initializeSocketServer();
    
    // ========== 新增：HTTP流式接入初始化 ==========
    nerNam = new QNetworkAccessManager(this);
    nerReply = nullptr;
    llmTypingTimer = new QTimer(this);
    llmTypingTimer->setInterval(30); // 20–40ms 之间
    llmCharsPerTick = 3;
    llmStreamFinished = false;
    connect(this, &Widget::llmTokens, this, &Widget::onLlmTokens);
    connect(this, &Widget::asrText, this, &Widget::updateAsrText);
    connect(llmTypingTimer, &QTimer::timeout, this, &Widget::onTypingTick);
}

Widget::~Widget()
{
    cleanupAnimations();
    // 新增：HTTP流式资源清理
    if (llmTypingTimer) {
        llmTypingTimer->stop();
    }
    if (nerReply) {
        disconnect(nerReply, nullptr, this, nullptr);
        nerReply->abort();
        nerReply->deleteLater();
        nerReply = nullptr;
    }
    delete ui;
}

void Widget::cleanupAnimations()
{
    if (expressionAnimation) {
        expressionAnimation->stop();
        expressionAnimation->clear();
        expressionAnimation = nullptr;
    }
}

void Widget::setupFaceDisplay()
{
    // 根布局：单元格叠放，文本浮动于表情底部
    QGridLayout *root = new QGridLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    faceLabel = new QLabel("", this);
    faceLabel->setAlignment(Qt::AlignCenter);
    QPixmap bgPixmap(faceRes("normal.png"));
    if (!bgPixmap.isNull()) {
        faceLabel->setPixmap(bgPixmap);
        faceLabel->setScaledContents(true);
    }
    faceLabel->setStyleSheet("QLabel { background-color: transparent; }");
    faceLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ======== 眨眼资源加载 ========
    openPixmap = QPixmap(faceRes("normal.png"));
    transitionPixmap = QPixmap(faceRes("transition.png"));
    closedPixmap = QPixmap(faceRes("closed.png"));

    // 初始化眨眼定时器
    blinkTimer = new QTimer(this);
    blinkTimer->setSingleShot(true);
    connect(blinkTimer, &QTimer::timeout, this, &Widget::onBlinkTimeout);

    // 启动首次随机眨眼
    blinkTimer->start(randomBlinkIntervalMs());

    // ========== ASR/LLM 显示区域（浮动） ==========
    QGroupBox* streamGroup = new QGroupBox(this);
    streamGroup->setTitle(QString());
    streamGroup->setStyleSheet("QGroupBox { background: transparent; border: none; margin: 0px; padding: 0px; }");
    streamGroup->setAttribute(Qt::WA_TranslucentBackground);
    QVBoxLayout* streamLayout = new QVBoxLayout(streamGroup);

    // 统一放大字体，适配1280x800
    QFont bigFont = this->font();
    bigFont.setPointSize(18);

    // ASR 前缀+文本框一行布局
    asrLabel = new QLabel(this);
    QPixmap userIcon(faceRes("user_icon.png"));
    if (!userIcon.isNull()) {
        userIcon = userIcon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        asrLabel->setPixmap(userIcon);
        asrLabel->setFixedSize(userIcon.size());
    }

    asrEdit = new QPlainTextEdit(this);
    asrEdit->setReadOnly(true);
    asrEdit->setFont(bigFont);
    asrEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    asrEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    asrEdit->setStyleSheet("QPlainTextEdit { background: transparent; border: none; padding: 0px; color: #ffffff; }");
    {
        QFontMetrics fm(asrEdit->font());
        const int lineH = fm.lineSpacing();
        const int frame = asrEdit->frameWidth();
        const int padding = 0;
        const int extra = 4; // 额外高度，让单行框稍高一点，避免文字贴边
        asrEdit->setFixedHeight(lineH + frame * 2 + padding + extra);
    }
    asrEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 显示固定文本
    asrEdit->setPlainText(QStringLiteral("今天我有没有吃999感冒灵？我有点感冒"));

    QHBoxLayout* asrRow = new QHBoxLayout();
    asrRow->setContentsMargins(0,0,0,0);
    asrRow->setSpacing(0);
    // 标签顶部对齐，保持与文本框首行对齐
    asrRow->addWidget(asrLabel, 0, Qt::AlignTop);
    asrRow->addWidget(asrEdit, 1, Qt::AlignTop);
    streamLayout->addLayout(asrRow);

    // LLM 前缀+文本框三行布局
    llmPrefixLabel = new QLabel(this);
    QPixmap robotIcon(faceRes("robot_icon.png"));
    if (!robotIcon.isNull()) {
        robotIcon = robotIcon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        llmPrefixLabel->setPixmap(robotIcon);
        llmPrefixLabel->setFixedSize(robotIcon.size());
    }

    llmEdit = new QPlainTextEdit(this);
    llmEdit->setReadOnly(true);
    llmEdit->setFont(bigFont);
    llmEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    llmEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    llmEdit->setStyleSheet("QPlainTextEdit { background: transparent; border: none; padding: 0px; color: #ffffff; }");
    {
        QFontMetrics fm(llmEdit->font());
        const int lineH = fm.lineSpacing();
        const int frame = llmEdit->frameWidth();
        const int padding = 0; 
        llmEdit->setFixedHeight(lineH * 3 + frame * 3 + padding);
    }
    llmEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 显示固定文本
    llmEdit->setPlainText(QStringLiteral("根据现有资料，阿司匹林与999感冒灵不建议同时服用，主要原因包括以下几点：\n阿司匹林是一种非甾体抗炎药（NSAID），而999感冒灵中含有对乙酰氨基酚（扑热息痛），两者均为解热镇痛药。若同时使用，可能导致同类药物过量，加重肝脏和肾脏的代谢负担，甚至引发肝损伤或肾毒性。\n副作用叠加，胃肠道和中枢神经系统风险增加\n阿司匹林对胃黏膜有刺激作用，可能引起胃痛、胃溃疡甚至出血；而999感冒灵中的马来酸氯苯那敏（一种抗组胺药）可能引起嗜睡、口干等中枢抑制反应。两者合用可能放大副作用，尤其对老年人或有基础疾病者不利。"));

    QHBoxLayout* llmRow = new QHBoxLayout();
    llmRow->setContentsMargins(0,0,0,0);
    llmRow->setSpacing(6);
    // 标签顶部对齐，保持与文本框首行对齐
    llmRow->addWidget(llmPrefixLabel, 0, Qt::AlignTop);
    llmRow->addWidget(llmEdit,1, Qt::AlignTop);
    streamLayout->addLayout(llmRow);
    streamLayout->addStretch(1);

    // 组装布局：faceLabel 占满，streamGroup 同单元格底对齐
    // 让流式对话区域宽度充满底部，并保持底部对齐
    root->addWidget(faceLabel, 0, 0);
    root->addWidget(streamGroup, 0, 0, Qt::AlignBottom);

    setLayout(root);
}

// 图像序列相关函数实现
QString Widget::expressionTypeToString(ExpressionType type)
{
    switch (type) {
        case ExpressionType::Happy:    return "Happy";
        case ExpressionType::Sad:      return "Sad";
        case ExpressionType::Warning:  return "Warning";
        case ExpressionType::Sleep:    return "Sleep";
        case ExpressionType::Normal:   return "Normal";
        default:                       return "Normal";
    }
}
// ========= 新增：根据表达类型设置背景 =========
void Widget::setExpressionBackground(ExpressionType type)
{
    QString path;
    switch(type){
        case ExpressionType::Happy:    path = faceRes("emotion_happy.png"); break;
        case ExpressionType::Sad:      path = faceRes("emotion_sad.png"); break;
        case ExpressionType::Warning:  path = faceRes("emotion_warning.png"); break;
        case ExpressionType::Sleep:    path = faceRes("sleep.png"); break;
        default: /* Normal */          path = faceRes("normal.png"); break;
    }
    QPixmap pix(path);
    if(!pix.isNull()){
        faceLabel->setPixmap(pix);
    }

    // 更新当前表情状态
    currentExpression = type;

    // Sleep 与 Warning 状态下禁止眨眼，其余状态恢复眨眼
    if (blinkTimer) {
        if (type == ExpressionType::Sleep || type == ExpressionType::Warning) {
            if (blinkTimer->isActive()) {
                blinkTimer->stop();
            }
        } else {
            if (!blinkTimer->isActive()) {
                blinkTimer->start(randomBlinkIntervalMs());
            }
        }
    }
}

// EmotionOutput接口实现
void Widget::processEmotionOutput(const QString& jsonString)
{
    EmotionOutput emotionData = parseEmotionOutputJson(jsonString);
    if (!emotionData.expression_type.isEmpty()) {
        processEmotionOutput(emotionData);
    }
}

void Widget::processEmotionOutput(const EmotionOutput& emotionData)
{
    // 停止当前的表情持续时间定时器
    if (expressionDurationTimer->isActive()) {
        expressionDurationTimer->stop();
    }
    
    // 记录当前表情作为上一个表情
    previousExpression = currentExpression;
    
    // 解析目标表情类型
    ExpressionType targetType = stringToExpressionType(emotionData.expression_type);
    
    // 记录触发原因
    logEmotionTrigger(emotionData.trigger_reason, targetType);
    
    // 保存当前emotion数据
    currentEmotionOutput = emotionData;
    
    // 切换到目标表情
    blinkOnceAsChangeExpression(nullptr);
    setExpressionBackground(targetType);
    // 重新计时空闲定时器
    resetIdleTimer();
    
    // 设置持续时间（如果不是永久状态）
    if (emotionData.duration_ms > 0) {
        expressionDurationTimer->start(emotionData.duration_ms);
    }
}

EmotionOutput Widget::parseEmotionOutputJson(const QString& jsonString)
{
    EmotionOutput result;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << parseError.errorString();
        return result;
    }
    
    QJsonObject rootObj = doc.object();
    
    // 检查是否包含emotion_output字段
    if (rootObj.contains("emotion_output") && rootObj["emotion_output"].isObject()) {
        QJsonObject emotionObj = rootObj["emotion_output"].toObject();
        
        if (emotionObj.contains("expression_type")) {
            result.expression_type = emotionObj["expression_type"].toString();
        }
        
        if (emotionObj.contains("duration_ms")) {
            result.duration_ms = emotionObj["duration_ms"].toInt(3000); // 默认3秒
        }
        
        if (emotionObj.contains("trigger_reason")) {
            result.trigger_reason = emotionObj["trigger_reason"].toString();
        }
    }
    
    return result;
}

ExpressionType Widget::stringToExpressionType(const QString& typeString)
{
    QString lowerType = typeString.toLower();
    if (lowerType == "happy"  || lowerType == "开心")   return ExpressionType::Happy;
    if (lowerType == "sad"    || lowerType == "悲伤")   return ExpressionType::Sad;
    if (lowerType == "warning"|| lowerType == "警示")   return ExpressionType::Warning;
    if (lowerType == "sleep"  || lowerType == "休眠")   return ExpressionType::Sleep;
    // 默认返回Normal
    return ExpressionType::Normal;
}

void Widget::logEmotionTrigger(const QString& reason, ExpressionType type)
{
    QString typeStr = expressionTypeToString(type);
    qDebug() << "[表情切换] 触发原因:" << reason << "目标表情:" << typeStr;
}

void Widget::onExpressionDurationTimeout()
{
    // 持续时间结束，恢复到上一个表情或Normal
    ExpressionType restoreType = (previousExpression != currentExpression) ? previousExpression : ExpressionType::Normal;
    qDebug() << "[表情切换] 持续时间结束，恢复到:" << expressionTypeToString(restoreType);
    // 恢复表情时触发一次眨眼动画
    blinkOnceAsChangeExpression(nullptr);
    setExpressionBackground(restoreType);
}

// ==================== Socket服务器相关函数实现 ====================

void Widget::initializeSocketServer()
{
    tcpServer = new QTcpServer(this);
    
    // 连接服务器信号
    connect(tcpServer, &QTcpServer::newConnection, this, &Widget::onNewConnection);
    
    // 自动启动服务器
    startSocketServer(serverPort);
    
    qDebug() << "[Socket服务器] 初始化完成";
}

void Widget::startSocketServer(quint16 port)
{
    if (isServerRunning) {
        qDebug() << "[Socket服务器] 服务器已在运行，端口:" << serverPort;
        return;
    }
    
    serverPort = port;
    
    if (tcpServer->listen(QHostAddress::Any, serverPort)) {
        isServerRunning = true;
        QString localIP = getLocalIPAddress();
        qDebug() << "[Socket服务器] 启动成功";
        qDebug() << "[Socket服务器] 监听地址:" << localIP << ":" << serverPort;
        qDebug() << "[Socket服务器] Python客户端可连接到:" << localIP << ":" << serverPort;
    } else {
        qDebug() << "[Socket服务器] 启动失败:" << tcpServer->errorString();
        isServerRunning = false;
    }
}

void Widget::stopSocketServer()
{
    if (!isServerRunning) {
        return;
    }
    
    // 断开所有客户端连接
    for (QTcpSocket* socket : clientSockets) {
        socket->disconnectFromHost();
        socket->deleteLater();
    }
    clientSockets.clear();
    
    // 停止服务器监听
    tcpServer->close();
    isServerRunning = false;
    
    qDebug() << "[Socket服务器] 已停止";
}

void Widget::onNewConnection()
{
    while (tcpServer->hasPendingConnections()) {
        QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
        clientSockets.append(clientSocket);
        
        // 连接客户端信号
        connect(clientSocket, &QTcpSocket::readyRead, this, &Widget::onDataReceived);
        connect(clientSocket, &QTcpSocket::disconnected, this, &Widget::onClientDisconnected);
        connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                this, &Widget::onSocketError);
        
        QString clientIP = clientSocket->peerAddress().toString();
        quint16 clientPort = clientSocket->peerPort();
        
        qDebug() << "[Socket服务器] 新客户端连接:" << clientIP << ":" << clientPort;
        qDebug() << "[Socket服务器] 当前连接数:" << clientSockets.size();
        
        // 更新UI显示（Socket UI已移除）
        // updateClientCount();
    }
}

void Widget::onClientDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        QString clientIP = clientSocket->peerAddress().toString();
        quint16 clientPort = clientSocket->peerPort();
        
        clientSockets.removeAll(clientSocket);
        clientSocket->deleteLater();
        
        qDebug() << "[Socket服务器] 客户端断开连接:" << clientIP << ":" << clientPort;
        qDebug() << "[Socket服务器] 当前连接数:" << clientSockets.size();
        
        // 更新UI显示（Socket UI已移除）
        // updateClientCount();
    }
}

void Widget::onDataReceived()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) {
        return;
    }
    
    QByteArray data = clientSocket->readAll();
    QString clientIP = clientSocket->peerAddress().toString();
    
    qDebug() << "[Socket服务器] 收到数据来自" << clientIP << ":" << data;
    
    // 处理接收到的数据
    processSocketData(data);
}

void Widget::onSocketError(QAbstractSocket::SocketError error)
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        QString clientIP = clientSocket->peerAddress().toString();
        qDebug() << "[Socket服务器] 客户端错误" << clientIP << ":" << error << clientSocket->errorString();
    }
}

void Widget::processSocketData(const QByteArray& data)
{
    QString jsonString = QString::fromUtf8(data).trimmed();
    
    // 处理可能的多行JSON数据
    QStringList jsonLines = jsonString.split('\n', QString::SkipEmptyParts);
    
    for (const QString& line : jsonLines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) {
            continue;
        }
        
        // 优先尝试解析ASR/LLM流式协议
        QJsonParseError perr;
        QJsonDocument doc = QJsonDocument::fromJson(trimmedLine.toUtf8(), &perr);
        if (perr.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            const QString type = obj.value("type").toString();
            if (type == "asr") {
                const QString text = obj.value("text").toString();
                const bool isFinal = obj.value("isFinal").toBool(false) || obj.value("is_final").toBool(false);
                Q_EMIT asrText(text, isFinal);
                continue;
            }
            if (type == "llm_stream") {
                const QString text = obj.value("text").toString();
                const bool isFinal = obj.value("isFinal").toBool(false) || obj.value("is_final").toBool(false);
                Q_EMIT llmTokens(text, isFinal);
                continue;
            }
        }
        
        qDebug() << "[Socket数据处理] 处理JSON:" << trimmedLine;
        
        // 调用现有的processEmotionOutput接口
        try {
            processEmotionOutput(trimmedLine);
        } catch (const std::exception& e) {
            qDebug() << "[Socket数据处理] JSON处理异常:" << e.what();
        } catch (...) {
            qDebug() << "[Socket数据处理] 未知异常";
        }
    }
}

QString Widget::getLocalIPAddress()
{
    // 获取本机IP地址
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : addresses) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost) {
            return addr.toString();
        }
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}

// =================== 新增：HTTP流式方法与显示逻辑 ===================
void Widget::startNerStreamJson(const QUrl& baseUrl, const QString& memoryId, const QString& text)
{
    if (nerReply) {
        disconnect(nerReply, nullptr, this, nullptr);
        nerReply->abort();
        nerReply->deleteLater();
        nerReply = nullptr;
    }
    llmPending.clear();
    llmDisplayed.clear();
    llmStreamFinished = false;
    updateLlmDisplay();

    QUrl url(baseUrl);
    QString path = url.path();
    if (!path.endsWith('/')) path += '/';
    path += "ner";
    url.setPath(path);
    QUrlQuery query(url);
    query.addQueryItem("memoryId", memoryId);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/json"));

    QJsonObject body;
    body.insert("text", text);
    QJsonDocument doc(body);

    nerReply = nerNam->post(req, doc.toJson(QJsonDocument::Compact));
    connect(nerReply, &QNetworkReply::readyRead, this, &Widget::onNerReadyRead);
    connect(nerReply, &QNetworkReply::finished, this, &Widget::onNerFinished);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(nerReply, &QNetworkReply::errorOccurred, this, &Widget::onNerError);
#else
    connect(nerReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &Widget::onNerError);
#endif
}

void Widget::startNerStreamMultipart(const QUrl& baseUrl, const QString& memoryId, const QString& imagePath)
{
    if (nerReply) {
        disconnect(nerReply, nullptr, this, nullptr);
        nerReply->abort();
        nerReply->deleteLater();
        nerReply = nullptr;
    }
    llmPending.clear();
    llmDisplayed.clear();
    llmStreamFinished = false;
    updateLlmDisplay();

    QUrl url(baseUrl);
    QString path = url.path();
    if (!path.endsWith('/')) path += '/';
    path += "ner";
    url.setPath(path);
    QUrlQuery query(url);
    query.addQueryItem("memoryId", memoryId);
    url.setQuery(query);

    QNetworkRequest req(url);

    QHttpMultiPart* multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"imageFile\"; filename=\"upload.jpg\""));
    QFile* file = new QFile(imagePath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        delete multi;
        Q_EMIT llmTokens(QStringLiteral("[错误] 无法打开文件: ") + imagePath + "\n", true);
        return;
    }
    filePart.setBodyDevice(file);
    file->setParent(multi); // multi析构时释放

    multi->append(filePart);

    nerReply = nerNam->post(req, multi);
    multi->setParent(nerReply); // reply完成后释放

    connect(nerReply, &QNetworkReply::readyRead, this, &Widget::onNerReadyRead);
    connect(nerReply, &QNetworkReply::finished, this, &Widget::onNerFinished);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(nerReply, &QNetworkReply::errorOccurred, this, &Widget::onNerError);
#else
    connect(nerReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &Widget::onNerError);
#endif
}

void Widget::onNerReadyRead()
{
    if (!nerReply) return;
    const QByteArray chunk = nerReply->readAll();
    if (chunk.isEmpty()) return;
    nerBuffer.append(chunk);
    const QString text = QString::fromUtf8(chunk);
    if (!text.isEmpty()) {
        Q_EMIT llmTokens(text, false);
        resetIdleTimer();
    }
}

void Widget::onNerFinished()
{
    llmStreamFinished = true;
    Q_EMIT llmTokens(QString(), true);
    if (nerReply) {
        nerReply->deleteLater();
        nerReply = nullptr;
    }
}

void Widget::onNerError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    const QString err = nerReply ? nerReply->errorString() : QStringLiteral("unknown error");
    Q_EMIT llmTokens(QStringLiteral("[网络错误] ") + err + "\n", true);
}

void Widget::onLlmTokens(const QString& text, bool isFinal)
{
    // 如果上一轮已结束且收到新文本，则清空显示，保证“每次只显示一次的回复”
    if (llmStreamFinished && !text.isEmpty()) {
        llmPending.clear();
        llmDisplayed.clear();
        llmStreamFinished = false;
        updateLlmDisplay();
    }
    if (!text.isEmpty()) {
        llmPending.append(text);
    }
    if (isFinal) {
        llmStreamFinished = true;
    }
    if (!llmTypingTimer->isActive()) {
        llmTypingTimer->start();
    }
}

void Widget::onTypingTick()
{
    if (llmPending.isEmpty()) {
        if (llmStreamFinished) {
            llmTypingTimer->stop();
        }
        return;
    }
    const int n = qMin(llmCharsPerTick, llmPending.size());
    const QString chunk = llmPending.left(n);
    llmPending.remove(0, n);
    llmDisplayed.append(chunk);
    updateLlmDisplay();
}

void Widget::updateLlmDisplay()
{
    // 框内仅显示内容，前缀在框上一行标签中
    llmEdit->setPlainText(llmDisplayed);
    
    // 将光标移动到文末并确保可见，避免首次刷新不显示的问题
    QTextCursor c = llmEdit->textCursor();
    c.movePosition(QTextCursor::End);
    llmEdit->setTextCursor(c);
    llmEdit->ensureCursorVisible();
    
    // 在布局完成后，将滚动条滚到最底：
    // - 当文本未超过两行时，最大值≈0，等价于正常显示；
    // - 当文本超过两行（出现第3行）时，自动滚动到底部，仅显示最新两行；
    QTimer::singleShot(0, this, [this]() {
        if (auto *bar = llmEdit->verticalScrollBar()) {
            bar->setValue(bar->maximum());
        }
    });
}

void Widget::updateAsrText(const QString& text, bool isFinal)
{
    Q_UNUSED(isFinal);
    // 框内仅显示内容，前缀在框上一行标签中
    asrEdit->setPlainText(text);
    resetIdleTimer();
}

// ==================== 新增：眨眼带回调实现 ====================
void Widget::blinkOnceAsChangeExpression(const std::function<void()>& callback)
{
    // 在0.5秒内切换三帧
    if (transitionPixmap.isNull() || closedPixmap.isNull()) {
        if (callback) callback();
        return; // 资源缺失
    }

    faceLabel->setPixmap(transitionPixmap);
    QTimer::singleShot(100, this, [this, callback]() {
        faceLabel->setPixmap(closedPixmap);
        QTimer::singleShot(100, this, [this, callback]() {
            faceLabel->setPixmap(transitionPixmap);
            QTimer::singleShot(100, this, [this, callback]() {
                // 根据是否有回调决定是否睁眼
                if (callback) {
                    // 直接执行回调，不再显示睁眼帧
                    callback();
                } else {
                    // 无回调时属于普通眨眼，恢复睁眼帧
                    faceLabel->setPixmap(openPixmap);
                }

                // 重新启动随机眨眼（仅当当前表情允许眨眼）
                if (blinkTimer && currentExpression != ExpressionType::Sleep && currentExpression != ExpressionType::Warning) {
                    blinkTimer->start(randomBlinkIntervalMs());
                }
            });
        });
    });
}
// 新增：onBlinkTimeout 实现（保持信号槽兼容）
void Widget::onBlinkTimeout()
{
    blinkOnceAsChangeExpression(nullptr);
    // 重新启动随机眨眼定时器
    if (blinkTimer) {
        blinkTimer->start(randomBlinkIntervalMs());
    }
}

// ========= 新增：空闲定时器槽函数 =========
void Widget::onIdleTimeout()
{
    // 休眠模式：停止随机眨眼定时器
    if (blinkTimer && blinkTimer->isActive()) {
        blinkTimer->stop();
    }
    // 播放眨眼动画并在结束后切换为 Sleep
    blinkOnceAsChangeExpression([this]() {
        setExpressionBackground(ExpressionType::Sleep);
    });
}

void Widget::resetIdleTimer()
{
    if(idleTimer){
        idleTimer->stop();
        idleTimer->start();
    }
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (currentMode == Mode::Expression) {
        // 进入界面模式
        currentMode = Mode::Interface;
        if (!interfaceWidget) {
            interfaceWidget = new InterfaceWidget(this);
            connect(interfaceWidget, &InterfaceWidget::backClicked, this, [this]() {
                // 返回表情模式
                currentMode = Mode::Expression;
                interfaceWidget->hide();
                setExpressionBackground(ExpressionType::Normal);
                resetIdleTimer();
            });
        }
        interfaceWidget->setGeometry(this->rect());
        interfaceWidget->show();
    }
}
