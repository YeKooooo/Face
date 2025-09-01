/**
 * EmotionOutput接口使用示例
 * 展示如何在实际应用中集成emotion_output功能
 */

#include "widget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QTimer>

class EmotionOutputDemo : public QWidget
{
    Q_OBJECT
    
public:
    EmotionOutputDemo(QWidget* parent = nullptr) : QWidget(parent)
    {
        setupUI();
        setupConnections();
        
        // 创建表情控制widget
        m_emotionWidget = new Widget();
        m_emotionWidget->show();
    }
    
private Q_SLOTS:
    void onTestCaringClicked() {
        QString jsonData = R"({
            "transcription": "你知道我是谁吗？",
            "speaker": {
                "id": "USR1755675801",
                "name": "张三",
                "status": "registered",
                "confidence": 0.75
            },
            "emotion_output": {
                "expression_type": "caring",
                "duration_ms": 3000,
                "trigger_reason": "user_identity_inquiry"
            }
        })";
        
        m_logTextEdit->append("发送关怀表情指令...");
        m_emotionWidget->processEmotionOutput(jsonData);
    }
    
    void onTestAlertClicked() {
        QString jsonData = R"({
            "transcription": "血压异常！",
            "emotion_output": {
                "expression_type": "alert",
                "duration_ms": 5000,
                "trigger_reason": "health_emergency"
            }
        })";
        
        m_logTextEdit->append("发送警觉表情指令...");
        m_emotionWidget->processEmotionOutput(jsonData);
    }
    
    void onTestHappyClicked() {
        // 使用直接结构体方式
        EmotionOutput emotionData("happy", 2500, "medication_completed");
        
        m_logTextEdit->append("发送开心表情指令（直接结构体）...");
        m_emotionWidget->processEmotionOutput(emotionData);
    }
    
    void onTestEncouragingClicked() {
        QString jsonData = R"({
            "emotion_output": {
                "expression_type": "encouraging",
                "duration_ms": 4000,
                "trigger_reason": "treatment_motivation"
            }
        })";
        
        m_logTextEdit->append("发送鼓励表情指令...");
        m_emotionWidget->processEmotionOutput(jsonData);
    }
    
    void onTestPermanentClicked() {
        QString jsonData = R"({
            "emotion_output": {
                "expression_type": "neutral",
                "duration_ms": -1,
                "trigger_reason": "permanent_state_test"
            }
        })";
        
        m_logTextEdit->append("发送永久中性表情指令...");
        m_emotionWidget->processEmotionOutput(jsonData);
    }
    
    void runAutoTest() {
        m_logTextEdit->append("\n=== 开始自动测试序列 ===");
        
        // 测试序列
        QTimer::singleShot(1000, this, &EmotionOutputDemo::onTestCaringClicked);
        QTimer::singleShot(5000, this, &EmotionOutputDemo::onTestEncouragingClicked);
        QTimer::singleShot(10000, this, &EmotionOutputDemo::onTestAlertClicked);
        QTimer::singleShot(16000, this, &EmotionOutputDemo::onTestHappyClicked);
        
        QTimer::singleShot(19000, [this]() {
            m_logTextEdit->append("\n=== 自动测试完成 ===");
        });
    }
    
private:
    void setupUI() {
        setWindowTitle("EmotionOutput API 演示程序");
        resize(400, 500);
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // 标题
        QLabel* titleLabel = new QLabel("智能用药提醒机器人表情控制演示");
        titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px;");
        layout->addWidget(titleLabel);
        
        // 按钮组
        m_caringButton = new QPushButton("关怀表情 (3秒)");
        m_alertButton = new QPushButton("警觉表情 (5秒)");
        m_happyButton = new QPushButton("开心表情 (2.5秒)");
        m_encouragingButton = new QPushButton("鼓励表情 (4秒)");
        m_permanentButton = new QPushButton("永久中性表情");
        m_autoTestButton = new QPushButton("运行自动测试");
        
        m_autoTestButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
        
        layout->addWidget(m_caringButton);
        layout->addWidget(m_alertButton);
        layout->addWidget(m_happyButton);
        layout->addWidget(m_encouragingButton);
        layout->addWidget(m_permanentButton);
        layout->addWidget(m_autoTestButton);
        
        // 日志区域
        QLabel* logLabel = new QLabel("操作日志:");
        layout->addWidget(logLabel);
        
        m_logTextEdit = new QTextEdit();
        m_logTextEdit->setMaximumHeight(200);
        m_logTextEdit->append("EmotionOutput API 演示程序已启动");
        m_logTextEdit->append("点击按钮测试不同的表情切换功能");
        layout->addWidget(m_logTextEdit);
    }
    
    void setupConnections() {
        connect(m_caringButton, &QPushButton::clicked, this, &EmotionOutputDemo::onTestCaringClicked);
        connect(m_alertButton, &QPushButton::clicked, this, &EmotionOutputDemo::onTestAlertClicked);
        connect(m_happyButton, &QPushButton::clicked, this, &EmotionOutputDemo::onTestHappyClicked);
        connect(m_encouragingButton, &QPushButton::clicked, this, &EmotionOutputDemo::onTestEncouragingClicked);
        connect(m_permanentButton, &QPushButton::clicked, this, &EmotionOutputDemo::onTestPermanentClicked);
        connect(m_autoTestButton, &QPushButton::clicked, this, &EmotionOutputDemo::runAutoTest);
    }
    
private:
    Widget* m_emotionWidget;
    QPushButton* m_caringButton;
    QPushButton* m_alertButton;
    QPushButton* m_happyButton;
    QPushButton* m_encouragingButton;
    QPushButton* m_permanentButton;
    QPushButton* m_autoTestButton;
    QTextEdit* m_logTextEdit;
};

// 使用示例的main函数
/*
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    EmotionOutputDemo demo;
    demo.show();
    
    return a.exec();
}
*/

#include "usage_example.moc"