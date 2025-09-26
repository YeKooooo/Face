#include "registrationwidget.h"
#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

const QString RegistrationWidget::GROUP_ID = "X16BAC";

RegistrationWidget::RegistrationWidget(QWidget *parent)
    : QWidget(parent)
    , stackedWidget(nullptr)
    , titleLabel(nullptr)
    , progressBar(nullptr)
    , prevButton(nullptr)
    , nextButton(nullptr)
    , finishButton(nullptr)
    , nameLineEdit(nullptr)
    , nameErrorLabel(nullptr)
    , recordingInstructionLabel(nullptr)
    , recordButton(nullptr)
    , recordStatusLabel(nullptr)
    , recordingProgressBar(nullptr)
    , recordingTimer(new QTimer(this))
    , audioRecorder(nullptr)
    , recordingInProgress(false)
    , usersScrollArea(nullptr)
    , usersWidget(nullptr)
    , usersLayout(nullptr)
    , relationComboBox(nullptr)
    , addRelationButton(nullptr)
    , skipRelationsButton(nullptr)
    , relationStatusLabel(nullptr)
    , completionInfoLabel(nullptr)
    , returnButton(nullptr)
    , currentStep(0)
    , networkManager(new QNetworkAccessManager(this))
    , currentReply(nullptr)
    , serverBaseUrl("http://81.69.221.200:8081")  // 公网服务器地址
{
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet("background-color:#1e1e1e;");
    
    // 强制初始化网络子系统
    qDebug() << "强制初始化Qt网络子系统...";
    
    // 启用网络访问
    networkManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
    
    // 开发板可能需要的额外网络配置
    networkManager->setProxy(QNetworkProxy::NoProxy);  // 禁用代理
    
    // 尝试强制刷新网络配置
    networkManager->clearAccessCache();
    networkManager->clearConnectionCache();
    
    qDebug() << "构造函数中网络管理器初始化完成:";
    qDebug() << "  - 网络访问状态:" << networkManager->networkAccessible();
    qDebug() << "  - 代理设置:" << networkManager->proxy().type();
    
    // 尝试强制设置多次
    for (int i = 0; i < 3; i++) {
        networkManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
        QCoreApplication::processEvents();
        qDebug() << "  - 第" << (i+1) << "次强制设置后状态:" << networkManager->networkAccessible();
    }
    
    // 初始化录音文件夹（使用Desktop路径）
    audioOutputDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/VoiceRegistration";
    QDir().mkpath(audioOutputDir);
    qDebug() << "录音文件保存路径:" << audioOutputDir;
    
    // 初始化关系选项
    relationOptions << "爸爸" << "妈妈" << "老公" << "老婆" 
                    << "儿子" << "女儿" << "哥哥" << "姐姐" 
                    << "弟弟" << "妹妹" << "爷爷" << "奶奶" 
                    << "外公" << "外婆" << "孙子" << "孙女";
    
    // 初始化录音提示文本
    recordingText = "请说一段话，内容可以是自我介绍、念一首诗或者随意聊天，时长约10-15秒，声音要清晰响亮。";
    
    setupUI();
    updateStepIndicator();
    updateButtonStates();
    
    // 连接录音定时器
    connect(recordingTimer, &QTimer::timeout, this, &RegistrationWidget::stopRecording);
    recordingTimer->setSingleShot(true);
}

RegistrationWidget::~RegistrationWidget()
{
    if (audioRecorder && audioRecorder->state() == QMediaRecorder::RecordingState) {
        audioRecorder->stop();
    }
    
    // 清理网络请求
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
        currentReply = nullptr;
    }
}

void RegistrationWidget::setupUI()
{
    setFixedSize(1280, 800);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 30, 50, 30);
    mainLayout->setSpacing(20);
    
    // 标题
    titleLabel = new QLabel("用户注册", this);
    titleLabel->setStyleSheet("color: #ffffff; font-size: 28px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // 按钮区域（移到顶部）
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);
    
    prevButton = new QPushButton("← 上一步", this);
    nextButton = new QPushButton("下一步 →", this);
    finishButton = new QPushButton("完成注册", this);
    
    QString buttonStyle = 
        "QPushButton {"
        "   color: #ffffff;"
        "   background: #3a3a3a;"
        "   border: none;"
        "   border-radius: 8px;"
        "   padding: 12px 24px;"
        "   font-size: 16px;"
        "   min-width: 120px;"
        "}"
        "QPushButton:hover {"
        "   background: #505050;"
        "}"
        "QPushButton:disabled {"
        "   background: #2a2a2a;"
        "   color: #666666;"
        "}";
    
    prevButton->setStyleSheet(buttonStyle);
    nextButton->setStyleSheet(buttonStyle);
    finishButton->setStyleSheet(buttonStyle.replace("#3a3a3a", "#4CAF50").replace("#505050", "#45a049"));
    
    finishButton->hide();
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(prevButton);
    buttonLayout->addWidget(nextButton);
    buttonLayout->addWidget(finishButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // 进度条（移到按钮下方）
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(25);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid #555;"
        "   border-radius: 8px;"
        "   background-color: #2d2d2d;"
        "   text-align: center;"
        "   color: #ffffff;"
        "   font-size: 14px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #4CAF50;"
        "   border-radius: 6px;"
        "}"
    );
    mainLayout->addWidget(progressBar);
    
    // 主内容区域
    stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(stackedWidget, 1);
    
    // 连接按钮信号
    connect(prevButton, &QPushButton::clicked, this, &RegistrationWidget::previousStep);
    connect(nextButton, &QPushButton::clicked, this, &RegistrationWidget::nextStep);
    connect(finishButton, &QPushButton::clicked, this, &RegistrationWidget::finishRegistration);
    
    // 设置各个步骤页面
    setupStep1();
    setupStep2();
    setupStep3();
    setupStep4();
}

void RegistrationWidget::setupStep1()
{
    QWidget *step1Widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(step1Widget);
    layout->setContentsMargins(150, 120, 150, 120);
    layout->setSpacing(50);
    
    // 标题
    QLabel *titleLabel = new QLabel();
    titleLabel->setText("用户注册");
    titleLabel->setStyleSheet("color: #ffffff; font-size: 32px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // 说明
    QLabel *descLabel = new QLabel();
    descLabel->setText("请输入您的姓名");
    descLabel->setStyleSheet("color: #cccccc; font-size: 18px;");
    descLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(descLabel);
    
    // 姓名输入框
    nameLineEdit = new QLineEdit();
    nameLineEdit->setPlaceholderText("请输入姓名");
    nameLineEdit->setStyleSheet(
        "QLineEdit {"
        "   padding: 8px 20px;"
        "   border: 2px solid #555;"
        "   border-radius: 8px;"
        "   background-color: #2d2d2d;"
        "   color: #ffffff;"
        "   font-size: 18px;"
        "}"
        "QLineEdit:focus {"
        "   border-color: #4CAF50;"
        "}"
    );
    nameLineEdit->setMaxLength(10);
    nameLineEdit->setAlignment(Qt::AlignCenter);
    nameLineEdit->setFixedHeight(34); // 设置固定高度，包含边框和padding
    layout->addWidget(nameLineEdit);
    
    // 提示信息
    QLabel *hintLabel = new QLabel();
    hintLabel->setText("2-10个字符");
    hintLabel->setStyleSheet("color: #888888; font-size: 18px;");
    hintLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(hintLabel);
    
    // 错误信息
    nameErrorLabel = new QLabel();
    nameErrorLabel->setStyleSheet("color: #ff6b6b; font-size: 20px; font-weight: bold;");
    nameErrorLabel->setAlignment(Qt::AlignCenter);
    nameErrorLabel->hide();
    layout->addWidget(nameErrorLabel);
    
    // 返回按钮
    QPushButton *backToMainButton = new QPushButton("返回主界面");
    backToMainButton->setStyleSheet(
        "QPushButton {"
        "   padding: 12px 24px;"
        "   background-color: #666666;"
        "   color: #ffffff;"
        "   border: none;"
        "   border-radius: 8px;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   min-height: 44px;"
        "   max-height: 44px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #777777;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #555555;"
        "}"
    );
    backToMainButton->setFixedHeight(44); // 设置固定高度
    
    QHBoxLayout *backButtonLayout = new QHBoxLayout();
    backButtonLayout->addStretch();
    backButtonLayout->addWidget(backToMainButton);
    backButtonLayout->addStretch();
    
    layout->addStretch();
    layout->addLayout(backButtonLayout);
    
    // 连接按钮信号
    connect(backToMainButton, &QPushButton::clicked, this, &RegistrationWidget::backToInterface);
    
    // 连接输入验证
    connect(nameLineEdit, &QLineEdit::textChanged, this, &RegistrationWidget::validateNameInput);
    
    stackedWidget->addWidget(step1Widget);
}

void RegistrationWidget::setupStep2()
{
    QWidget *step2Widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(step2Widget);
    layout->setContentsMargins(40, 30, 40, 30);
    layout->setSpacing(25);
    
    // 标题
    QLabel *titleLabel = new QLabel();
    titleLabel->setText("家庭关系设置");
    titleLabel->setStyleSheet("color: #ffffff; font-size: 32px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // 说明
    QLabel *descLabel = new QLabel();
    descLabel->setText("可选步骤，可直接跳过");
    descLabel->setStyleSheet("color: #e0e0e0; font-size: 18px;");
    descLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(descLabel);
    
    // 已设置关系显示
    relationStatusLabel = new QLabel();
    relationStatusLabel->setText("暂无设置关系");
    relationStatusLabel->setStyleSheet(
        "color: #ffffff; font-size: 16px; font-weight: bold; "
        "background: #2d2d2d; padding: 20px; border-radius: 8px; "
        "border: 2px solid #555;"
    );
    relationStatusLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    relationStatusLabel->setMinimumHeight(80);
    relationStatusLabel->setWordWrap(true);
    layout->addWidget(relationStatusLabel);
    
    // 用户列表
    QLabel *usersLabel = new QLabel();
    usersLabel->setText("已注册用户:");
    usersLabel->setStyleSheet("color: #ffffff; font-size: 20px; font-weight: bold;");
    layout->addWidget(usersLabel);
    
    usersScrollArea = new QScrollArea();
    usersScrollArea->setStyleSheet(
        "QScrollArea {"
        "   border: 2px solid #666;"
        "   border-radius: 8px;"
        "   background-color: #2d2d2d;"
        "}"
        "QScrollBar:vertical {"
        "   background: #3d3d3d;"
        "   width: 16px;"
        "   border-radius: 8px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: #666;"
        "   border-radius: 8px;"
        "}"
    );
    usersScrollArea->setWidgetResizable(true);
    usersScrollArea->setMinimumHeight(250);
    
    usersWidget = new QWidget();
    usersLayout = new QVBoxLayout(usersWidget);
    usersLayout->setSpacing(15);
    usersLayout->setContentsMargins(15, 15, 15, 15);
    usersScrollArea->setWidget(usersWidget);
    
    layout->addWidget(usersScrollArea);
    
    // 跳过按钮
    skipRelationsButton = new QPushButton();
    skipRelationsButton->setText("跳过关系设置");
    skipRelationsButton->setStyleSheet(
        "QPushButton {"
        "   color: #ffffff;"
        "   background: #666666;"
        "   border: 2px solid #888;"
        "   border-radius: 8px;"
        "   padding: 12px 24px;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background: #777777;"
        "   border-color: #999;"
        "}"
    );
    connect(skipRelationsButton, &QPushButton::clicked, this, &RegistrationWidget::skipRelations);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(skipRelationsButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    stackedWidget->addWidget(step2Widget);
}

void RegistrationWidget::setupStep3()
{
    QWidget *step3Widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(step3Widget);
    layout->setContentsMargins(50, 40, 50, 40);
    layout->setSpacing(30);
    
    // 标题
    QLabel *titleLabel = new QLabel();
    titleLabel->setText("语音录制");
    titleLabel->setStyleSheet("color: #ffffff; font-size: 32px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // 说明
    QLabel *descLabel = new QLabel();
    descLabel->setText("请录制一段12秒的清晰语音，用于声纹识别");
    descLabel->setStyleSheet("color: #e0e0e0; font-size: 18px;");
    descLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(descLabel);
    
    // 录制提示
    recordingInstructionLabel = new QLabel();
    recordingInstructionLabel->setText(recordingText);
    recordingInstructionLabel->setStyleSheet(
        "color: #ffffff; font-size: 16px; font-weight: bold; "
        "background: #2d2d2d; padding: 20px; border-radius: 8px; border: 2px solid #4CAF50;"
    );
    recordingInstructionLabel->setAlignment(Qt::AlignCenter);
    recordingInstructionLabel->setWordWrap(true);
    recordingInstructionLabel->setMinimumHeight(120);
    layout->addWidget(recordingInstructionLabel);
    
    // 录制容器
    QWidget *recordWidget = new QWidget();
    recordWidget->setStyleSheet(
        "QWidget {"
        "   background: #2d2d2d;"
        "   border: 1px solid #555;"
        "   border-radius: 12px;"
        "   padding: 30px;"
        "   margin: 10px;"
        "}"
    );
    recordWidget->setMinimumHeight(200);
    
    QVBoxLayout *recordLayout = new QVBoxLayout(recordWidget);
    recordLayout->setSpacing(20);
    recordLayout->setAlignment(Qt::AlignCenter);
    
    // 录制按钮
    recordButton = new QPushButton();
    recordButton->setText("🎤 开始录制");
    recordButton->setStyleSheet(
        "QPushButton {"
        "   color: #ffffff;"
        "   background: #4CAF50;"
        "   border: 3px solid #4CAF50;"
        "   border-radius: 12px;"
        "   padding: 15px 30px;"
        "   font-size: 20px;"
        "   font-weight: bold;"
        "   min-width: 200px;"
        "   min-height: 60px;"
        "}"
        "QPushButton:hover {"
        "   background: #45a049;"
        "   border-color: #45a049;"
        "}"
        "QPushButton:disabled {"
        "   background: #666;"
        "   color: #999;"
        "   border-color: #666;"
        "}"
    );
    
    connect(recordButton, &QPushButton::clicked, this, &RegistrationWidget::startRecording);
    recordLayout->addWidget(recordButton, 0, Qt::AlignCenter);
    
    // 进度条
    recordingProgressBar = new QProgressBar();
    recordingProgressBar->setRange(0, RECORDING_DURATION_MS);
    recordingProgressBar->setValue(0);
    recordingProgressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid #555;"
        "   border-radius: 8px;"
        "   background-color: #1e1e1e;"
        "   text-align: center;"
        "   color: #ffffff;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   min-height: 25px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #4CAF50;"
        "   border-radius: 6px;"
        "}"
    );
    recordingProgressBar->hide(); // 初始隐藏
    recordLayout->addWidget(recordingProgressBar);
    
    // 状态标签
    recordStatusLabel = new QLabel();
    recordStatusLabel->setText("准备录制");
    recordStatusLabel->setStyleSheet("color: #e0e0e0; font-size: 16px; font-weight: bold;");
    recordStatusLabel->setAlignment(Qt::AlignCenter);
    recordLayout->addWidget(recordStatusLabel);
    
    layout->addWidget(recordWidget);
    layout->addStretch();
    stackedWidget->addWidget(step3Widget);
}

void RegistrationWidget::setupStep4()
{
    QWidget *step4Widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(step4Widget);
    layout->setContentsMargins(150, 100, 150, 100);
    layout->setSpacing(50);
    
    // 成功标题
    QLabel *titleLabel = new QLabel();
    titleLabel->setText("注册成功！");
    titleLabel->setStyleSheet("color: #4CAF50; font-size: 32px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // 注册信息
    completionInfoLabel = new QLabel();
    completionInfoLabel->setStyleSheet(
        "color: #ffffff; font-size: 16px; background: #2d2d2d; "
        "padding: 20px; border-radius: 8px;"
    );
    completionInfoLabel->setAlignment(Qt::AlignCenter);
    completionInfoLabel->setWordWrap(true);
    layout->addWidget(completionInfoLabel);
    
    // 完成按钮
    returnButton = new QPushButton();
    returnButton->setText("完成注册");
    returnButton->setStyleSheet(
        "QPushButton {"
        "   color: #ffffff;"
        "   background: #4CAF50;"
        "   border: none;"
        "   border-radius: 8px;"
        "   padding: 12px 24px;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background: #45a049;"
        "}"
    );
    connect(returnButton, &QPushButton::clicked, this, &RegistrationWidget::finishRegistration);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(returnButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    layout->addStretch();
    stackedWidget->addWidget(step4Widget);
}

void RegistrationWidget::startRegistration()
{
    // 重置所有数据
    registrationData = UserRegistrationData();
    currentStep = 0;
    
    // 重置UI状态
    nameLineEdit->clear();
    nameErrorLabel->hide();
    
    // 重置录音状态
    if (recordStatusLabel) {
        recordStatusLabel->setText("准备录制");
        recordStatusLabel->setStyleSheet("color: #e0e0e0; font-size: 16px; font-weight: bold;");
    }
    if (recordButton) {
        recordButton->setEnabled(true);
        recordButton->setText("🎤 开始录制");
        recordButton->setStyleSheet(
            "QPushButton {"
            "   color: #ffffff;"
            "   background: #4CAF50;"
            "   border: 3px solid #4CAF50;"
            "   border-radius: 12px;"
            "   padding: 15px 30px;"
            "   font-size: 20px;"
            "   font-weight: bold;"
            "   min-width: 200px;"
            "   min-height: 60px;"
            "}"
        );
    }
    if (recordingProgressBar) {
        recordingProgressBar->setValue(0);
        recordingProgressBar->hide();
    }
    
    // 加载已有用户（用于关系设置）
    loadExistingUsers();
    
    // 显示第一步
    stackedWidget->setCurrentIndex(0);
    updateStepIndicator();
    updateButtonStates();
    
    // 聚焦到姓名输入框
    nameLineEdit->setFocus();
}

void RegistrationWidget::validateNameInput()
{
    QString name = nameLineEdit->text().trimmed();
    
    if (name.isEmpty()) {
        nameErrorLabel->setText("请输入姓名");
        nameErrorLabel->show();
        return;
    }
    
    if (name.length() < 2) {
        nameErrorLabel->setText("姓名至少需要2个字符");
        nameErrorLabel->show();
        return;
    }
    
    if (name.length() > 10) {
        nameErrorLabel->setText("姓名不能超过10个字符");
        nameErrorLabel->show();
        return;
    }
    
    nameErrorLabel->hide();
    updateButtonStates();
}

void RegistrationWidget::generateUserId()
{
    // 生成13位时间戳
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    registrationData.userId = GROUP_ID + QString::number(timestamp);
}

void RegistrationWidget::nextStep()
{
    // 验证当前步骤
    if (currentStep == 0) {
        // 验证姓名
        QString name = nameLineEdit->text().trimmed();
        if (name.isEmpty() || name.length() < 2 || name.length() > 10) {
            nameLineEdit->setFocus();
            return;
        }
        registrationData.name = name;
        generateUserId();
    }
    else if (currentStep == 1) {
        // 关系设置是可选的，直接通过
    }
    else if (currentStep == 2) {
        // 验证录音
        if (registrationData.audioFile.isEmpty() || !QFile::exists(registrationData.audioFile)) {
            QMessageBox::warning(this, "提示", "请完成语音录制");
            return;
        }
    }
    
    currentStep++;
    if (currentStep >= stackedWidget->count() - 1) {
        currentStep = stackedWidget->count() - 1;
        
        // 显示完成信息
        QString audioStatus = registrationData.audioFile.isEmpty() ? "未录制" : "已录制";
        QString infoText = QString(
            "用户信息：\n\n"
            "姓名：%1\n"
            "用户ID：%2\n"
            "语音样本：%3\n"
            "家庭关系：%4个\n"
            "注册时间：%5"
        ).arg(registrationData.name)
         .arg(registrationData.userId)
         .arg(audioStatus)
         .arg(registrationData.relations.size())
         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        
        completionInfoLabel->setText(infoText);
    }
    
    // 如果进入录音页面，更新录音提示文本
    if (currentStep == 2 && !registrationData.name.isEmpty() && recordingInstructionLabel) {
        QString personalizedText = QString("你好，我是%1。%2").arg(registrationData.name).arg(recordingText);
        recordingInstructionLabel->setText(personalizedText);
    }
    
    stackedWidget->setCurrentIndex(currentStep);
    updateStepIndicator();
    updateButtonStates();
}

void RegistrationWidget::previousStep()
{
    if (currentStep > 0) {
        currentStep--;
        stackedWidget->setCurrentIndex(currentStep);
        updateStepIndicator();
        updateButtonStates();
    }
}

void RegistrationWidget::updateStepIndicator()
{
    int progress = (currentStep + 1) * 25;
    progressBar->setValue(progress);
    
    QStringList stepTitles = {"基本信息", "关系设置", "语音录制", "注册完成"};
    if (currentStep < stepTitles.size()) {
        titleLabel->setText("用户注册 - " + stepTitles[currentStep]);
    }
}

void RegistrationWidget::updateButtonStates()
{
    // 第一步：隐藏上一步按钮
    if (currentStep == 0) {
        prevButton->hide();
    } else {
        prevButton->show();
        prevButton->setEnabled(currentStep > 0);
    }
    
    bool canNext = false;
    if (currentStep == 0) {
        QString name = nameLineEdit->text().trimmed();
        canNext = !name.isEmpty() && name.length() >= 2 && name.length() <= 10;
    }
    else if (currentStep == 1) {
        canNext = true; // 关系设置是可选的
    }
    else if (currentStep == 2) {
        canNext = !registrationData.audioFile.isEmpty() && QFile::exists(registrationData.audioFile);
    }
    
    // 最后一步：隐藏所有导航按钮
    if (currentStep == stackedWidget->count() - 1) {
        nextButton->hide();
        finishButton->hide();
        prevButton->hide();
    } else {
        nextButton->show();
        nextButton->setEnabled(canNext);
        finishButton->hide();
    }
}

void RegistrationWidget::startRecording()
{
    if (recordingInProgress) {
        stopRecording();
        return;
    }
    
    recordingInProgress = true;
    
    // 更新UI
    recordButton->setText("⏹️ 停止录制");
    recordButton->setStyleSheet(
        "QPushButton {"
        "   color: #ffffff;"
        "   background: #f44336;"
        "   border: 3px solid #f44336;"
        "   border-radius: 12px;"
        "   padding: 15px 30px;"
        "   font-size: 20px;"
        "   font-weight: bold;"
        "   min-width: 200px;"
        "   min-height: 60px;"
        "}"
        "QPushButton:hover {"
        "   background: #d32f2f;"
        "   border-color: #d32f2f;"
        "}"
    );
    recordStatusLabel->setText("🔴 录制中...");
    recordStatusLabel->setStyleSheet("color: #f44336; font-size: 16px; font-weight: bold;");
    
    // 显示进度条
    recordingProgressBar->show();
    recordingProgressBar->setValue(0);
    
    // 设置录音文件路径
    QString fileName = QString("user_%1_audio_%2.wav")
                      .arg(registrationData.userId.isEmpty() ? "temp" : registrationData.userId)
                      .arg(QDateTime::currentMSecsSinceEpoch());
    QString filePath = audioOutputDir + "/" + fileName;
    registrationData.audioFile = filePath;
    
    // 初始化录音器（如果需要的话）
    if (!audioRecorder) {
        audioRecorder = new QAudioRecorder(this);
        connect(audioRecorder, &QAudioRecorder::stateChanged, this, &RegistrationWidget::onRecordingFinished);
    }
    
    // 检查支持的编解码器
    qDebug() << "支持的音频编解码器:" << audioRecorder->supportedAudioCodecs();
    qDebug() << "支持的容器格式:" << audioRecorder->supportedContainers();
    
    // 设置录音参数（使用更兼容的设置）
    QAudioEncoderSettings audioSettings;
    
    // 尝试使用系统支持的编解码器
    QStringList supportedCodecs = audioRecorder->supportedAudioCodecs();
    if (supportedCodecs.contains("audio/x-wav")) {
        audioSettings.setCodec("audio/x-wav");
    } else if (supportedCodecs.contains("audio/pcm")) {
        audioSettings.setCodec("audio/pcm");
    } else if (!supportedCodecs.isEmpty()) {
        audioSettings.setCodec(supportedCodecs.first());
        qDebug() << "使用第一个可用编解码器:" << supportedCodecs.first();
    }
    
    audioSettings.setQuality(QMultimedia::NormalQuality); // 降低质量要求
    audioSettings.setSampleRate(16000);
    audioSettings.setChannelCount(1);
    
    // 设置容器格式
    QStringList supportedContainers = audioRecorder->supportedContainers();
    if (supportedContainers.contains("audio/x-wav")) {
        audioRecorder->setContainerFormat("audio/x-wav");
    } else if (supportedContainers.contains("wav")) {
        audioRecorder->setContainerFormat("wav");
    } else if (!supportedContainers.isEmpty()) {
        audioRecorder->setContainerFormat(supportedContainers.first());
        qDebug() << "使用第一个可用容器格式:" << supportedContainers.first();
    }
    
    audioRecorder->setEncodingSettings(audioSettings);
    
    qDebug() << "录音设置:";
    qDebug() << "  - 编解码器:" << audioSettings.codec();
    qDebug() << "  - 采样率:" << audioSettings.sampleRate();
    qDebug() << "  - 声道数:" << audioSettings.channelCount();
    qDebug() << "  - 容器格式:" << audioRecorder->containerFormat();
    
    // 开始录音
    audioRecorder->setOutputLocation(QUrl::fromLocalFile(filePath));
    qDebug() << "开始录音到:" << filePath;
    audioRecorder->record();
    
    // 检查录音状态
    QTimer::singleShot(500, [this]() {
        qDebug() << "录音状态检查:";
        qDebug() << "  - 当前状态:" << audioRecorder->state();
        qDebug() << "  - 错误信息:" << audioRecorder->errorString();
        qDebug() << "  - 实际输出位置:" << audioRecorder->actualLocation().toString();
    });
    
    // 启动进度更新定时器
    QTimer *progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, [this, progressTimer]() {
        if (!recordingInProgress) {
            progressTimer->deleteLater();
            return;
        }
        int currentValue = recordingProgressBar->value();
        if (currentValue < recordingProgressBar->maximum()) {
            recordingProgressBar->setValue(currentValue + 100); // 每100ms更新一次
        }
    });
    progressTimer->start(100);
    
    // 启动12秒定时器
    recordingTimer->start(RECORDING_DURATION_MS);
    
    qDebug() << "开始录制音频，文件路径:" << filePath;
}

void RegistrationWidget::stopRecording()
{
    if (!recordingInProgress) {
        return;
    }
    
    recordingInProgress = false;
    recordingTimer->stop();
    
    if (audioRecorder && audioRecorder->state() == QMediaRecorder::RecordingState) {
        audioRecorder->stop();
    }
    
    // 更新UI
    recordButton->setText("🎤 重新录制");
    recordButton->setStyleSheet(
        "QPushButton {"
        "   color: #ffffff;"
        "   background: #2196F3;"
        "   border: 3px solid #2196F3;"
        "   border-radius: 12px;"
        "   padding: 15px 30px;"
        "   font-size: 20px;"
        "   font-weight: bold;"
        "   min-width: 200px;"
        "   min-height: 60px;"
        "}"
        "QPushButton:hover {"
        "   background: #1976D2;"
        "   border-color: #1976D2;"
        "}"
    );
    
    // 验证文件是否存在
    QString filePath = registrationData.audioFile;
    qDebug() << "录制完成，检查文件:" << filePath;
    
    if (QFile::exists(filePath)) {
        QFileInfo fileInfo(filePath);
        qDebug() << "音频文件大小:" << fileInfo.size() << "字节";
        
        if (fileInfo.size() > 0) {
            recordStatusLabel->setText("✅ 录制完成");
            recordStatusLabel->setStyleSheet("color: #4CAF50; font-size: 16px; font-weight: bold;");
            recordingProgressBar->setValue(recordingProgressBar->maximum());
            recordingInstructionLabel->setText("录制成功！您可以重新录制或继续下一步。");
        } else {
            recordStatusLabel->setText("录音文件为空，请重新录制");
            recordStatusLabel->setStyleSheet("color: #f44336; font-size: 16px; font-weight: bold;");
            recordingProgressBar->setValue(0);
            registrationData.audioFile = "";
        }
    } else {
        qDebug() << "警告：录音文件不存在，可能录音失败";
        recordStatusLabel->setText("录音失败，请重新录制");
        recordStatusLabel->setStyleSheet("color: #f44336; font-size: 16px; font-weight: bold;");
        recordingProgressBar->setValue(0);
        registrationData.audioFile = "";
    }
    
    updateButtonStates();
}

void RegistrationWidget::onRecordingFinished()
{
    // 录音状态改变的处理
}

void RegistrationWidget::loadExistingUsers()
{
    // 清空现有用户列表
    existingUsers.clear();
    
    // 清空布局
    QLayoutItem *item;
    while ((item = usersLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // 显示加载状态
    showLoadingState(true);
    
    // 通过网络API获取已注册用户列表
    fetchRegisteredUsers();
    
    if (existingUsers.isEmpty()) {
        QLabel *noUsersLabel = new QLabel("暂无已注册用户\n您是第一个注册的用户，可以跳过关系设置");
        noUsersLabel->setStyleSheet("color: #cccccc; font-size: 16px;");
        noUsersLabel->setAlignment(Qt::AlignCenter);
        usersLayout->addWidget(noUsersLabel);
    } else {
        // 显示已有用户
        for (const UserData &user : existingUsers) {
            // 检查是否已经设置了关系
            QString existingRelation = "";
            for (const RelationData &relation : registrationData.relations) {
                if (relation.objectUserId == user.userId) {
                    existingRelation = relation.relationType;
                    break;
                }
            }
            
            QWidget *userWidget = new QWidget();
            QString borderColor = existingRelation.isEmpty() ? "#555" : "#4CAF50";
            userWidget->setStyleSheet(QString(
                "QWidget {"
                "   background: #2d2d2d;"
                "   border: 2px solid %1;"
                "   border-radius: 12px;"
                "   padding: 20px;"
                "   margin: 5px;"
                "}"
            ).arg(borderColor));
            userWidget->setMinimumHeight(180);
            
            QVBoxLayout *userMainLayout = new QVBoxLayout(userWidget);
            userMainLayout->setSpacing(15);
            
            // 用户信息行
            QHBoxLayout *userInfoLayout = new QHBoxLayout();
            
            // 用户头像和基本信息
            QVBoxLayout *infoLayout = new QVBoxLayout();
            infoLayout->setSpacing(8);
            QLabel *nameLabel = new QLabel(user.name);
            nameLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold;");
            QLabel *idLabel = new QLabel("ID: " + user.userId);
            idLabel->setStyleSheet("color: #e0e0e0; font-size: 14px;");
            
            infoLayout->addWidget(nameLabel);
            infoLayout->addWidget(idLabel);
            userInfoLayout->addLayout(infoLayout);
            
            userInfoLayout->addStretch();
            
            // 关系状态
            if (!existingRelation.isEmpty()) {
                QLabel *relationLabel = new QLabel("关系：" + existingRelation);
                relationLabel->setStyleSheet(
                    "color: #ffffff; font-size: 14px; font-weight: bold; "
                    "background: #4CAF50; "
                    "padding: 8px 12px; border-radius: 6px;"
                );
                userInfoLayout->addWidget(relationLabel);
            }
            
            userMainLayout->addLayout(userInfoLayout);
            
            // 操作区域
            QHBoxLayout *actionLayout = new QHBoxLayout();
            actionLayout->setSpacing(10);
            actionLayout->setContentsMargins(0, 5, 0, 5);
            
            if (existingRelation.isEmpty()) {
                // 未设置关系：显示选择器和添加按钮
                QComboBox *relationCombo = new QComboBox();
                relationCombo->addItem("选择关系", "");
                for (const QString &relation : relationOptions) {
                    relationCombo->addItem(relation, relation);
                }
                relationCombo->addItem("自定义关系", "custom");
                relationCombo->setStyleSheet(
                    "QComboBox {"
                    "   padding: 8px;"
                    "   border: 2px solid #666;"
                    "   border-radius: 6px;"
                    "   background: #1a1a1a;"
                    "   color: #ffffff;"
                    "   min-width: 120px;"
                    "   font-size: 14px;"
                    "}"
                    "QComboBox:focus {"
                    "   border-color: #4CAF50;"
                    "}"
                    "QComboBox::drop-down {"
                    "   border: none;"
                    "   width: 20px;"
                    "}"
                );
                actionLayout->addWidget(relationCombo);
                
                // 自定义关系输入框（初始隐藏）
                QLineEdit *customRelationEdit = new QLineEdit();
                customRelationEdit->setPlaceholderText("请输入自定义关系");
                customRelationEdit->setStyleSheet(
                    "QLineEdit {"
                    "   padding: 8px;"
                    "   border: 2px solid #666;"
                    "   border-radius: 6px;"
                    "   background: #1a1a1a;"
                    "   color: #ffffff;"
                    "   min-width: 120px;"
                    "   font-size: 14px;"
                    "}"
                    "QLineEdit:focus {"
                    "   border-color: #4CAF50;"
                    "}"
                );
                customRelationEdit->setVisible(false);
                customRelationEdit->setMaxLength(10);
                actionLayout->addWidget(customRelationEdit);
                
                // 当选择自定义关系时显示输入框
                connect(relationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [relationCombo, customRelationEdit](int index) {
                    QString selectedData = relationCombo->currentData().toString();
                    customRelationEdit->setVisible(selectedData == "custom");
                    if (selectedData != "custom") {
                        customRelationEdit->clear();
                    }
                });
                
                QPushButton *addButton = new QPushButton("添加关系");
                addButton->setStyleSheet(
                    "QPushButton {"
                    "   color: #ffffff;"
                    "   background: #4CAF50;"
                    "   border: 2px solid #4CAF50;"
                    "   border-radius: 6px;"
                    "   padding: 8px 16px;"
                    "   font-size: 14px;"
                    "   font-weight: bold;"
                    "   min-width: 80px;"
                    "}"
                    "QPushButton:hover {"
                    "   background: #45a049;"
                    "   border-color: #45a049;"
                    "}"
                );
                
                // 连接添加关系信号
                connect(addButton, &QPushButton::clicked, [this, user, relationCombo, customRelationEdit]() {
                    QString selectedRelation = relationCombo->currentData().toString();
                    QString finalRelation;
                    
                    if (selectedRelation == "custom") {
                        // 使用自定义关系
                        finalRelation = customRelationEdit->text().trimmed();
                        if (finalRelation.isEmpty()) {
                            QMessageBox::warning(this, "输入错误", "请输入自定义关系名称");
                            return;
                        }
                        // 验证自定义关系名称
                        if (finalRelation.length() > 10) {
                            QMessageBox::warning(this, "输入错误", "关系名称不能超过10个字符");
                            return;
                        }
                    } else if (!selectedRelation.isEmpty()) {
                        // 使用预定义关系
                        finalRelation = selectedRelation;
                    } else {
                        QMessageBox::warning(this, "选择错误", "请选择一个关系");
                        return;
                    }
                    
                    RelationData relation;
                    relation.subjectUserId = registrationData.userId;
                    relation.subjectName = registrationData.name;
                    relation.objectUserId = user.userId;
                    relation.objectName = user.name;
                    relation.relationType = finalRelation;
                    
                    registrationData.relations.append(relation);
                    updateRelationDisplay();
                    loadExistingUsers(); // 重新加载以更新显示
                });
                
                actionLayout->addWidget(addButton);
            } else {
                // 已设置关系：显示删除按钮
                actionLayout->addStretch();
                
                QPushButton *removeButton = new QPushButton("删除关系");
                removeButton->setStyleSheet(
                    "QPushButton {"
                    "   color: #ffffff;"
                    "   background: #f44336;"
                    "   border: 2px solid #f44336;"
                    "   border-radius: 6px;"
                    "   padding: 8px 16px;"
                    "   font-size: 14px;"
                    "   font-weight: bold;"
                    "   min-width: 80px;"
                    "}"
                    "QPushButton:hover {"
                    "   background: #d32f2f;"
                    "   border-color: #d32f2f;"
                    "}"
                );
                
                connect(removeButton, &QPushButton::clicked, [this, user]() {
                    QMessageBox::StandardButton reply = QMessageBox::question(this, 
                        "删除关系确认", 
                        QString("确定要删除与 %1 的关系吗？").arg(user.name),
                        QMessageBox::Yes | QMessageBox::No);
                    
                    if (reply == QMessageBox::Yes) {
                        removeRelation(user.userId);
                        loadExistingUsers(); // 重新加载以更新显示
                    }
                });
                
                actionLayout->addWidget(removeButton);
            }
            
            userMainLayout->addLayout(actionLayout);
            usersLayout->addWidget(userWidget);
        }
    }
    
    updateRelationDisplay();
}

void RegistrationWidget::updateRelationDisplay()
{
    if (registrationData.relations.isEmpty()) {
        relationStatusLabel->setText("暂无设置关系");
        relationStatusLabel->setStyleSheet(
            "QLabel {"
            "   color: #888888; font-size: 16px;"
            "   background: #2d2d2d;"
            "   padding: 15px;"
            "   border-radius: 8px;"
            "   border: 1px solid #555;"
            "}"
        );
    } else {
        QStringList relationTexts;
        for (const RelationData &relation : registrationData.relations) {
            relationTexts.append(QString("• %1 是我的 %2").arg(relation.objectName).arg(relation.relationType));
        }
        
        QString displayText = QString("✅ 已设置关系 (%1个):\n%2")
                             .arg(registrationData.relations.size())
                             .arg(relationTexts.join("\n"));
        
        relationStatusLabel->setText(displayText);
        relationStatusLabel->setStyleSheet(
            "QLabel {"
            "   color: #4CAF50; font-size: 16px; font-weight: bold;"
            "   background: rgba(76, 175, 80, 0.15);"
            "   padding: 15px;"
            "   border-radius: 8px;"
            "   border: 2px solid #4CAF50;"
            "}"
        );
    }
}

void RegistrationWidget::addRelation()
{
    // 这个方法由按钮回调处理
}

void RegistrationWidget::removeRelation(const QString& objectUserId)
{
    // 使用传统的迭代器方式移除元素（兼容性更好）
    for (int i = registrationData.relations.size() - 1; i >= 0; --i) {
        if (registrationData.relations[i].objectUserId == objectUserId) {
            registrationData.relations.removeAt(i);
        }
    }
    updateRelationDisplay();
}

void RegistrationWidget::skipRelations()
{
    nextStep();
}

void RegistrationWidget::finishRegistration()
{
    registrationData.registrationTime = QDateTime::currentDateTime();
    
    // 显示加载状态
    showLoadingState(true);
    
    // 通过网络API提交注册信息
    submitRegistration();
}

void RegistrationWidget::returnToInterface()
{
    emit backToInterface();
}

// ==================== 网络请求实现 ====================

void RegistrationWidget::fetchRegisteredUsers()
{
    qDebug() << "Qt网络模块不可用，使用curl命令替代...";
    
    // 使用curl命令获取用户列表
    QString url = serverBaseUrl + "/getRegisteredUsers?groupId=" + GROUP_ID;
    
    QProcess *curlProcess = new QProcess(this);
    QStringList arguments;
    arguments << "-s" << "-X" << "GET" << url;
    
    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, curlProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    QByteArray responseData = curlProcess->readAllStandardOutput();
                    qDebug() << "curl获取成功，响应数据:" << responseData;
                    parseFetchUsersResponse(responseData);
                } else {
                    QByteArray errorData = curlProcess->readAllStandardError();
                    qDebug() << "curl获取失败:" << errorData;
                    showNetworkError("网络请求失败：" + QString::fromUtf8(errorData));
                }
                curlProcess->deleteLater();
                showLoadingState(false);
            });
    
    showLoadingState(true);
    qDebug() << "使用curl请求:" << url;
    curlProcess->start("curl", arguments);
}

void RegistrationWidget::submitRegistration()
{
    qDebug() << "使用curl进行用户注册...";
    
    // 检查音频文件
    if (registrationData.audioFile.isEmpty()) {
        showNetworkError("没有录制音频文件");
        return;
    }
    
    QFile audioFile(registrationData.audioFile);
    if (!audioFile.exists()) {
        qDebug() << "音频文件不存在:" << registrationData.audioFile;
        showNetworkError("音频文件不存在");
        return;
    }
    
    if (audioFile.size() == 0) {
        qDebug() << "音频文件为空:" << registrationData.audioFile;
        showNetworkError("音频文件为空");
        return;
    }
    
    // 构建curl命令
    QString url = serverBaseUrl + "/registerUser";
    
    QProcess *curlProcess = new QProcess(this);
    QStringList arguments;
    arguments << "-s" << "-X" << "POST";
    arguments << "-F" << QString("userId=%1").arg(registrationData.userId);
    arguments << "-F" << QString("nickname=%1").arg(registrationData.name);
    arguments << "-F" << QString("groupId=%1").arg(GROUP_ID);
    arguments << "-F" << QString("createTime=%1").arg(registrationData.registrationTime.toString("yyyy-MM-dd hh:mm:ss"));
    
    // 添加关系数据
    if (!registrationData.relations.isEmpty()) {
        QJsonArray relationArray;
        for (const RelationData &relation : registrationData.relations) {
            QJsonObject relationObj;
            relationObj["userId"] = relation.objectUserId;
            relationObj["relation"] = relation.relationType;
            relationArray.append(relationObj);
        }
        QJsonDocument relationDoc(relationArray);
        arguments << "-F" << QString("relationships=%1").arg(QString::fromUtf8(relationDoc.toJson(QJsonDocument::Compact)));
    }
    
    // 添加音频文件
    arguments << "-F" << QString("audio=@%1").arg(registrationData.audioFile);
    arguments << url;
    
    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, curlProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    QByteArray responseData = curlProcess->readAllStandardOutput();
                    qDebug() << "curl注册成功，响应数据:" << responseData;
                    parseRegistrationResponse(responseData);
                } else {
                    QByteArray errorData = curlProcess->readAllStandardError();
                    qDebug() << "curl注册失败:" << errorData;
                    showNetworkError("注册请求失败：" + QString::fromUtf8(errorData));
                }
                curlProcess->deleteLater();
                showLoadingState(false);
            });
    
    showLoadingState(true);
    qDebug() << "使用curl注册用户:" << url;
    qDebug() << "音频文件:" << registrationData.audioFile;
    curlProcess->start("curl", arguments);
}

void RegistrationWidget::onGetUsersFinished()
{
    qDebug() << "=== 网络请求完成回调 ===";
    showLoadingState(false);
    
    if (!currentReply) {
        qDebug() << "错误：currentReply为空";
        return;
    }
    
    // 检查HTTP状态码
    int httpStatus = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP状态码:" << httpStatus;
    
    QNetworkReply::NetworkError error = currentReply->error();
    qDebug() << "网络错误代码:" << error;
    
    if (error != QNetworkReply::NoError) {
        qDebug() << "获取用户列表失败:" << currentReply->errorString();
        qDebug() << "详细错误信息:" << currentReply->readAll();
        showNetworkError("获取用户列表失败：" + currentReply->errorString());
        currentReply->deleteLater();
        currentReply = nullptr;
        return;
    }
    
    // 解析响应
    QByteArray responseData = currentReply->readAll();
    qDebug() << "响应数据长度:" << responseData.size();
    qDebug() << "响应内容:" << responseData;
    
    currentReply->deleteLater();
    currentReply = nullptr;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << parseError.errorString();
        showNetworkError("服务器响应格式错误");
        return;
    }
    
    QJsonObject response = doc.object();
    int code = response["code"].toInt();
    
    if (code != 200) {
        QString message = response["message"].toString();
        qDebug() << "服务器错误:" << code << message;
        showNetworkError("获取用户列表失败：" + message);
        return;
    }
    
    // 解析用户数据
    QJsonArray dataArray = response["data"].toArray();
    existingUsers.clear();
    
    for (const QJsonValue &value : dataArray) {
        QJsonObject userObj = value.toObject();
        UserData user;
        user.userId = userObj["userId"].toString();
        user.name = userObj["nickname"].toString();
        if (user.name.isEmpty()) {
            user.name = user.userId; // 如果昵称为空，显示用户ID
        }
        user.registrationTime = ""; // API响应中没有注册时间
        existingUsers.append(user);
    }
    
    qDebug() << "成功获取到" << existingUsers.size() << "个已注册用户";
    
    // 更新UI显示
    updateUserListUI();
}

void RegistrationWidget::onRegisterUserFinished()
{
    showLoadingState(false);
    
    if (!currentReply) {
        return;
    }
    
    QNetworkReply::NetworkError error = currentReply->error();
    if (error != QNetworkReply::NoError) {
        qDebug() << "用户注册失败:" << currentReply->errorString();
        showNetworkError("用户注册失败：" + currentReply->errorString());
        currentReply->deleteLater();
        currentReply = nullptr;
        return;
    }
    
    // 解析响应
    QByteArray responseData = currentReply->readAll();
    currentReply->deleteLater();
    currentReply = nullptr;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << parseError.errorString();
        showNetworkError("服务器响应格式错误");
        return;
    }
    
    QJsonObject response = doc.object();
    int code = response["code"].toInt();
    QString status = response["status"].toString();
    QString message = response["message"].toString();
    
    if (code == 200) {
        // 完全成功
        qDebug() << "用户注册完全成功:" << message;
        QMessageBox::information(this, "注册成功", "用户注册成功！\n声纹识别也已成功注册。");
        cleanupAudioFile(); // 清理音频文件
        emit registrationCompleted(registrationData);
    } else if (code == 207) {
        // 部分成功
        qDebug() << "用户注册部分成功:" << message;
        QJsonObject data = response["data"].toObject();
        QJsonObject audioResult = data["audioResult"].toObject();
        QString audioError = audioResult["error"].toString();
        
        QString detailMessage = "用户信息注册成功，但声纹注册失败。\n\n";
        detailMessage += "错误详情：" + audioError + "\n\n";
        detailMessage += "您可以稍后重新录制声纹。";
        
        QMessageBox::warning(this, "注册部分成功", detailMessage);
        cleanupAudioFile(); // 清理音频文件
        emit registrationCompleted(registrationData);
    } else if (code == 409) {
        // 用户ID冲突
        qDebug() << "用户ID冲突:" << message;
        showNetworkError("用户ID已存在，请重新生成用户ID");
        // 重新生成用户ID
        generateUserId();
    } else {
        // 其他错误
        qDebug() << "用户注册失败:" << code << message;
        showNetworkError("用户注册失败：" + message);
    }
}

void RegistrationWidget::onNetworkError(QNetworkReply::NetworkError error)
{
    qDebug() << "=== 网络错误回调 ===";
    qDebug() << "错误代码:" << error;
    
    showLoadingState(false);
    
    if (currentReply) {
        QString errorString = currentReply->errorString();
        int httpStatus = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray responseData = currentReply->readAll();
        
        qDebug() << "网络错误详情:";
        qDebug() << "  - 错误代码:" << error;
        qDebug() << "  - 错误描述:" << errorString;
        qDebug() << "  - HTTP状态:" << httpStatus;
        qDebug() << "  - 响应数据:" << responseData;
        qDebug() << "  - 请求URL:" << currentReply->url().toString();
        
        // 判断错误类型
        QString errorType = "未知错误";
        switch (error) {
            case QNetworkReply::ConnectionRefusedError:
                errorType = "连接被拒绝（服务器可能关闭）";
                break;
            case QNetworkReply::RemoteHostClosedError:
                errorType = "远程主机关闭连接";
                break;
            case QNetworkReply::HostNotFoundError:
                errorType = "主机未找到（DNS解析失败）";
                break;
            case QNetworkReply::TimeoutError:
                errorType = "请求超时（网络太慢）";
                break;
            case QNetworkReply::OperationCanceledError:
                errorType = "操作被取消";
                break;
            case QNetworkReply::SslHandshakeFailedError:
                errorType = "SSL握手失败";
                break;
            // case QNetworkReply::NetworkAccessibilityError:  // 在某些Qt版本中不存在
            //     errorType = "网络访问不可用";
            //     break;
            default:
                errorType = QString("其他错误（代码：%1）").arg(error);
        }
        qDebug() << "  - 错误类型:" << errorType;
        
        showNetworkError("网络连接错误：" + errorString);
        currentReply->deleteLater();
        currentReply = nullptr;
    } else {
        qDebug() << "currentReply为空";
    }
}

void RegistrationWidget::showNetworkError(const QString& message)
{
    QMessageBox::critical(this, "网络错误", message + "\n\n请检查网络连接并重试。");
}

void RegistrationWidget::showLoadingState(bool loading)
{
    // 禁用/启用相关按钮
    if (nextButton) nextButton->setEnabled(!loading);
    if (prevButton) prevButton->setEnabled(!loading);
    if (finishButton) finishButton->setEnabled(!loading);
    
    // 更新光标状态
    if (loading) {
        setCursor(Qt::WaitCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    
    // TODO: 可以添加加载动画或进度条
}

void RegistrationWidget::updateUserListUI()
{
    // 清空当前布局
    QLayoutItem *item;
    while ((item = usersLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    if (existingUsers.isEmpty()) {
        QLabel *noUsersLabel = new QLabel("暂无已注册用户\n您是第一个注册的用户，可以跳过关系设置");
        noUsersLabel->setStyleSheet("color: #cccccc; font-size: 16px;");
        noUsersLabel->setAlignment(Qt::AlignCenter);
        usersLayout->addWidget(noUsersLabel);
    } else {
        // 显示已有用户
        for (const UserData &user : existingUsers) {
            // 检查是否已经设置了关系
            QString existingRelation = "";
            for (const RelationData &relation : registrationData.relations) {
                if (relation.objectUserId == user.userId) {
                    existingRelation = relation.relationType;
                    break;
                }
            }
            
            QWidget *userWidget = new QWidget();
            userWidget->setMinimumHeight(120); // 增加最小高度，允许自适应
            userWidget->setStyleSheet(
                "QWidget {"
                "   background: #2d2d2d;"
                "   border: 1px solid #444;"
                "   border-radius: 8px;"
                "   margin: 4px;"
                "}"
            );
            
            QHBoxLayout *userLayout = new QHBoxLayout(userWidget);
            userLayout->setContentsMargins(15, 10, 15, 10);
            userLayout->setSpacing(15);
            
            // 用户信息区域
            QVBoxLayout *userInfoLayout = new QVBoxLayout();
            
            QLabel *nameLabel = new QLabel(user.name);
            nameLabel->setStyleSheet("color: #ffffff; font-size: 18px; font-weight: bold;");
            nameLabel->setMinimumHeight(25); // 确保姓名显示完整
            
            QLabel *idLabel = new QLabel("ID: " + user.userId);
            idLabel->setStyleSheet("color: #cccccc; font-size: 16px;");
            idLabel->setMinimumHeight(20); // 确保ID显示完整
            
            userInfoLayout->addWidget(nameLabel);
            userInfoLayout->addWidget(idLabel);
            userLayout->addLayout(userInfoLayout, 1);
            
            // 操作区域
            QHBoxLayout *actionLayout = new QHBoxLayout();
            actionLayout->setSpacing(10);
            
            if (existingRelation.isEmpty()) {
                // 未设置关系：显示下拉框和添加按钮
                QComboBox *relationCombo = new QComboBox();
                relationCombo->setStyleSheet(
                    "QComboBox {"
                    "   color: #ffffff;"
                    "   background: #3a3a3a;"
                    "   border: 1px solid #555;"
                    "   border-radius: 4px;"
                    "   padding: 5px 10px;"
                    "   font-size: 14px;"
                    "   min-width: 80px;"
                    "}"
                    "QComboBox::drop-down {"
                    "   border: none;"
                    "}"
                    "QComboBox::down-arrow {"
                    "   image: none;"
                    "   border-left: 5px solid transparent;"
                    "   border-right: 5px solid transparent;"
                    "   border-top: 5px solid #ffffff;"
                    "}"
                );
                
                relationCombo->addItem("选择关系", "");
                for (const QString &relation : relationOptions) {
                    relationCombo->addItem(relation, relation);
                }
                relationCombo->addItem("自定义关系", "custom");
                
                // 自定义关系输入框（初始隐藏）
                QLineEdit *customRelationEdit = new QLineEdit();
                customRelationEdit->setPlaceholderText("请输入自定义关系");
                customRelationEdit->setStyleSheet(
                    "QLineEdit {"
                    "   color: #ffffff;"
                    "   background: #3a3a3a;"
                    "   border: 1px solid #555;"
                    "   border-radius: 4px;"
                    "   padding: 5px 10px;"
                    "   font-size: 14px;"
                    "   min-width: 80px;"
                    "}"
                    "QLineEdit:focus {"
                    "   border-color: #4CAF50;"
                    "}"
                );
                customRelationEdit->setVisible(false);
                customRelationEdit->setMaxLength(10);
                
                // 当选择自定义关系时显示输入框
                connect(relationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [relationCombo, customRelationEdit](int index) {
                    QString selectedData = relationCombo->currentData().toString();
                    customRelationEdit->setVisible(selectedData == "custom");
                    if (selectedData != "custom") {
                        customRelationEdit->clear();
                    }
                });
                
                QPushButton *addButton = new QPushButton("添加");
                addButton->setStyleSheet(
                    "QPushButton {"
                    "   color: #ffffff;"
                    "   background: #4CAF50;"
                    "   border: none;"
                    "   border-radius: 4px;"
                    "   padding: 5px 15px;"
                    "   font-size: 14px;"
                    "   min-width: 60px;"
                    "}"
                    "QPushButton:hover {"
                    "   background: #45a049;"
                    "}"
                );
                
                // 连接添加关系信号
                connect(addButton, &QPushButton::clicked, [this, user, relationCombo, customRelationEdit]() {
                    QString selectedRelation = relationCombo->currentData().toString();
                    QString finalRelation;
                    
                    if (selectedRelation == "custom") {
                        // 使用自定义关系
                        finalRelation = customRelationEdit->text().trimmed();
                        if (finalRelation.isEmpty()) {
                            QMessageBox::warning(this, "输入错误", "请输入自定义关系名称");
                            return;
                        }
                        // 验证自定义关系名称
                        if (finalRelation.length() > 10) {
                            QMessageBox::warning(this, "输入错误", "关系名称不能超过10个字符");
                            return;
                        }
                    } else if (!selectedRelation.isEmpty()) {
                        // 使用预定义关系
                        finalRelation = selectedRelation;
                    } else {
                        QMessageBox::warning(this, "选择错误", "请选择一个关系");
                        return;
                    }
                    
                    RelationData relation;
                    relation.subjectUserId = registrationData.userId;
                    relation.subjectName = registrationData.name;
                    relation.objectUserId = user.userId;
                    relation.objectName = user.name;
                    relation.relationType = finalRelation;
                    
                    registrationData.relations.append(relation);
                    updateRelationDisplay();
                    updateUserListUI(); // 重新加载以更新显示
                });
                
                actionLayout->addWidget(relationCombo);
                actionLayout->addWidget(customRelationEdit);
                actionLayout->addWidget(addButton);
            } else {
                // 已设置关系：显示关系和删除按钮
                QLabel *relationLabel = new QLabel("关系: " + existingRelation);
                relationLabel->setStyleSheet("color: #4CAF50; font-size: 14px; font-weight: bold;");
                
                QPushButton *deleteButton = new QPushButton("删除");
                deleteButton->setStyleSheet(
                    "QPushButton {"
                    "   color: #ffffff;"
                    "   background: #f44336;"
                    "   border: none;"
                    "   border-radius: 4px;"
                    "   padding: 5px 15px;"
                    "   font-size: 14px;"
                    "   min-width: 60px;"
                    "}"
                    "QPushButton:hover {"
                    "   background: #da190b;"
                    "}"
                );
                
                // 连接删除关系信号
                connect(deleteButton, &QPushButton::clicked, [this, user]() {
                    removeRelation(user.userId);
                });
                
                actionLayout->addWidget(relationLabel);
                actionLayout->addWidget(deleteButton);
            }
            
            userLayout->addLayout(actionLayout);
            usersLayout->addWidget(userWidget);
        }
    }
    
    usersLayout->addStretch();
}

void RegistrationWidget::cleanupAudioFile()
{
    if (registrationData.audioFile.isEmpty()) {
        return;
    }
    
    QString filePath = registrationData.audioFile;
    registrationData.audioFile.clear(); // 先清空记录，避免重复尝试
    
    if (!QFile::exists(filePath)) {
        qDebug() << "音频文件不存在，无需删除:" << filePath;
        return;
    }
    
    // 确保音频录制器已经停止并释放文件
    if (audioRecorder && audioRecorder->state() != QMediaRecorder::StoppedState) {
        audioRecorder->stop();
        QTimer::singleShot(100, [this, filePath]() {
            deleteAudioFileWithRetry(filePath, 3);
        });
    } else {
        deleteAudioFileWithRetry(filePath, 3);
    }
}

void RegistrationWidget::deleteAudioFileWithRetry(const QString& filePath, int retryCount)
{
    QFile file(filePath);
    
    // 尝试设置文件为可写（有时文件可能是只读状态）
    file.setPermissions(QFile::WriteOwner | QFile::ReadOwner);
    
    if (file.remove()) {
        qDebug() << "成功删除音频文件:" << filePath;
        return;
    }
    
    if (retryCount > 0) {
        qDebug() << "删除音频文件失败，剩余重试次数:" << retryCount << "文件:" << filePath;
        // 等待100ms后重试
        QTimer::singleShot(100, [this, filePath, retryCount]() {
            deleteAudioFileWithRetry(filePath, retryCount - 1);
        });
    } else {
        qDebug() << "删除音频文件最终失败:" << filePath;
        qDebug() << "文件错误:" << file.errorString();
        
        // 如果删除失败，尝试标记文件为临时文件（系统重启时会清理）
        QString tempPath = filePath + ".tmp";
        if (file.rename(tempPath)) {
            qDebug() << "将文件重命名为临时文件:" << tempPath;
        }
    }
}

void RegistrationWidget::testBasicNetworkConnection()
{
    qDebug() << "=== 开始基础网络连接测试 ===";
    
    // 检查系统网络信息
    qDebug() << "系统网络配置检查:";
    qDebug() << "  - Qt版本:" << QT_VERSION_STR;
    qDebug() << "  - 网络管理器有效:" << (networkManager != nullptr);
    qDebug() << "  - 当前代理:" << networkManager->proxy().hostName() << ":" << networkManager->proxy().port();
    
    // 测试1：尝试连接百度（国内服务器）
    qDebug() << "测试1：连接百度服务器...";
    QNetworkRequest testRequest(QUrl("http://www.baidu.com"));
    testRequest.setRawHeader("User-Agent", "Qt Test");
    testRequest.setRawHeader("Accept", "*/*");
    
    QNetworkReply *testReply = networkManager->get(testRequest);
    
    if (!testReply) {
        qDebug() << "✗ 无法创建测试请求";
        return;
    }
    
    qDebug() << "测试请求已创建，等待响应...";
    
    // 设置较短的超时时间进行快速测试
    QTimer::singleShot(5000, [testReply]() {
        if (testReply && !testReply->isFinished()) {
            qDebug() << "百度连接测试超时（5秒）";
            testReply->abort();
        }
    });
    
    connect(testReply, &QNetworkReply::finished, [testReply]() {
        if (testReply->error() == QNetworkReply::NoError) {
            qDebug() << "✓ 基础网络连接正常（百度可访问）";
            qDebug() << "  - HTTP状态:" << testReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qDebug() << "  - 响应大小:" << testReply->readAll().size() << "字节";
        } else {
            qDebug() << "✗ 基础网络连接失败:" << testReply->errorString();
            qDebug() << "  - 错误代码:" << testReply->error();
        }
        testReply->deleteLater();
    });
    
    qDebug() << "基础网络连接测试已启动...";
}

void RegistrationWidget::parseFetchUsersResponse(const QByteArray& responseData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析失败:" << parseError.errorString();
        showNetworkError("服务器响应格式错误");
        return;
    }
    
    QJsonObject root = doc.object();
    int code = root["code"].toInt();
    
    if (code != 200) {
        QString message = root["message"].toString();
        qDebug() << "服务器返回错误:" << code << message;
        showNetworkError("服务器错误：" + message);
        return;
    }
    
    // 解析用户数据
    existingUsers.clear();
    QJsonArray dataArray = root["data"].toArray();
    
    for (const auto& value : dataArray) {
        QJsonObject userObj = value.toObject();
        UserData user;
        user.userId = userObj["userId"].toString();
        user.name = userObj["nickname"].toString();
        if (user.name.isEmpty()) {
            user.name = user.userId;
        }
        user.registrationTime = "";
        existingUsers.append(user);
    }
    
    qDebug() << "成功获取到" << existingUsers.size() << "个已注册用户";
    updateUserListUI();
}

void RegistrationWidget::parseRegistrationResponse(const QByteArray& responseData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "注册响应JSON解析失败:" << parseError.errorString();
        showNetworkError("服务器响应格式错误");
        return;
    }
    
    QJsonObject root = doc.object();
    int code = root["code"].toInt();
    QString message = root["message"].toString();
    
    qDebug() << "注册响应:" << code << message;
    
    if (code == 200) {
        qDebug() << "用户注册成功";
        cleanupAudioFile(); // 删除临时音频文件
        nextStep(); // 进入完成页面
    } else if (code == 207) {
        qDebug() << "部分注册成功:" << message;
        cleanupAudioFile();
        nextStep();
    } else if (code == 409) {
        qDebug() << "用户ID冲突:" << message;
        showNetworkError("用户ID已存在：" + message);
    } else {
        qDebug() << "用户注册失败:" << code << message;
        showNetworkError("用户注册失败：" + message);
    }
}
