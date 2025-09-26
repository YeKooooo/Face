#ifndef REGISTRATIONWIDGET_H
#define REGISTRATIONWIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProgressBar>
#include <QTextEdit>
#include <QTimer>
#include <QDateTime>
#include <QScrollArea>
#include <QCheckBox>
#include <QAudioRecorder>
#include <QAudioEncoderSettings>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QMediaRecorder>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkProxy>
#include <QProcess>

struct UserData {
    QString name;
    QString userId;
    QString registrationTime;
};

struct RelationData {
    QString subjectUserId;    // 当前注册用户ID
    QString subjectName;      // 当前注册用户姓名
    QString objectUserId;     // 目标用户ID  
    QString objectName;       // 目标用户姓名
    QString relationType;     // "爸爸" (纯中文)
};

struct UserRegistrationData {
    QString name;                    // 姓名
    QString userId;                  // X16BAC + 13位时间戳
    QString audioFile;               // 单段录音文件路径（10-15秒）
    QList<RelationData> relations;   // 家庭关系列表
    QDateTime registrationTime;      // 注册时间
};

class RegistrationWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit RegistrationWidget(QWidget *parent = nullptr);
    ~RegistrationWidget();
    
public slots:
    void startRegistration();
    void returnToInterface();
    
signals:
    void backToInterface();
    void registrationCompleted(const UserRegistrationData& userData);
    
private slots:
    void nextStep();
    void previousStep();
    void startRecording();
    void stopRecording();
    void onRecordingFinished();
    void finishRegistration();
    void validateNameInput();
    void addRelation();
    void removeRelation(const QString& objectUserId);
    void skipRelations();
    
    // 网络请求相关槽函数
    void onGetUsersFinished();
    void onRegisterUserFinished();
    void onNetworkError(QNetworkReply::NetworkError error);
    
private:
    void setupUI();
    void setupStep1();     // 姓名输入
    void setupStep2();     // 语音录制  
    void setupStep3();     // 关系设置
    void setupStep4();     // 注册完成
    void updateStepIndicator();
    void updateButtonStates();
    void generateUserId();
    void loadExistingUsers();
    void updateRelationDisplay();
    
    // 网络请求方法
    void fetchRegisteredUsers();
    void submitRegistration();
    void showNetworkError(const QString& message);
    void showLoadingState(bool loading);
    void updateUserListUI();
    void cleanupAudioFile();
    void deleteAudioFileWithRetry(const QString& filePath, int retryCount);
    void testBasicNetworkConnection();
    void parseFetchUsersResponse(const QByteArray& responseData);
    void parseRegistrationResponse(const QByteArray& responseData);
    
    // UI组件
    QStackedWidget *stackedWidget;
    QLabel *titleLabel;
    QProgressBar *progressBar;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *finishButton;
    
    // Step 1: 姓名输入
    QLineEdit *nameLineEdit;
    QLabel *nameErrorLabel;
    
    // Step 2: 语音录制
    QLabel *recordingInstructionLabel;
    QPushButton *recordButton;
    QLabel *recordStatusLabel;
    QProgressBar *recordingProgressBar;
    QTimer *recordingTimer;
    QAudioRecorder *audioRecorder;
    bool recordingInProgress;
    QString recordingText;
    
    // Step 3: 关系设置
    QScrollArea *usersScrollArea;
    QWidget *usersWidget;
    QVBoxLayout *usersLayout;
    QList<UserData> existingUsers;
    QComboBox *relationComboBox;
    QPushButton *addRelationButton;
    QPushButton *skipRelationsButton;
    QLabel *relationStatusLabel;
    
    // Step 4: 注册完成
    QLabel *completionInfoLabel;
    QPushButton *returnButton;
    
    // 数据
    UserRegistrationData registrationData;
    QStringList relationOptions;
    int currentStep;
    QString audioOutputDir;
    
    // 网络相关成员
    QNetworkAccessManager *networkManager;
    QNetworkReply *currentReply;
    QString serverBaseUrl;
    
    // 常量
    static const QString GROUP_ID;
    static const int RECORDING_DURATION_MS = 12000; // 12秒
};

#endif // REGISTRATIONWIDGET_H
