#include "widget.h"
#include "ui_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGraphicsOpacityEffect>
#include <QFont>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , currentExpression(ExpressionType::Neutral)
    , isAnimating(false)
{
    ui->setupUi(this);
    setWindowTitle("智能用药提醒机器人表情系统");
    resize(600, 400);
    
    initializeExpressions();
    setupFaceDisplay();
    createAnimations();
}

Widget::~Widget()
{
    cleanupAnimations();
    delete ui;
}

void Widget::initializeExpressions()
{
    // 初始化表情数据
    expressions[ExpressionType::Happy] = {"😊", "开心", "#FFD700", "成功完成用药、健康状况良好"};
    expressions[ExpressionType::Caring] = {"🤗", "关怀", "#4CAF50", "用药提醒、健康关怀"};
    expressions[ExpressionType::Concerned] = {"😟", "担忧", "#FF9800", "延迟用药、健康指标异常"};
    expressions[ExpressionType::Encouraging] = {"💪", "鼓励", "#2196F3", "激励坚持治疗、克服困难"};
    expressions[ExpressionType::Alert] = {"⚠️", "警觉", "#FF6B6B", "紧急情况、重要提醒"};
    expressions[ExpressionType::Sad] = {"😢", "难过", "#607D8B", "身体不适、治疗效果不佳"};
    expressions[ExpressionType::Neutral] = {"😐", "中性", "#9E9E9E", "默认状态、日常交互"};
}

void Widget::cleanupAnimations()
{
    if (expressionAnimation) {
        expressionAnimation->stop();
        expressionAnimation->clear();
    }
}

void Widget::setupFaceDisplay()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建表情显示标签
    ExpressionData neutralData = expressions[currentExpression];
    faceLabel = new QLabel(neutralData.emoji, this);
    faceLabel->setAlignment(Qt::AlignCenter);
    QFont font = faceLabel->font();
    font.setPointSize(80);
    faceLabel->setFont(font);
    faceLabel->setStyleSheet(QString("QLabel { color: #333; background-color: %1; border-radius: 15px; padding: 30px; }").arg(neutralData.color));
    
    // 创建按钮网格布局
    QGridLayout *buttonLayout = new QGridLayout();
    
    // 创建所有表情按钮
    QList<ExpressionType> expressionOrder = {
        ExpressionType::Happy, ExpressionType::Caring, ExpressionType::Encouraging,
        ExpressionType::Neutral, ExpressionType::Concerned, ExpressionType::Alert, ExpressionType::Sad
    };
    
    int row = 0, col = 0;
    for (ExpressionType type : expressionOrder) {
        ExpressionData data = expressions[type];
        QPushButton *button = new QPushButton(QString("%1 %2").arg(data.emoji, data.name), this);
        
        // 设置按钮样式
        QString buttonStyle = QString(
            "QPushButton { "
            "font-size: 14px; "
            "padding: 8px 12px; "
            "background-color: %1; "
            "color: white; "
            "border: none; "
            "border-radius: 8px; "
            "min-width: 120px; "
            "} "
            "QPushButton:hover { "
            "background-color: %2; "
            "transform: scale(1.05); "
            "} "
            "QPushButton:pressed { "
            "background-color: %3; "
            "}"
        ).arg(data.color, 
              QString(data.color).replace("#", "#CC"), // 悬停时稍微透明
              QString(data.color).replace("#", "#AA")); // 按下时更透明
        
        button->setStyleSheet(buttonStyle);
        button->setToolTip(data.description);
        
        // 存储按钮引用
        expressionButtons[type] = button;
        
        // 连接信号槽
        connect(button, &QPushButton::clicked, this, &Widget::switchToExpression);
        
        // 添加到网格布局
        buttonLayout->addWidget(button, row, col);
        col++;
        if (col >= 3) {
            col = 0;
            row++;
        }
    }
    
    // 添加说明标签
    QLabel *infoLabel = new QLabel("点击按钮切换机器人表情，每种表情适用于不同的用药提醒场景", this);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; margin: 10px; }");
    
    mainLayout->addWidget(faceLabel, 2);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(infoLabel);
    
    setLayout(mainLayout);
}

void Widget::createAnimations()
{
    // 创建缩放动画
    scaleAnimation = new QPropertyAnimation(faceLabel, "geometry", this);
    scaleAnimation->setDuration(300);
    scaleAnimation->setEasingCurve(QEasingCurve::OutBounce);
    
    // 创建透明度动画
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(this);
    faceLabel->setGraphicsEffect(opacityEffect);
    opacityAnimation = new QPropertyAnimation(opacityEffect, "opacity", this);
    opacityAnimation->setDuration(200);
    
    // 创建序列动画组
    expressionAnimation = new QSequentialAnimationGroup(this);
    
    // 连接动画完成信号
    connect(expressionAnimation, &QSequentialAnimationGroup::finished, this, &Widget::onAnimationFinished);
}

void Widget::animateToExpression(ExpressionType type)
{
    if (isAnimating || type == currentExpression) {
        return;
    }
    
    isAnimating = true;
    ExpressionData newExpressionData = expressions[type];
    
    // 停止并清除之前的动画
    if (expressionAnimation->state() == QAbstractAnimation::Running) {
        expressionAnimation->stop();
    }
    expressionAnimation->clear();
    
    // 获取当前几何信息
    QRect currentGeometry = faceLabel->geometry();
    QRect shrinkGeometry = currentGeometry;
    shrinkGeometry.setWidth(currentGeometry.width() * 0.85);
    shrinkGeometry.setHeight(currentGeometry.height() * 0.85);
    shrinkGeometry.moveCenter(currentGeometry.center());
    
    // 第一阶段：缩小并淡出
    QPropertyAnimation *shrinkAnim = new QPropertyAnimation(faceLabel, "geometry", expressionAnimation);
    shrinkAnim->setDuration(200);
    shrinkAnim->setStartValue(currentGeometry);
    shrinkAnim->setEndValue(shrinkGeometry);
    shrinkAnim->setEasingCurve(QEasingCurve::InQuad);
    
    QPropertyAnimation *fadeOutAnim = new QPropertyAnimation(faceLabel->graphicsEffect(), "opacity", expressionAnimation);
    fadeOutAnim->setDuration(200);
    fadeOutAnim->setStartValue(1.0);
    fadeOutAnim->setEndValue(0.3);
    
    QParallelAnimationGroup *shrinkGroup = new QParallelAnimationGroup(expressionAnimation);
    shrinkGroup->addAnimation(shrinkAnim);
    shrinkGroup->addAnimation(fadeOutAnim);
    
    // 第二阶段：更换表情和颜色
    QPropertyAnimation *changeExpression = new QPropertyAnimation(expressionAnimation);
    changeExpression->setDuration(100);
    connect(changeExpression, &QPropertyAnimation::finished, [this, type, newExpressionData]() {
        faceLabel->setText(newExpressionData.emoji);
        faceLabel->setStyleSheet(QString("QLabel { color: #333; background-color: %1; border-radius: 15px; padding: 30px; }").arg(newExpressionData.color));
        currentExpression = type;
    });
    
    // 第三阶段：放大并淡入
    QPropertyAnimation *expandAnim = new QPropertyAnimation(faceLabel, "geometry", expressionAnimation);
    expandAnim->setDuration(300);
    expandAnim->setEasingCurve(QEasingCurve::OutBounce);
    expandAnim->setStartValue(shrinkGeometry);
    expandAnim->setEndValue(currentGeometry);
    
    QPropertyAnimation *fadeInAnim = new QPropertyAnimation(faceLabel->graphicsEffect(), "opacity", expressionAnimation);
    fadeInAnim->setDuration(250);
    fadeInAnim->setStartValue(0.3);
    fadeInAnim->setEndValue(1.0);
    
    QParallelAnimationGroup *expandGroup = new QParallelAnimationGroup(expressionAnimation);
    expandGroup->addAnimation(expandAnim);
    expandGroup->addAnimation(fadeInAnim);
    
    // 组合动画序列
    expressionAnimation->addAnimation(shrinkGroup);
    expressionAnimation->addAnimation(changeExpression);
    expressionAnimation->addAnimation(expandGroup);
    
    // 开始动画
    expressionAnimation->start();
}

void Widget::switchToExpression()
{
    QPushButton *senderButton = qobject_cast<QPushButton*>(sender());
    if (!senderButton) return;
    
    // 查找对应的表情类型
    for (auto it = expressionButtons.begin(); it != expressionButtons.end(); ++it) {
        if (it.value() == senderButton) {
            animateToExpression(it.key());
            break;
        }
    }
}

void Widget::onAnimationFinished()
{
    isAnimating = false;
}

