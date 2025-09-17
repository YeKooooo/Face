#include "interfacewidget.h"

InterfaceWidget::InterfaceWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet("background-color:#1e1e1e;");

    QFont font;
    font.setPointSize(20);

    registerBtn = new QPushButton(QStringLiteral("用户注册"), this);
    homeMapBtn = new QPushButton(QStringLiteral("家庭地图"), this);
    deviceManageBtn = new QPushButton(QStringLiteral("设备管理"), this);
    consultBtn = new QPushButton(QStringLiteral("用药咨询"), this);
    backBtn = new QPushButton(QStringLiteral("返回"), this);

    QList<QPushButton*> btns{registerBtn, homeMapBtn, deviceManageBtn, consultBtn, backBtn};
    for (auto *btn : btns) {
        btn->setMinimumHeight(60);
        btn->setFont(font);
        btn->setStyleSheet("QPushButton { color:#ffffff; background:#3a3a3a; border:none; border-radius:8px; } QPushButton:hover { background:#505050; }");
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->setContentsMargins(100, 100, 100, 100);
    layout->addWidget(registerBtn);
    layout->addWidget(homeMapBtn);
    layout->addWidget(deviceManageBtn);
    layout->addWidget(consultBtn);
    layout->addStretch(1);
    layout->addWidget(backBtn);

    connect(backBtn, &QPushButton::clicked, this, &InterfaceWidget::backClicked);
} 