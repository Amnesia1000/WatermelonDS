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
#include <QDateTimeEdit>
#include <QDateTime>
#include <vector>
#include "Net_PCap.h"

class InputConfigDialog;
class QRadioButton;
class QDoubleSpinBox;

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
    void updateNetAdapterControls();
    void onNetAdapterIndexChanged(int index);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    // Pages
    QWidget* createGeneralPage();
    QWidget* createSystemPage();
    QWidget* createCpuPage();
    QWidget* createGraphicsPage();
    QWidget* createDisplayPage();
    QWidget* createAudioPage();
    QWidget* createControlsPage();
    QWidget* createMultiplayerPage();
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
    QLineEdit* txtSaveFilePath = nullptr;
    QLineEdit* txtSavestatePath = nullptr;
    QLineEdit* txtCheatFilePath = nullptr;
    QSpinBox* spnMouseHideSeconds = nullptr;
    QDoubleSpinBox* spnTargetFPS = nullptr;
    QDoubleSpinBox* spnFastForwardFPS = nullptr;
    QDoubleSpinBox* spnSlowmoFPS = nullptr;
    WatermelonSwitch* swSavestateRelocSRAM = nullptr;

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

    // System Page - DSi & DLDI widgets
    WatermelonSwitch* swDSiFullBIOSBoot = nullptr;
    WatermelonSwitch* swDSPHLE = nullptr;
    WatermelonSwitch* swDSiSDEnable = nullptr;
    QWidget* dsiSDContainer = nullptr;
    QLineEdit* txtDSiSDPath = nullptr;
    QComboBox* cbxDSiSDSize = nullptr;
    WatermelonSwitch* swDSiSDReadOnly = nullptr;
    WatermelonSwitch* swDSiSDFolder = nullptr;
    QWidget* dsiSDFolderContainer = nullptr;
    QLineEdit* txtDSiSDFolder = nullptr;

    WatermelonSwitch* swDLDIEnable = nullptr;
    QWidget* dldiContainer = nullptr;
    QLineEdit* txtDLDISDPath = nullptr;
    QComboBox* cbxDLDISize = nullptr;
    WatermelonSwitch* swDLDIReadOnly = nullptr;
    WatermelonSwitch* swDLDIFolder = nullptr;
    QWidget* dldiFolderContainer = nullptr;
    QLineEdit* txtDLDIFolder = nullptr;

    // DSi Cameras
    QComboBox* cbxCam0Type = nullptr;
    QLineEdit* txtCam0ImagePath = nullptr;
    QComboBox* cbxCam0Device = nullptr;
    WatermelonSwitch* swCam0XFlip = nullptr;
    QWidget* cam0ImageContainer = nullptr;
    QWidget* cam0DeviceContainer = nullptr;

    QComboBox* cbxCam1Type = nullptr;
    QLineEdit* txtCam1ImagePath = nullptr;
    QComboBox* cbxCam1Device = nullptr;
    WatermelonSwitch* swCam1XFlip = nullptr;
    QWidget* cam1ImageContainer = nullptr;
    QWidget* cam1DeviceContainer = nullptr;

    // System Page - RTC & Power widgets
    WatermelonSwitch* swCustomRTC = nullptr;
    QWidget* rtcContainer = nullptr;
    QDateTimeEdit* dteCustomRTC = nullptr;
    QPushButton* btnRTCNow = nullptr;
    QComboBox* cbxDSBattery = nullptr;
    QComboBox* cbxDSiBattery = nullptr;
    WatermelonSwitch* swDSiCharging = nullptr;

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

    // Display Page widgets
    QComboBox* cbxWindowSize = nullptr;
    QComboBox* cbxScreenLayout = nullptr;
    QComboBox* cbxScreenSizing = nullptr;
    QComboBox* cbxScreenRotation = nullptr;
    QSpinBox* spnScreenGap = nullptr;
    QComboBox* cbxScreenAspectTop = nullptr;
    QComboBox* cbxScreenAspectBot = nullptr;
    WatermelonSwitch* swScreenSwap = nullptr;
    WatermelonSwitch* swIntegerScaling = nullptr;
    WatermelonSwitch* swScreenFilter = nullptr;

    // Audio Page widgets
    QSlider* sldVolume = nullptr;
    QLabel* lblVolumeVal = nullptr;
    QComboBox* cbxInterpolation = nullptr;
    QComboBox* cbxBitDepth = nullptr;
    WatermelonSwitch* swDSiVolumeSync = nullptr;
    QComboBox* cbxMicInputType = nullptr;
    QComboBox* cbxMicDevice = nullptr;
    QLineEdit* txtMicWavPath = nullptr;
    QWidget* micDeviceContainer = nullptr;
    QWidget* micWavContainer = nullptr;

    // Controls Page widgets
    InputConfigDialog* inputConfigWidget = nullptr;

    // Multiplayer & Network Page widgets
    QComboBox* cbxMPAudioMode = nullptr;
    QSpinBox* spnMPRecvTimeout = nullptr;
    QRadioButton* rbDirectMode = nullptr;
    QRadioButton* rbIndirectMode = nullptr;
    QComboBox* cbxNetAdapter = nullptr;
    QLabel* lblAdapterMAC = nullptr;
    QLabel* lblAdapterIP = nullptr;
    QWidget* adapterContainer = nullptr;
    std::vector<melonDS::AdapterData> netAdapters;
    bool hasNetPcap = false;

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
