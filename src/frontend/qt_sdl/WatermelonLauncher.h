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
class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QLabel;
class QPushButton;

struct WatermelonRom
{
    QString filePath;
    QString fileName;
    QString title;
    QString gameCode;
    QPixmap cover;
    QPixmap icon;
    bool isRecent = false;
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

private slots:
    void onSearchTextChanged(const QString& text);
    void onOpenClicked();
    void onAddDirectoryClicked();
    void onRefreshClicked();
    void onShadersClicked();
    void onRetroAchievementsClicked();
    void onSettingsClicked();
    void onCoverDownloaded(const QString& key, const QPixmap& pixmap);

private:
    void setupUI();
    void scanDirectories();
    void loadRecent();
    void renderGames();
    QWidget* createGameCard(const WatermelonRom& rom, bool isRecentShelf = false);
    void showContextMenu(const WatermelonRom& rom, const QPoint& globalPos);
    void openGameDialog(const WatermelonRom& rom, int initialTab = 0);
    QPixmap extractRomInfo(const QString& path, QString& outTitle, QString& outCode, QPixmap& outIcon);
    QPixmap generateDefaultCover(const QString& title, const QString& gameCode, const QPixmap& iconPix);
    void fetchCoverArt(const QString& title, const QString& gameCode, const QString& rawBaseName);
    void tryDownloadCandidate(const QString& key, const QStringList& urls, int index);
    void updateHeaderResponsiveness(int width);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* titleLogo = nullptr;
    QLineEdit* searchInput = nullptr;
    QPushButton* btnOpen = nullptr;
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
    QGridLayout* gridLayout = nullptr;
    QLabel* emptyLabel = nullptr;

    QList<WatermelonRom> allRoms;
    QList<WatermelonRom> recentRoms;
    QString currentFilter;

    QNetworkAccessManager* networkManager = nullptr;
    QSet<QString> pendingDownloads;
    QTimer* renderDebounceTimer = nullptr;
};
