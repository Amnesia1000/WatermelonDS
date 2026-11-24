#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QList>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QSet>
#include <QTimer>

class QLineEdit;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QLabel;
class QPushButton;
class QSlider;

struct WatermelonRom
{
    QString filePath;
    QString fileName;
    QString title;
    QString gameCode;
    QString raHash;
    QPixmap cover;
    QPixmap icon;
    bool isRecent = false;
    bool hasCover = false;
    bool isDSi = false;
    bool hasAchievements = false;
};

class WatermelonLauncher : public QWidget
{
    Q_OBJECT

public:
    explicit WatermelonLauncher(QWidget* parent = nullptr);
    ~WatermelonLauncher() override;

    void refresh();

signals:
    void romSelected(const QString& filePath);
    void openFileRequested();
    void bootFirmwareRequested();
    void mpNewInstanceRequested();
    void lanStartHostRequested();
    void lanStartClientRequested();
    void mpSettingsRequested();
    void settingsCategoryRequested(int category);

private slots:
    void onSearchTextChanged(const QString& text);
    void onOpenClicked();
    void onBootFirmwareClicked();
    void onMultiplayerClicked();
    void onAddDirectoryClicked();
    void onRefreshClicked();
    void onShadersClicked();
    void onRetroAchievementsClicked();
    void onSettingsClicked();
    void onCoverDownloaded(const QString& key, const QPixmap& pixmap);
    void onCoverSizeChanged(int val);

private:
    void setupUI();
    void scanDirectories();
    void loadRecent();
    void renderGames();
    QWidget* createGameCard(const WatermelonRom& rom, bool isRecentShelf = false);
    void showContextMenu(const WatermelonRom& rom, const QPoint& globalPos);
    void openGameDialog(const WatermelonRom& rom, int initialTab = 0);
    QPixmap extractRomInfo(const QString& path, QString& outTitle, QString& outCode, QPixmap& outIcon, bool& outHasCover, bool& outIsDSi);
    QPixmap generateDefaultCover(const QString& title, const QString& gameCode, const QPixmap& iconPix);
    void fetchCoverArt(const QString& title, const QString& gameCode, const QString& rawBaseName);
    void tryDownloadCandidate(const QString& key, const QStringList& urls, int index);
    void updateHeaderResponsiveness(int width);
    void fetchRAGamesList();
    void applyRAData();
    void saveRACache();
    QString computeRAHash(const QString& romPath);
    void checkRAHashForRom(const QString& hash);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* titleLogo = nullptr;
    QLineEdit* searchInput = nullptr;
    QPushButton* btnOpen = nullptr;
    QPushButton* btnBootFirmware = nullptr;
    QPushButton* btnMultiplayer = nullptr;
    QPushButton* btnShaders = nullptr;
    QPushButton* btnRA = nullptr;
    QPushButton* btnSettings = nullptr;
    QPushButton* btnAddDir = nullptr;
    QPushButton* btnRefresh = nullptr;

    QScrollArea* scrollArea = nullptr;
    QWidget* recentShelfContainer = nullptr;
    QHBoxLayout* recentShelfLayout = nullptr;
    QWidget* recentShelfSection = nullptr;

    QWidget* gridContainer = nullptr;
    QLabel* emptyLabel = nullptr;
    QSlider* sldCoverSize = nullptr;
    int m_coverWidth = 169;

    QList<WatermelonRom> allRoms;
    QList<WatermelonRom> recentRoms;
    QString currentFilter;

    QNetworkAccessManager* networkManager = nullptr;
    QSet<QString> pendingDownloads;
    QTimer* renderDebounceTimer = nullptr;
    int lastColumns = -1;
    QMap<QString, bool> raHashCache;  // raHash -> hasAchievements
    bool raChecksRunning = false;
};
