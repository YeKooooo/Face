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

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , currentExpression(ExpressionType::Neutral)
    , isAnimating(false)
    , fromExpression(ExpressionType::Happy)
    , toExpression(ExpressionType::Sad)
    , interpolationTimer(new QTimer(this))
    , interpolationStep(0)
    , maxInterpolationSteps(50)
{
    ui->setupUi(this);
    setWindowTitle("智能用药提醒机器人表情系统 - 插值版");
    resize(800, 600);
    
    initializeExpressions();
    initializeExpressionParams();
    setupFaceDisplay();
    setupInterpolationUI();
    createAnimations();
    
    // 连接插值定时器
    connect(interpolationTimer, &QTimer::timeout, this, [this]() {
        double t = static_cast<double>(interpolationStep) / maxInterpolationSteps;
        ExpressionParams params = interpolateExpressions(
            expressionParams[fromExpression], 
            expressionParams[toExpression], 
            t
        );
        applyExpressionParams(params);
        
        interpolationStep++;
        if (interpolationStep > maxInterpolationSteps) {
            interpolationTimer->stop();
            interpolationStep = 0;
        }
    });
}

Widget::~Widget()
{
    cleanupAnimations();
    if (interpolationTimer) {
        interpolationTimer->stop();
    }
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
        expressionAnimation = nullptr;
    }
}

void Widget::initializeExpressionParams()
{
    // 为每种表情定义可插值的参数
    expressionParams[ExpressionType::Happy] = ExpressionParams(
        QColor(144, 238, 144), QColor(0, 100, 0), 1.2, 1.0, "😊", "开心"
    );
    expressionParams[ExpressionType::Caring] = ExpressionParams(
        QColor(255, 182, 193), QColor(139, 69, 19), 1.1, 0.9, "🤗", "关怀"
    );
    expressionParams[ExpressionType::Concerned] = ExpressionParams(
        QColor(255, 165, 0), QColor(139, 69, 19), 0.9, 0.8, "😟", "担忧"
    );
    expressionParams[ExpressionType::Encouraging] = ExpressionParams(
        QColor(173, 216, 230), QColor(25, 25, 112), 1.3, 1.0, "💪", "鼓励"
    );
    expressionParams[ExpressionType::Alert] = ExpressionParams(
        QColor(255, 99, 71), QColor(139, 0, 0), 1.1, 1.0, "⚠️", "警示"
    );
    expressionParams[ExpressionType::Sad] = ExpressionParams(
        QColor(169, 169, 169), QColor(105, 105, 105), 0.8, 0.7, "😢", "悲伤"
    );
    expressionParams[ExpressionType::Neutral] = ExpressionParams(
        QColor(211, 211, 211), QColor(105, 105, 105), 1.0, 0.8, "😐", "中性"
    );
}

void Widget::setupInterpolationUI()
{
    // 创建插值控制面板
    QGroupBox* interpolationGroup = new QGroupBox("表情插值控制", this);
    QVBoxLayout* interpolationLayout = new QVBoxLayout(interpolationGroup);
    
    // 表情选择
    QHBoxLayout* expressionSelectLayout = new QHBoxLayout();
    
    QLabel* fromLabel = new QLabel("起始表情:");
    fromExpressionCombo = new QComboBox();
    QLabel* toLabel = new QLabel("目标表情:");
    toExpressionCombo = new QComboBox();
    
    // 填充下拉框
    QStringList expressionNames = {"开心", "关怀", "担忧", "鼓励", "警示", "悲伤", "中性"};
    fromExpressionCombo->addItems(expressionNames);
    toExpressionCombo->addItems(expressionNames);
    toExpressionCombo->setCurrentIndex(5); // 默认选择悲伤
    
    expressionSelectLayout->addWidget(fromLabel);
    expressionSelectLayout->addWidget(fromExpressionCombo);
    expressionSelectLayout->addWidget(toLabel);
    expressionSelectLayout->addWidget(toExpressionCombo);
    
    // 插值滑块
    QHBoxLayout* sliderLayout = new QHBoxLayout();
    QLabel* sliderLabel = new QLabel("插值比例:");
    interpolationSlider = new QSlider(Qt::Horizontal);
    interpolationSlider->setRange(0, 100);
    interpolationSlider->setValue(0);
    
    sliderLayout->addWidget(sliderLabel);
    sliderLayout->addWidget(interpolationSlider);
    
    // 动画控制
    QHBoxLayout* animationLayout = new QHBoxLayout();
    playAnimationButton = new QPushButton("播放插值动画");
    QLabel* speedLabel = new QLabel("速度(ms):");
    animationSpeedSpinBox = new QSpinBox();
    animationSpeedSpinBox->setRange(10, 1000);
    animationSpeedSpinBox->setValue(50);
    
    animationLayout->addWidget(playAnimationButton);
    animationLayout->addWidget(speedLabel);
    animationLayout->addWidget(animationSpeedSpinBox);
    
    interpolationLayout->addLayout(expressionSelectLayout);
    interpolationLayout->addLayout(sliderLayout);
    interpolationLayout->addLayout(animationLayout);
    
    // 连接信号槽
    connect(fromExpressionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Widget::onFromExpressionChanged);
    connect(toExpressionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Widget::onToExpressionChanged);
    connect(interpolationSlider, &QSlider::valueChanged,
            this, &Widget::onInterpolationSliderChanged);
    connect(playAnimationButton, &QPushButton::clicked,
            this, &Widget::playInterpolationAnimation);
    
    // 将插值面板添加到主布局
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->addWidget(interpolationGroup);
    }
}

ExpressionParams Widget::interpolateExpressions(const ExpressionParams& from, 
                                                const ExpressionParams& to, 
                                                double t)
{
    // 限制t在[0,1]范围内
    t = qBound(0.0, t, 1.0);
    
    // 使用平滑插值曲线（ease-in-out）
    t = t * t * (3.0 - 2.0 * t);
    
    ExpressionParams result;
    
    // 颜色插值
    result.backgroundColor = QColor(
        static_cast<int>(from.backgroundColor.red() + t * (to.backgroundColor.red() - from.backgroundColor.red())),
        static_cast<int>(from.backgroundColor.green() + t * (to.backgroundColor.green() - from.backgroundColor.green())),
        static_cast<int>(from.backgroundColor.blue() + t * (to.backgroundColor.blue() - from.backgroundColor.blue()))
    );
    
    result.textColor = QColor(
        static_cast<int>(from.textColor.red() + t * (to.textColor.red() - from.textColor.red())),
        static_cast<int>(from.textColor.green() + t * (to.textColor.green() - from.textColor.green())),
        static_cast<int>(from.textColor.blue() + t * (to.textColor.blue() - from.textColor.blue()))
    );
    
    // 数值插值
    result.scale = from.scale + t * (to.scale - from.scale);
    result.opacity = from.opacity + t * (to.opacity - from.opacity);
    
    // 表情符号和描述的切换（在中点切换）
    if (t < 0.5) {
        result.emoji = from.emoji;
        result.description = from.description;
    } else {
        result.emoji = to.emoji;
        result.description = to.description;
    }
    
    return result;
}

void Widget::applyExpressionParams(const ExpressionParams& params)
{
    if (!faceLabel) return;
    
    // 应用表情符号
    faceLabel->setText(params.emoji);
    
    // 应用样式
    QString styleSheet = QString(
        "QLabel {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 3px solid %3;"
        "    border-radius: 20px;"
        "    font-size: %4px;"
        "    font-weight: bold;"
        "    padding: 20px;"
        "    text-align: center;"
        "}"
    ).arg(params.backgroundColor.name())
     .arg(params.textColor.name())
     .arg(params.textColor.darker(150).name())
     .arg(static_cast<int>(48 * params.scale));
    
    faceLabel->setStyleSheet(styleSheet);
    
    // 应用透明度
    QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect();
    opacityEffect->setOpacity(params.opacity);
    faceLabel->setGraphicsEffect(opacityEffect);
}

void Widget::onInterpolationSliderChanged(int value)
{
    double t = static_cast<double>(value) / 100.0;
    ExpressionParams params = interpolateExpressions(
        expressionParams[fromExpression], 
        expressionParams[toExpression], 
        t
    );
    applyExpressionParams(params);
}

void Widget::onFromExpressionChanged(int index)
{
    fromExpression = static_cast<ExpressionType>(index);
    onInterpolationSliderChanged(interpolationSlider->value());
}

void Widget::onToExpressionChanged(int index)
{
    toExpression = static_cast<ExpressionType>(index);
    onInterpolationSliderChanged(interpolationSlider->value());
}

void Widget::playInterpolationAnimation()
{
    if (interpolationTimer->isActive()) {
        interpolationTimer->stop();
        playAnimationButton->setText("播放插值动画");
        return;
    }
    
    interpolationStep = 0;
    maxInterpolationSteps = 100; // 100步插值
    interpolationTimer->setInterval(animationSpeedSpinBox->value());
    interpolationTimer->start();
    playAnimationButton->setText("停止动画");
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

