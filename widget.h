#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QTimer>
#include <QSlider>
#include <QComboBox>
#include <QSpinBox>
#include <QPixmap>
#include <QDir>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
// 新增：HTTP 流式接入
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
// 新增：LLM显示采用QPlainTextEdit以支持滚动条
#include <QPlainTextEdit>
#include <QGroupBox>
#include "interfacewidget.h"
#include "registrationwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

// 表情类型枚举
enum class ExpressionType {
    Normal,     // 默认/普通
    Happy,      // 开心
    Sad,        // 难过
    Warning,    // 警示
    Sleep       // 休眠模式
};

struct ExpressionParams {
    QColor backgroundColor;
    QColor textColor;
    double scale;
    double opacity;
    QString emoji;
    QString description;
    
    ExpressionParams() : scale(1.0), opacity(1.0) {}
    ExpressionParams(const QColor& bgColor, const QColor& txtColor, 
                    double s, double o, const QString& e, const QString& d)
        : backgroundColor(bgColor), textColor(txtColor), scale(s), 
          opacity(o), emoji(e), description(d) {}
};

// EmotionOutput数据结构
struct EmotionOutput {
    QString expression_type;    // 表情类型
    int duration_ms;           // 持续时间(毫秒)
    QString trigger_reason;    // 触发原因
    
    EmotionOutput() : duration_ms(3000) {}
    EmotionOutput(const QString& type, int duration, const QString& reason)
        : expression_type(type), duration_ms(duration), trigger_reason(reason) {}
};

// 表情数据结构
struct ExpressionData {
    QString emoji;
    QString name;
    QString color;
    QString description;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

public Q_SLOTS:
    // EmotionOutput接口
    void processEmotionOutput(const QString& jsonString);
    void processEmotionOutput(const EmotionOutput& emotionData);
    
    // LLM/ASR 流式HTTP启动（与NerController匹配）
    void startNerStreamJson(const QUrl& baseUrl, const QString& memoryId, const QString& text);
    void startNerStreamMultipart(const QUrl& baseUrl, const QString& memoryId, const QString& imagePath);

Q_SIGNALS:
    // 流式文本信号：在UI线程内消费
    void llmTokens(const QString& text, bool isFinal);
    void asrText(const QString& text, bool isFinal);
    
private Q_SLOTS:
    void onExpressionDurationTimeout();

    // LLM/ASR 显示槽
    void onLlmTokens(const QString& text, bool isFinal);
    void onTypingTick();
    void onNerReadyRead();
    void onNerFinished();
    void onNerError(QNetworkReply::NetworkError code);
    void updateAsrText(const QString& text, bool isFinal);

    // 眨眼动画（带回调）：动画完成后执行回调
    void blinkOnceAsChangeExpression(const std::function<void()>& callback);
    // 眨眼定时器触发槽：播放后重新定时
    void onBlinkTimeout();

private:
    void setupFaceDisplay();
    void cleanupAnimations();
    // 图像序列相关函数
    void preloadImageSequence(const QString& sequenceName);
    QString expressionTypeToString(ExpressionType type);
    ExpressionType stringToExpressionType(const QString& typeString);
    EmotionOutput parseEmotionOutputJson(const QString& jsonString);
    void logEmotionTrigger(const QString& reason, ExpressionType type);
    // 根据表达类型设置背景
    void setExpressionBackground(ExpressionType type);

    // 更新LLM文本显示（仅保留1-2行可见，超出出现滚动条并自动滚动）
    void updateLlmDisplay();
    
    Ui::Widget *ui;
    
    // 表情显示相关
    QLabel *faceLabel;
    QMap<ExpressionType, QPushButton*> expressionButtons;
    QMap<ExpressionType, ExpressionData> expressions;
    QMap<ExpressionType, ExpressionParams> expressionParams;
    
    // 动画相关
    QPropertyAnimation *scaleAnimation;
    QPropertyAnimation *opacityAnimation;
    QSequentialAnimationGroup *expressionAnimation;
    
    // 状态管理
    ExpressionType currentExpression;
    bool isAnimating;

    ExpressionType fromExpression;
    ExpressionType toExpression;

    
    // 图像序列相关成员变量
    QMap<QString, QList<QPixmap>> imageSequenceCache;
    QTimer* imageAnimationTimer;
    QStringList currentImageSequence;
    int currentImageFrame;
    QString interpolationBasePath;
    bool useImageSequences; // 优先采用图像序列模式
    int imageAnimationIntervalMs; // 新增：图像序列播放间隔(ms)
    
    // EmotionOutput相关成员
    QTimer* expressionDurationTimer;
    EmotionOutput currentEmotionOutput;
    ExpressionType previousExpression;
    
    // Socket服务器相关成员
    QTcpServer* tcpServer;
    QList<QTcpSocket*> clientSockets;
    quint16 serverPort;
    bool isServerRunning;

    // 眨眼相关成员
    QTimer* blinkTimer;
    QTimer* idleTimer; // 新增：空闲定时器，用于20秒无输入时切换至休眠
    QPixmap openPixmap;
    QPixmap transitionPixmap;
    QPixmap closedPixmap;
    // LLM/ASR 文本显示与HTTP接入成员
    QLabel* asrLabel;
    QPlainTextEdit* llmEdit; // 替换为可滚动文本框
    QLabel* llmPrefixLabel; // "机器人："固定前缀
    QNetworkAccessManager* nerNam;
    QNetworkReply* nerReply;
    QByteArray nerBuffer;
    QTimer* llmTypingTimer;
    QString llmPending;
    QString llmDisplayed;
    bool llmStreamFinished;
    int llmCharsPerTick;
    QPlainTextEdit* asrEdit; // 新增ASR编辑框指针
private Q_SLOTS:
    // Socket相关槽函数
    void onNewConnection();
    void onClientDisconnected();
    void onDataReceived();
    void onSocketError(QAbstractSocket::SocketError error);
    
private:
    // Socket相关私有函数
    void initializeSocketServer();
    void startSocketServer(quint16 port = 8888);
    void stopSocketServer();
    void processSocketData(const QByteArray& data);
    QString getLocalIPAddress();

    
private Q_SLOTS:

    void onIdleTimeout();
    void resetIdleTimer();

protected:
    void mousePressEvent(QMouseEvent *event) override;

    enum class Mode { Expression, Interface, Registration };

    Mode currentMode;

    InterfaceWidget *interfaceWidget;
    RegistrationWidget *registrationWidget;

    QGroupBox* streamGroup;
};
#endif // WIDGET_H
