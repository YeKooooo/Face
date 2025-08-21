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

private Q_SLOTS:
    void switchToExpression();
    void onAnimationFinished();

private:
    void setupFaceDisplay();
    void createAnimations();
    void animateToExpression(ExpressionType type);
    void initializeExpressions();
    void cleanupAnimations();
    
    Ui::Widget *ui;
    
    // 表情显示相关
    QLabel *faceLabel;
    QMap<ExpressionType, QPushButton*> expressionButtons;
    QMap<ExpressionType, ExpressionData> expressions;
    
    // 动画相关
    QPropertyAnimation *scaleAnimation;
    QPropertyAnimation *opacityAnimation;
    QSequentialAnimationGroup *expressionAnimation;
    
    // 状态管理
    ExpressionType currentExpression;
    bool isAnimating;
};
#endif // WIDGET_H
