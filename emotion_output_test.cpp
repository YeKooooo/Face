#include "widget.h"
#include <QApplication>
#include <QTimer>
#include <QDebug>

/**
 * EmotionOutput接口测试示例
 * 演示如何使用emotion_output字段来控制机器人表情
 */

class EmotionOutputTester : public QObject
{
    Q_OBJECT
    
public:
    EmotionOutputTester(Widget* widget) : m_widget(widget) {}
    
    void runTests() {
        // 测试1: 用户身份询问场景
        testUserIdentityInquiry();
        
        // 测试2: 用药提醒场景 (延迟3秒)
        QTimer::singleShot(4000, this, &EmotionOutputTester::testMedicationReminder);
        
        // 测试3: 紧急情况场景 (延迟7秒)
        QTimer::singleShot(8000, this, &EmotionOutputTester::testHealthEmergency);
        
        // 测试4: 直接使用EmotionOutput结构 (延迟12秒)
        QTimer::singleShot(13000, this, &EmotionOutputTester::testDirectEmotionOutput);
    }
    
private Q_SLOTS:
    void testUserIdentityInquiry() {
        qDebug() << "\n=== 测试1: 用户身份询问场景 ===";
        
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
        
        m_widget->processEmotionOutput(jsonData);
    }
    
    void testMedicationReminder() {
        qDebug() << "\n=== 测试2: 用药提醒场景 ===";
        
        QString jsonData = R"({
            "transcription": "该吃药了",
            "emotion_output": {
                "expression_type": "encouraging",
                "duration_ms": 4000,
                "trigger_reason": "medication_reminder"
            }
        })";
        
        m_widget->processEmotionOutput(jsonData);
    }
    
    void testHealthEmergency() {
        qDebug() << "\n=== 测试3: 紧急情况场景 ===";
        
        QString jsonData = R"({
            "transcription": "血压异常！",
            "emotion_output": {
                "expression_type": "alert",
                "duration_ms": 6000,
                "trigger_reason": "health_emergency"
            }
        })";
        
        m_widget->processEmotionOutput(jsonData);
    }
    
    void testDirectEmotionOutput() {
        qDebug() << "\n=== 测试4: 直接使用EmotionOutput结构 ===";
        
        EmotionOutput emotionData("happy", 2500, "medication_completed");
        m_widget->processEmotionOutput(emotionData);
    }
    
private:
    Widget* m_widget;
};

// 在main.cpp中使用示例:
/*
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    
    // 创建测试器并运行测试
    EmotionOutputTester tester(&w);
    QTimer::singleShot(1000, &tester, &EmotionOutputTester::runTests);
    
    return a.exec();
}
*/

#include "emotion_output_test.moc"