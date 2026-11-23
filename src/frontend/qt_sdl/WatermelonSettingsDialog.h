#ifndef WATERMELONSETTINGSDIALOG_H
#define WATERMELONSETTINGSDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QStackedWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

class InputConfigDialog;

class WatermelonSwitch : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(int offset READ offset WRITE setOffset)

public:
    explicit WatermelonSwitch(QWidget* parent = nullptr);
    QSize sizeHint() const override;

    int offset() const { return m_offset; }
    void setOffset(int o) { m_offset = o; update(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void nextCheckState() override;

private:
    int m_offset = 0;
};

class WatermelonSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WatermelonSettingsDialog(QWidget* parent = nullptr, int initialCategory = 0);
    ~WatermelonSettingsDialog() override;

signals:
    void settingsSaved();

private slots:
    void onCategoryChanged(int row);
    void onAddRomDirClicked();
    void onRemoveRomDirClicked();
    void onResetDefaultsClicked();
    void onApplyClicked();
    void onOkClicked();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    // Pages
    QWidget* createGeneralPage();
    QWidget* createSystemPage();
    QWidget* createCpuPage();
    QWidget* createGraphicsPage();
    QWidget* createAudioPage();
    QWidget* createControlsPage();
    QWidget* createRAPage();

    // Sidebar & Navigation
    QListWidget* sidebarList = nullptr;
    QStackedWidget* pageStack = nullptr;

    // General Page widgets
    WatermelonSwitch* swPauseLostFocus = nullptr;
    WatermelonSwitch* swMouseHide = nullptr;
    WatermelonSwitch* swDirectBoot = nullptr;
    QListWidget* listRomDirs = nullptr;
    QComboBox* cbxUILanguage = nullptr;

    // System Page widgets
    QComboBox* cbxConsoleType = nullptr;
    QComboBox* cbxLanguage = nullptr;
    QLineEdit* txtUsername = nullptr;
    QComboBox* cbxFavColour = nullptr;
    QSpinBox* spnBirthDay = nullptr;
    QComboBox* cbxBirthMonth = nullptr;
    QLineEdit* txtMessage = nullptr;
    WatermelonSwitch* swExternalBios = nullptr;
    QWidget* biosFilesContainer = nullptr;
    QLineEdit* txtBios9 = nullptr;
    QLineEdit* txtBios7 = nullptr;
    QLineEdit* txtFirmware = nullptr;
    QLineEdit* txtDSiBios9 = nullptr;
    QLineEdit* txtDSiBios7 = nullptr;
    QLineEdit* txtDSiFirmware = nullptr;
    QLineEdit* txtDSiNAND = nullptr;

    // CPU Page widgets
    WatermelonSwitch* swJitEnable = nullptr;
    QSpinBox* spnJitBlockSize = nullptr;
    WatermelonSwitch* swJitBranchOpt = nullptr;
    WatermelonSwitch* swJitLiteralOpt = nullptr;
    WatermelonSwitch* swJitFastMem = nullptr;

    // Graphics Page widgets
    QComboBox* cbxRenderer3D = nullptr;
    QComboBox* cbxScaleFactor = nullptr;
    QComboBox* cbxShader = nullptr;
    WatermelonSwitch* swVSync = nullptr;
    WatermelonSwitch* swBetterPolygons = nullptr;
    WatermelonSwitch* swHiresCoords = nullptr;
    WatermelonSwitch* swSoftThreaded = nullptr;

    // Audio Page widgets
    QSlider* sldVolume = nullptr;
    QLabel* lblVolumeVal = nullptr;
    QComboBox* cbxInterpolation = nullptr;
    QComboBox* cbxBitDepth = nullptr;

    // Controls Page widgets
    InputConfigDialog* inputConfigWidget = nullptr;

    // RetroAchievements Page widgets
    WatermelonSwitch* swRAEnabled = nullptr;
    WatermelonSwitch* swRAHardcore = nullptr;
    WatermelonSwitch* swRARichPresence = nullptr;
    WatermelonSwitch* swRASound = nullptr;
    WatermelonSwitch* swRAUnofficial = nullptr;
    WatermelonSwitch* swRAEncore = nullptr;
    QLineEdit* txtRAUser = nullptr;
    QLineEdit* txtRAToken = nullptr;
    QLabel* lblRAStatus = nullptr;
};

#endif // WATERMELONSETTINGSDIALOG_H
