#pragma once

#include <QDialog>
#include <QString>
#include <QPixmap>
#include <QNetworkAccessManager>

class QLineEdit;
class QLabel;
class QPushButton;
class QProgressBar;

class WatermelonRADialog : public QDialog
{
    Q_OBJECT

public:
    explicit WatermelonRADialog(QWidget* parent = nullptr);
    ~WatermelonRADialog() override;

    static bool isEnabled();
    static bool isHardcore();
    static QString getUsername();
    static QString getToken();

signals:
    void loginStateChanged(bool loggedIn);

private slots:
    void onLoginClicked();
    void onLogoutClicked();
    void onOpenWebProfileClicked();
    void onOpenSettingsClicked();
    void onLoginReplyFinished();
    void onUserPicDownloaded();

private:
    void setupUI();
    void loadUserData();
    void updateProfileView();
    void testAuthentication(const QString& username, const QString& password);
    void verifyToken(const QString& username, const QString& token);

    QNetworkAccessManager* networkManager = nullptr;

    // Login Widgets
    QWidget* loginSection = nullptr;
    QLineEdit* userInput = nullptr;
    QLineEdit* passInput = nullptr;
    QPushButton* btnLogin = nullptr;
    QLabel* loginErrorLabel = nullptr;
    QProgressBar* loginProgress = nullptr;

    // User Profile / Summary Widgets
    QWidget* profileSection = nullptr;
    QLabel* avatarLabel = nullptr;
    QLabel* profileNameLabel = nullptr;
    QLabel* profileStatusBadge = nullptr;
    QLabel* lblHardcorePoints = nullptr;
    QLabel* lblSoftcorePoints = nullptr;
    QLabel* lblGameMode = nullptr;
    QPushButton* btnViewWebProfile = nullptr;
    QPushButton* btnOpenSettings = nullptr;
    QPushButton* btnLogout = nullptr;

    bool isLoggedIn = false;
    int userPoints = 0;
    int userSoftcorePoints = 0;
    int userRank = 0;
};
