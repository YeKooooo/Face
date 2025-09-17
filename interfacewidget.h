#ifndef INTERFACEWIDGET_H
#define INTERFACEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>

class InterfaceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit InterfaceWidget(QWidget *parent = nullptr);

signals:
    void backClicked();

private:
    QPushButton *registerBtn;
    QPushButton *homeMapBtn;
    QPushButton *deviceManageBtn;
    QPushButton *consultBtn;
    QPushButton *backBtn;
};

#endif // INTERFACEWIDGET_H 