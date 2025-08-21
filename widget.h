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
    void onInterpolationSliderChanged(int value);
    void onFromExpressionChanged(int index);
    void onToExpressionChanged(int index);
    void playInterpolationAnimation();

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
};
#endif // WIDGET_H
