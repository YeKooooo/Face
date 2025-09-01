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

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

// 表情类型枚举
enum class ExpressionType {
    Happy,      // 开心
    Caring,     // 关怀
    Concerned,  // 担忧
    Encouraging,// 鼓励
    Alert,      // 警觉
    Sad,        // 难过
    Neutral     // 中性
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
    
private Q_SLOTS:
    void switchToExpression();
    void onAnimationFinished();
    void onInterpolationSliderChanged(int value);
    void onFromExpressionChanged(int index);
    void onToExpressionChanged(int index);
    void playInterpolationAnimation();
    void playImageSequenceAnimation();
    void onImageAnimationStep();
    void onExpressionDurationTimeout();

private:
    void setupFaceDisplay();
    void createAnimations();
    void animateToExpression(ExpressionType type);
    void initializeExpressions();
    void cleanupAnimations();
    void setupInterpolationUI();
    void initializeExpressionParams();
    ExpressionParams interpolateExpressions(const ExpressionParams& from, 
                                          const ExpressionParams& to, 
                                          double t);
    void applyExpressionParams(const ExpressionParams& params);
    
    // 图像序列相关函数
    void loadImageSequences();
    void preloadImageSequence(const QString& sequenceName);
    QString getSequencePath(ExpressionType from, ExpressionType to);
    QString expressionTypeToString(ExpressionType type);
    ExpressionType stringToExpressionType(const QString& typeString);
    void switchToExpressionWithImages(ExpressionType targetType);
    EmotionOutput parseEmotionOutputJson(const QString& jsonString);
    void logEmotionTrigger(const QString& reason, ExpressionType type);
    
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
    
    // 插值控件
    QComboBox* fromExpressionCombo;
    QComboBox* toExpressionCombo;
    QSlider* interpolationSlider;
    QPushButton* playAnimationButton;
    QSpinBox* animationSpeedSpinBox;
    
    // 插值状态
    ExpressionType fromExpression;
    ExpressionType toExpression;
    QTimer* interpolationTimer;
    int interpolationStep;
    int maxInterpolationSteps;
    
    // 图像序列相关成员变量
    QMap<QString, QList<QPixmap>> imageSequenceCache;
    QTimer* imageAnimationTimer;
    QStringList currentImageSequence;
    int currentImageFrame;
    QString interpolationBasePath;
    bool useImageSequences; // 优先采用图像序列模式
    QPushButton* toggleModeButton;
    
    // EmotionOutput相关成员
    QTimer* expressionDurationTimer;
    EmotionOutput currentEmotionOutput;
    ExpressionType previousExpression;
    
    // Socket服务器相关成员
    QTcpServer* tcpServer;
    QList<QTcpSocket*> clientSockets;
    quint16 serverPort;
    bool isServerRunning;
    
    // Socket服务器UI控件
    QPushButton* startServerButton;
    QPushButton* stopServerButton;
    QLabel* serverStatusLabel;
    QLabel* clientCountLabel;
    QSpinBox* portSpinBox;
    QLabel* ipAddressLabel;
    
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
    void setupSocketServerUI();
    void updateServerStatus();
    void updateClientCount();
    
private Q_SLOTS:
    // Socket UI相关槽函数
    void onStartServerClicked();
    void onStopServerClicked();
    void onPortChanged(int port);
};
#endif // WIDGET_H
