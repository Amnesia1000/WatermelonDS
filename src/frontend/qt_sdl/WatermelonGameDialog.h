#pragma once

#include <QDialog>
#include <QString>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QJsonObject>
#include "WatermelonLauncher.h"

class QTabWidget;
class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QWidget;
class QFrame;
class QStackedWidget;
class QLineEdit;
class PickerRow;
class QTreeWidget;
class QTreeWidgetItem;
class WatermelonSwitch;

namespace melonDS {
    class ARCodeFile;
    struct ARCode;
}

// ─── Single achievement data ───────────────────────────────────────────────
struct RAGameAchievement
{
    int     id          = 0;
    QString title;
    QString description;
    int     points      = 0;
    QString badgeName;
    QString badgeUrl;
    bool    isUnlocked  = false;
    bool    isHardcore  = false;
};

// ─── Full RA game info ─────────────────────────────────────────────────────
struct RAGameInfo
{
    int     gameId          = 0;
    QString title;
    int     numAchievements = 0;
    int     numUnlocked     = 0;
    int     pointsTotal     = 0;
    int     pointsEarned    = 0;
    QList<RAGameAchievement> achievements;
};

// ─────────────────────────────────────────────────────────────────────────────
class WatermelonGameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WatermelonGameDialog(const WatermelonRom& rom, int initialTab = 0, QWidget* parent = nullptr);
    ~WatermelonGameDialog() override;

    static void applyGameConfig(const QString& gameCode);

private slots:
    void onSaveConfigClicked();
    void onChooseShaderClicked();
    void onClearShaderClicked();
    void onRALoginClicked();
    void onRALogoutClicked();
    void onViewOnWebClicked();
    void onSyncOfflineClicked();
    void onLoginReplyFinished(QNetworkReply* reply);
    void onGameInfoReplyFinished(QNetworkReply* reply);

private:
    void   setupUI(int initialTab);
    QWidget* createConfigTab();
    QWidget* createCheatsTab();
    QWidget* createRetroAchievementsTab();
    QWidget* createOfflineTab();

    // Cheats helpers
    void loadCheatsForGame();
    void populateCheatTree();
    void onAddCheatClicked();
    void onImportCheatDbClicked();
    void onCheatItemChanged(QTreeWidgetItem* item, int column);

    // RA helpers
    void loadRAGameData();
    void searchRAByGameList();
    void fetchGameProgress();
    void renderRAContent();
    void showRAError(const QString& msg);
    QFrame* buildAchievementCard(const QList<RAGameAchievement>& list, bool unlocked);
    void downloadBadge(const QString& badgeName, QLabel* lbl, bool unlocked);

    // Config load/save
    void loadPerGameConfig();
    void savePerGameConfig();

    // ─── Fields ───────────────────────────────────────────────────────────
    WatermelonRom romInfo;
    QTabWidget*   tabWidget = nullptr;

    // Config tab – picker rows
    PickerRow* pickerConsole   = nullptr;
    PickerRow* pickerRenderer  = nullptr;
    PickerRow* pickerJit       = nullptr;
    PickerRow* pickerBoot      = nullptr;
    PickerRow* pickerMic       = nullptr;
    QLabel*    shaderLabel     = nullptr;
    QString    currentShaderPath;

    // Cheats tab
    WatermelonSwitch* switchCheatsEnabled = nullptr;
    QTreeWidget*      treeCheats          = nullptr;
    QLabel*           cheatsStatusLabel   = nullptr;
    QWidget*          emptyCheatsWidget   = nullptr;
    std::unique_ptr<melonDS::ARCodeFile> gameCheatFile;
    QString           cheatFilePath;
    bool              loadingCheatItems   = false;

    // RA tab
    QNetworkAccessManager* networkMgr   = nullptr;
    QStackedWidget*  raStack            = nullptr;
    QLineEdit*       raUserInput        = nullptr;
    QLineEdit*       raPassInput        = nullptr;
    QLabel*          raLoginError       = nullptr;
    QLabel*          raErrorLabel       = nullptr;   // error page dynamic label
    QPushButton*     btnRALogin         = nullptr;
    QLabel*          raUserBadge        = nullptr;
    QLabel*          raUserName         = nullptr;
    QLabel*          raProgressLabel    = nullptr;
    QWidget*         raSummaryBar       = nullptr;
    QVBoxLayout*     raListLayout       = nullptr;
    bool             raLoggedIn         = false;
    RAGameInfo       raGame;

    // Offline tab
    QLabel*      offlineStatusLabel = nullptr;
    QPushButton* btnSyncOffline     = nullptr;
};
