#pragma once

#include <QDialog>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>

class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QLabel;
class QPushButton;
class QProgressBar;

class WatermelonShaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WatermelonShaderDialog(QWidget* parent = nullptr);
    ~WatermelonShaderDialog() override;

    static QString getShaderDirectory();
    static QString getActivePresetPath();
    static void setActivePresetPath(const QString& path);

signals:
    void shaderPresetChanged(const QString& presetPath);

private slots:
    void onDownloadClicked();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onFilterChanged(const QString& filter);
    void onCategorySelectionChanged();
    void onPresetItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onApplyClicked();
    void onDisableClicked();

private:
    void setupUI();
    void loadShaderCategories();
    void updateActivePresetLabel();
    bool checkShadersInstalled() const;

    QNetworkAccessManager* networkManager = nullptr;
    QLabel* statusLabel = nullptr;
    QWidget* downloadBanner = nullptr;
    QPushButton* btnDownload = nullptr;
    QProgressBar* progressBar = nullptr;

    QLineEdit* filterInput = nullptr;
    QTreeWidget* treeWidget = nullptr;
    QLabel* activePresetLabel = nullptr;
    QPushButton* btnApply = nullptr;
    QPushButton* btnDisable = nullptr;

    QString shadersPath;
    QString currentSelectedPreset;
};
