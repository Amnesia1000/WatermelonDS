#include "WatermelonSettingsDialog.h"
#include "WatermelonRADialog.h"
#include "InputConfig/InputConfigDialog.h"
#include "Window.h"
#include "Config.h"
#include "Net.h"
#include "Net_PCap.h"
#include "Wifi.h"
#include "MPInterface.h"
#include "CameraManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QCoreApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFrame>
#include <QDialogButtonBox>
#include <QApplication>
#include <QRadioButton>
#include <QButtonGroup>
#include <SDL2/SDL.h>
#if QT_VERSION >= 0x060000
#include <QMediaDevices>
#include <QCameraDevice>
#endif

#ifdef __WIN32__
#define PCAP_NAME "winpcap/npcap"
#else
#define PCAP_NAME "libpcap"
#endif

extern std::optional<melonDS::LibPCap> pcap;
extern melonDS::Net net;
extern CameraManager* camManager[2];
void NetInit();

// ═══════════════════════════════════════════════════════════════════════════
//  Helper: Find MainWindow
// ═══════════════════════════════════════════════════════════════════════════

static MainWindow* findMainWindow(QWidget* w)
{
    while (w) {
        if (auto* mw = qobject_cast<MainWindow*>(w)) return mw;
        w = w->parentWidget();
    }
    for (QWidget* top : QApplication::topLevelWidgets()) {
        if (auto* mw = qobject_cast<MainWindow*>(top)) return mw;
    }
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
//  WatermelonSwitch Implementation (Toggle Switch like modern mobile/apps)
// ═══════════════════════════════════════════════════════════════════════════

WatermelonSwitch::WatermelonSwitch(QWidget* parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setChecked(false);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize WatermelonSwitch::sizeHint() const
{
    return QSize(46, 24);
}

void WatermelonSwitch::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int radius = h / 2;

    // Track color (Vibrant Pink/Watermelon when ON, dark subtle track when OFF)
    QColor trackColor = isChecked() ? QColor(0xe5, 0x4d, 0x5e) : QColor(0x35, 0x33, 0x40);
    if (!isEnabled())
        trackColor = QColor(0x28, 0x27, 0x30);

    QPainterPath trackPath;
    trackPath.addRoundedRect(0, 0, w, h, radius, radius);
    p.fillPath(trackPath, trackColor);

    // Knob color & position
    const int knobMargin = 3;
    const int knobDiameter = h - (knobMargin * 2);
    const int knobX = isChecked() ? (w - knobDiameter - knobMargin) : knobMargin;
    const int knobY = knobMargin;

    // Knob subtle shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 40));
    p.drawEllipse(knobX, knobY + 1, knobDiameter, knobDiameter);

    // Knob circle
    p.setBrush(isEnabled() ? QColor(0xf6, 0xf5, 0xf3) : QColor(0x75, 0x73, 0x80));
    p.drawEllipse(knobX, knobY, knobDiameter, knobDiameter);
}

void WatermelonSwitch::nextCheckState()
{
    setChecked(!isChecked());
    emit toggled(isChecked());
    update();
}

// ═══════════════════════════════════════════════════════════════════════════
//  WatermelonSettingsDialog Implementation
// ═══════════════════════════════════════════════════════════════════════════

WatermelonSettingsDialog::WatermelonSettingsDialog(QWidget* parent, int initialCategory)
    : QDialog(parent)
{
    setWindowTitle(tr("Configuración de WatermelonDS"));
    resize(940, 680);
    setMinimumSize(860, 600);
    setModal(true);

    if (!pcap)
        pcap = melonDS::LibPCap::New();

    hasNetPcap = pcap.has_value();
    if (pcap)
        netAdapters = pcap->GetAdapters();

    setupUI();
    loadSettings();

    if (initialCategory >= 0 && initialCategory < sidebarList->count()) {
        sidebarList->setCurrentRow(initialCategory);
    }
}

WatermelonSettingsDialog::~WatermelonSettingsDialog() = default;

void WatermelonSettingsDialog::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Main Area: Split into Left Sidebar and Right Pages
    auto* bodyWidget = new QWidget(this);
    auto* bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // --- Sidebar (Left) ---
    sidebarList = new QListWidget(bodyWidget);
    sidebarList->setFixedWidth(200);
    sidebarList->setFrameShape(QFrame::NoFrame);
    sidebarList->setFocusPolicy(Qt::NoFocus);
    sidebarList->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background-color: #151419;"
        "  border-right: 1px solid #23222b;"
        "  padding: 12px 6px;"
        "  outline: 0;"
        "}"
        "QListWidget::item {"
        "  height: 40px;"
        "  border-radius: 8px;"
        "  margin: 3px 6px;"
        "  padding-left: 12px;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "  color: #a09ea8;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #1e1d25;"
        "  color: #f6f5f3;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #2b1921;"
        "  color: #ff5477;"
        "  border-left: 3px solid #ff5477;"
        "}"
    ));

    auto* itemGeneral     = new QListWidgetItem(tr("🛠  General"), sidebarList);
    auto* itemSystem      = new QListWidgetItem(tr("🪪  Sistema"), sidebarList);
    auto* itemCpu         = new QListWidgetItem(tr("⚙  CPU"), sidebarList);
    auto* itemGraphics    = new QListWidgetItem(tr("🖥  Gráficos"), sidebarList);
    auto* itemDisplay     = new QListWidgetItem(tr("🖼  Pantalla"), sidebarList);
    auto* itemAudio       = new QListWidgetItem(tr("🔊  Audio"), sidebarList);
    auto* itemControls    = new QListWidgetItem(tr("🎮  Controles"), sidebarList);
    auto* itemMultiplayer = new QListWidgetItem(tr("🌐  Multijugador y Red"), sidebarList);
    auto* itemRA          = new QListWidgetItem(tr("🏆  Logros"), sidebarList);

    Q_UNUSED(itemGeneral);
    Q_UNUSED(itemSystem);
    Q_UNUSED(itemCpu);
    Q_UNUSED(itemGraphics);
    Q_UNUSED(itemDisplay);
    Q_UNUSED(itemAudio);
    Q_UNUSED(itemControls);
    Q_UNUSED(itemMultiplayer);
    Q_UNUSED(itemRA);

    connect(sidebarList, &QListWidget::currentRowChanged, this, &WatermelonSettingsDialog::onCategoryChanged);
    bodyLayout->addWidget(sidebarList);

    // --- Page Stack (Right) ---
    pageStack = new QStackedWidget(bodyWidget);
    pageStack->setStyleSheet(QStringLiteral("background-color: #141317;"));

    pageStack->addWidget(createGeneralPage());
    pageStack->addWidget(createSystemPage());
    pageStack->addWidget(createCpuPage());
    pageStack->addWidget(createGraphicsPage());
    pageStack->addWidget(createDisplayPage());
    pageStack->addWidget(createAudioPage());
    pageStack->addWidget(createControlsPage());
    pageStack->addWidget(createMultiplayerPage());
    pageStack->addWidget(createRAPage());

    bodyLayout->addWidget(pageStack, 1);
    rootLayout->addWidget(bodyWidget, 1);

    // --- Bottom Bar ---
    auto* bottomBar = new QWidget(this);
    bottomBar->setFixedHeight(58);
    bottomBar->setStyleSheet(QStringLiteral(
        "background-color: #131216;"
        "border-top: 1px solid #23222b;"
        "padding: 0 16px;"
    ));
    auto* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(16, 0, 16, 0);
    bottomLayout->setSpacing(12);

    auto* btnReset = new QPushButton(tr("Reiniciar todos los ajustes"), bottomBar);
    btnReset->setCursor(Qt::PointingHandCursor);
    btnReset->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #1f1e26; border: 1px solid #2c2b36; border-radius: 6px;"
        "  color: #a09ea8; font-size: 12px; padding: 6px 14px;"
        "}"
        "QPushButton:hover { background: #292833; color: #f6f5f3; border-color: #3b3a47; }"
    ));
    connect(btnReset, &QPushButton::clicked, this, &WatermelonSettingsDialog::onResetDefaultsClicked);
    bottomLayout->addWidget(btnReset);

    auto* lblNote = new QLabel(tr("Algunos ajustes solo están disponibles cuando no se estén ejecutando juegos."), bottomBar);
    lblNote->setStyleSheet(QStringLiteral("color: #63616e; font-size: 11px; margin-left: 6px; border: none; background: transparent;"));
    bottomLayout->addWidget(lblNote);

    bottomLayout->addStretch();

    auto* btnOk = new QPushButton(tr("OK"), bottomBar);
    btnOk->setCursor(Qt::PointingHandCursor);
    btnOk->setFixedSize(88, 32);
    btnOk->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #e54d5e; border: none; border-radius: 6px;"
        "  color: #ffffff; font-size: 13px; font-weight: 700;"
        "}"
        "QPushButton:hover { background: #ff5c6f; }"
        "QPushButton:pressed { background: #cc3e4e; }"
    ));
    connect(btnOk, &QPushButton::clicked, this, &WatermelonSettingsDialog::onOkClicked);
    bottomLayout->addWidget(btnOk);

    auto* btnApply = new QPushButton(tr("Apply"), bottomBar);
    btnApply->setCursor(Qt::PointingHandCursor);
    btnApply->setFixedSize(88, 32);
    btnApply->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #24222c; border: 1px solid #32303c; border-radius: 6px;"
        "  color: #f6f5f3; font-size: 13px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #2f2d3a; border-color: #403e4d; }"
    ));
    connect(btnApply, &QPushButton::clicked, this, &WatermelonSettingsDialog::onApplyClicked);
    bottomLayout->addWidget(btnApply);

    auto* btnCancel = new QPushButton(tr("Cancel"), bottomBar);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setFixedSize(88, 32);
    btnCancel->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #24222c; border: 1px solid #32303c; border-radius: 6px;"
        "  color: #f6f5f3; font-size: 13px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #2f2d3a; border-color: #403e4d; }"
    ));
    connect(btnCancel, &QPushButton::clicked, this, [this]() {
        if (inputConfigWidget) inputConfigWidget->reject();
        reject();
    });
    bottomLayout->addWidget(btnCancel);

    rootLayout->addWidget(bottomBar);
}

void WatermelonSettingsDialog::onCategoryChanged(int row)
{
    pageStack->setCurrentIndex(row);
}

// ───────────────────────────────────────────────────────────────────────────
//  Helper UI builders
// ───────────────────────────────────────────────────────────────────────────

static void setupInput(QWidget* w, int width = -1)
{
    w->setFixedHeight(26);
    if (width > 0) {
        w->setFixedWidth(width);
    }
}

static QFrame* createCard(QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("settingsCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#settingsCard {"
        "  background-color: #1a1921;"
        "  border: 1px solid #272632;"
        "  border-radius: 8px;"
        "}"
        "QLabel { background: transparent; border: none; font-size: 13px; color: #f6f5f3; }"
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {"
        "  background-color: #141318; border: 1px solid #2d2c38; border-radius: 6px;"
        "  color: #f6f5f3; padding: 0px 8px; height: 26px; min-height: 26px; max-height: 26px;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {"
        "  border-color: #ff5477; background-color: #181720;"
        "}"
        "QComboBox { padding-right: 22px; }"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding; subcontrol-position: top right; width: 22px; border: none; background: transparent;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/arrow-down); width: 10px; height: 10px;"
        "}"
        "QSpinBox, QDoubleSpinBox { padding-right: 20px; }"
        "QSpinBox::up-button, QDoubleSpinBox::up-button {"
        "  subcontrol-origin: border; subcontrol-position: top right; width: 18px;"
        "  border-left: 1px solid #2d2c38; border-bottom: 1px solid #2d2c38; border-top-right-radius: 5px;"
        "  background: #1c1b22;"
        "}"
        "QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover { background: #282732; }"
        "QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {"
        "  image: url(:/arrow-up); width: 8px; height: 8px;"
        "}"
        "QSpinBox::down-button, QDoubleSpinBox::down-button {"
        "  subcontrol-origin: border; subcontrol-position: bottom right; width: 18px;"
        "  border-left: 1px solid #2d2c38; border-bottom-right-radius: 5px;"
        "  background: #1c1b22;"
        "}"
        "QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover { background: #282732; }"
        "QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {"
        "  image: url(:/arrow-down-small); width: 8px; height: 8px;"
        "}"
    ));
    return card;
}

static QLabel* createSectionTitle(const QString& title, QWidget* parent)
{
    auto* lbl = new QLabel(title, parent);
    lbl->setStyleSheet(QStringLiteral(
        "font-size: 13px; font-weight: 700; color: #ff5477; margin-top: 6px; margin-bottom: 2px;"
        "background: transparent; border: none;"
    ));
    return lbl;
}

static QHBoxLayout* createSwitchRow(WatermelonSwitch* sw, const QString& title, const QString& desc = QString(), QWidget* parent = nullptr)
{
    auto* row = new QHBoxLayout();
    row->setContentsMargins(6, 4, 6, 4);
    row->setSpacing(14);

    row->addWidget(sw);

    auto* textLay = new QVBoxLayout();
    textLay->setSpacing(1);
    auto* titleLbl = new QLabel(title, parent);
    titleLbl->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 600; color: #f6f5f3; background: transparent; border: none;"));
    textLay->addWidget(titleLbl);

    if (!desc.isEmpty()) {
        auto* descLbl = new QLabel(desc, parent);
        descLbl->setStyleSheet(QStringLiteral("font-size: 11px; color: #8e8c99; background: transparent; border: none;"));
        descLbl->setWordWrap(true);
        textLay->addWidget(descLbl);
    }

    row->addLayout(textLay, 1);
    return row;
}

static QHBoxLayout* createPathRow(const QString& labelText, QLineEdit*& txtOut, const QString& filter, QWidget* parent)
{
    auto* row = new QHBoxLayout();
    row->setSpacing(8);

    auto* lbl = new QLabel(labelText, parent);
    lbl->setFixedWidth(120);
    row->addWidget(lbl);

    txtOut = new QLineEdit(parent);
    setupInput(txtOut);
    row->addWidget(txtOut, 1);

    auto* btnBrowse = new QPushButton(QCoreApplication::translate("WatermelonSettingsDialog", "Examinar..."), parent);
    btnBrowse->setCursor(Qt::PointingHandCursor);
    setupInput(btnBrowse, 90);
    btnBrowse->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #252430; border: 1px solid #363444; border-radius: 6px;"
        "  color: #f6f5f3; padding: 2px 10px; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #31303e; border-color: #ff5477; color: #ff5477; }"
    ));
    QLineEdit* targetEdit = txtOut;
    QObject::connect(btnBrowse, &QPushButton::clicked, parent, [parent, targetEdit, filter, labelText]() {
        QString f = QFileDialog::getOpenFileName(parent, labelText, targetEdit->text(), filter);
        if (!f.isEmpty()) targetEdit->setText(f);
    });
    row->addWidget(btnBrowse);

    return row;
}

static QHBoxLayout* createFilePickerRow(const QString& labelText, QLineEdit*& txtOut, const QString& filter, QWidget* parent)
{
    auto* row = new QHBoxLayout();
    row->setSpacing(8);

    auto* lbl = new QLabel(labelText, parent);
    lbl->setFixedWidth(120);
    row->addWidget(lbl);

    txtOut = new QLineEdit(parent);
    setupInput(txtOut);
    row->addWidget(txtOut, 1);

    auto* btnBrowse = new QPushButton(QCoreApplication::translate("WatermelonSettingsDialog", "Examinar..."), parent);
    btnBrowse->setCursor(Qt::PointingHandCursor);
    setupInput(btnBrowse, 90);
    btnBrowse->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #252430; border: 1px solid #363444; border-radius: 6px;"
        "  color: #f6f5f3; padding: 2px 10px; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #31303e; border-color: #ff5477; color: #ff5477; }"
    ));
    QLineEdit* targetEdit = txtOut;
    QObject::connect(btnBrowse, &QPushButton::clicked, parent, [parent, targetEdit, labelText, filter]() {
        QString f = QFileDialog::getOpenFileName(parent, labelText, targetEdit->text(), filter);
        if (!f.isEmpty()) targetEdit->setText(f);
    });
    row->addWidget(btnBrowse);

    return row;
}

static QHBoxLayout* createFolderRow(const QString& labelText, QLineEdit*& txtOut, QWidget* parent)
{
    auto* row = new QHBoxLayout();
    row->setSpacing(8);

    auto* lbl = new QLabel(labelText, parent);
    lbl->setFixedWidth(120);
    row->addWidget(lbl);

    txtOut = new QLineEdit(parent);
    setupInput(txtOut);
    row->addWidget(txtOut, 1);

    auto* btnBrowse = new QPushButton(QCoreApplication::translate("WatermelonSettingsDialog", "Examinar..."), parent);
    btnBrowse->setCursor(Qt::PointingHandCursor);
    setupInput(btnBrowse, 90);
    btnBrowse->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #252430; border: 1px solid #363444; border-radius: 6px;"
        "  color: #f6f5f3; padding: 2px 10px; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #31303e; border-color: #ff5477; color: #ff5477; }"
    ));
    QLineEdit* targetEdit = txtOut;
    QObject::connect(btnBrowse, &QPushButton::clicked, parent, [parent, targetEdit, labelText]() {
        QString d = QFileDialog::getExistingDirectory(parent, labelText, targetEdit->text());
        if (!d.isEmpty()) targetEdit->setText(d);
    });
    row->addWidget(btnBrowse);

    return row;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 1: General
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createGeneralPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(18);

    lay->addWidget(createSectionTitle(tr("Comportamiento del Emulador"), content));

    auto* cardGen = createCard(content);
    auto* cardGenLay = new QVBoxLayout(cardGen);
    cardGenLay->setContentsMargins(14, 12, 14, 12);
    cardGenLay->setSpacing(12);

    swPauseLostFocus = new WatermelonSwitch(cardGen);
    cardGenLay->addLayout(createSwitchRow(swPauseLostFocus, tr("Pausar cuando la ventana no esté activa"), tr("Detiene la emulación temporalmente si cambias a otra aplicación."), cardGen));

    swMouseHide = new WatermelonSwitch(cardGen);
    cardGenLay->addLayout(createSwitchRow(swMouseHide, tr("Ocultar el cursor por inactividad"), tr("Oculta automáticamente la flecha del ratón mientras juegas."), cardGen));

    auto* mouseDelayRow = new QHBoxLayout();
    mouseDelayRow->setContentsMargins(24, 0, 0, 0);
    mouseDelayRow->setSpacing(10);
    auto* lblMouseDelay = new QLabel(tr("Segundos de inactividad:"), cardGen);
    lblMouseDelay->setFixedWidth(160);
    mouseDelayRow->addWidget(lblMouseDelay);
    spnMouseHideSeconds = new QSpinBox(cardGen);
    spnMouseHideSeconds->setRange(1, 30);
    spnMouseHideSeconds->setValue(5);
    spnMouseHideSeconds->setSuffix(QStringLiteral(" s"));
    setupInput(spnMouseHideSeconds, 80);
    mouseDelayRow->addWidget(spnMouseHideSeconds);
    mouseDelayRow->addStretch();
    cardGenLay->addLayout(mouseDelayRow);

    swDirectBoot = new WatermelonSwitch(cardGen);
    cardGenLay->addLayout(createSwitchRow(swDirectBoot, tr("Ejecución directa (Direct Boot)"), tr("Inicia los juegos inmediatamente saltando el menú de inicio de la consola."), cardGen));

    lay->addWidget(cardGen);

    // Contenido externo / Carpetas de ROMs
    lay->addWidget(createSectionTitle(tr("Contenido externo (Carpetas de ROMs)"), content));

    auto* cardDirs = createCard(content);
    auto* cardDirsLay = new QVBoxLayout(cardDirs);
    cardDirsLay->setContentsMargins(14, 14, 14, 14);
    cardDirsLay->setSpacing(10);

    auto* descDirs = new QLabel(tr("Añade directorios para buscar automáticamente juegos de Nintendo DS y carátulas:"), cardDirs);
    descDirs->setStyleSheet(QStringLiteral("color: #a09ea8; font-size: 12px; background: transparent; border: none;"));
    cardDirsLay->addWidget(descDirs);

    listRomDirs = new QListWidget(cardDirs);
    listRomDirs->setFixedHeight(120);
    listRomDirs->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background: #131217; border: 1px solid #282733; border-radius: 6px;"
        "  color: #f6f5f3; padding: 6px;"
        "}"
        "QListWidget::item { padding: 6px; border-radius: 4px; }"
        "QListWidget::item:selected { background: #2c1a22; color: #ff5477; }"
    ));
    cardDirsLay->addWidget(listRomDirs);

    auto* dirBtnsLay = new QHBoxLayout();
    dirBtnsLay->setSpacing(10);

    auto* btnAddDir = new QPushButton(tr("Añadir directorio"), cardDirs);
    btnAddDir->setCursor(Qt::PointingHandCursor);
    btnAddDir->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #252430; border: 1px solid #363444; border-radius: 6px;"
        "  color: #f6f5f3; padding: 6px 14px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #31303e; border-color: #48465a; }"
    ));
    connect(btnAddDir, &QPushButton::clicked, this, &WatermelonSettingsDialog::onAddRomDirClicked);
    dirBtnsLay->addWidget(btnAddDir);

    auto* btnRemDir = new QPushButton(tr("Eliminar seleccionado"), cardDirs);
    btnRemDir->setCursor(Qt::PointingHandCursor);
    btnRemDir->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #252430; border: 1px solid #363444; border-radius: 6px;"
        "  color: #f6f5f3; padding: 6px 14px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #31303e; border-color: #48465a; }"
    ));
    connect(btnRemDir, &QPushButton::clicked, this, &WatermelonSettingsDialog::onRemoveRomDirClicked);
    dirBtnsLay->addWidget(btnRemDir);
    dirBtnsLay->addStretch();

    cardDirsLay->addLayout(dirBtnsLay);
    lay->addWidget(cardDirs);

    // Rutas de Guardado
    lay->addWidget(createSectionTitle(tr("Rutas de Guardado"), content));
    auto* cardPaths = createCard(content);
    auto* pathsLay = new QVBoxLayout(cardPaths);
    pathsLay->setContentsMargins(14, 12, 14, 12);
    pathsLay->setSpacing(10);

    pathsLay->addLayout(createFolderRow(tr("Partidas guardadas:"), txtSaveFilePath, cardPaths));
    pathsLay->addLayout(createFolderRow(tr("Estados de guardado:"), txtSavestatePath, cardPaths));
    pathsLay->addLayout(createFolderRow(tr("Archivos de trucos:"), txtCheatFilePath, cardPaths));
    pathsLay->addLayout(createFilePickerRow(tr("DB de trucos (.dat):"), txtCheatDbPath, tr("Bases de datos de trucos (*.dat);;Todos los archivos (*)"), cardPaths));
    txtCheatDbPath->setPlaceholderText(tr("Ruta global a usrcheat.dat (importación automática)"));

    swSavestateRelocSRAM = new WatermelonSwitch(cardPaths);
    pathsLay->addLayout(createSwitchRow(swSavestateRelocSRAM,
        tr("Partidas independientes por cada Savestate"),
        tr("Guarda una copia independiente del archivo de guardado (.sav) para cada ranura de savestate."),
        cardPaths));

    auto* lblPathsHint = new QLabel(tr("Si se dejan en blanco, los archivos se guardarán junto a la ROM o en el directorio por defecto."), cardPaths);
    lblPathsHint->setStyleSheet(QStringLiteral("color: #8e8c99; font-size: 11px; background: transparent; border: none;"));
    lblPathsHint->setWordWrap(true);
    pathsLay->addWidget(lblPathsHint);

    lay->addWidget(cardPaths);

    // Velocidad de Emulación
    lay->addWidget(createSectionTitle(tr("Velocidad de Emulación"), content));
    auto* cardSpeed = createCard(content);
    auto* speedGrid = new QGridLayout(cardSpeed);
    speedGrid->setContentsMargins(16, 16, 16, 16);
    speedGrid->setHorizontalSpacing(16);
    speedGrid->setVerticalSpacing(14);

    speedGrid->addWidget(new QLabel(tr("Velocidad Normal (FPS objetivo):"), cardSpeed), 0, 0);
    spnTargetFPS = new QDoubleSpinBox(cardSpeed);
    spnTargetFPS->setRange(1.0, 300.0);
    spnTargetFPS->setDecimals(4);
    spnTargetFPS->setValue(60.0);
    spnTargetFPS->setSuffix(QStringLiteral(" FPS"));
    setupInput(spnTargetFPS, 120);
    speedGrid->addWidget(spnTargetFPS, 0, 1);

    speedGrid->addWidget(new QLabel(tr("Avance Rápido (Fast Forward):"), cardSpeed), 1, 0);
    spnFastForwardFPS = new QDoubleSpinBox(cardSpeed);
    spnFastForwardFPS->setRange(1.0, 1000.0);
    spnFastForwardFPS->setDecimals(2);
    spnFastForwardFPS->setValue(120.0);
    spnFastForwardFPS->setSuffix(QStringLiteral(" FPS"));
    setupInput(spnFastForwardFPS, 120);
    speedGrid->addWidget(spnFastForwardFPS, 1, 1);

    speedGrid->addWidget(new QLabel(tr("Cámara Lenta (Slow Motion):"), cardSpeed), 2, 0);
    spnSlowmoFPS = new QDoubleSpinBox(cardSpeed);
    spnSlowmoFPS->setRange(1.0, 120.0);
    spnSlowmoFPS->setDecimals(2);
    spnSlowmoFPS->setValue(30.0);
    spnSlowmoFPS->setSuffix(QStringLiteral(" FPS"));
    setupInput(spnSlowmoFPS, 120);
    speedGrid->addWidget(spnSlowmoFPS, 2, 1);

    lay->addWidget(cardSpeed);

    // Idioma de la interfaz
    lay->addWidget(createSectionTitle(tr("Idioma de la Interfaz"), content));

    auto* cardLang = createCard(content);
    auto* langLay = new QVBoxLayout(cardLang);
    langLay->setContentsMargins(14, 12, 14, 12);
    langLay->setSpacing(8);

    auto* langRow = new QHBoxLayout();
    langRow->setSpacing(10);
    langRow->addWidget(new QLabel(tr("Idioma de la interfaz:"), cardLang));
    cbxUILanguage = new QComboBox(cardLang);
    cbxUILanguage->addItem(tr("Automático (según el sistema)"), QString());
    cbxUILanguage->addItem(QStringLiteral("Español"), QStringLiteral("es"));
    cbxUILanguage->addItem(QStringLiteral("English"), QStringLiteral("en"));
    setupInput(cbxUILanguage, 240);
    langRow->addWidget(cbxUILanguage);
    langRow->addStretch();
    langLay->addLayout(langRow);

    auto* langDesc = new QLabel(tr("Selecciona el idioma de los menús y diálogos. Los cambios se aplican al reiniciar el emulador."), cardLang);
    langDesc->setStyleSheet(QStringLiteral("color: #8e8c99; font-size: 11px; background: transparent; border: none;"));
    langDesc->setWordWrap(true);
    langLay->addWidget(langDesc);
    lay->addWidget(cardLang);

    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 2: Sistema
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createSystemPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(18);

    lay->addWidget(createSectionTitle(tr("Consola y Firmware"), content));

    auto* cardSys = createCard(content);
    auto* grid = new QGridLayout(cardSys);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(tr("Tipo de Consola:"), cardSys), 0, 0);
    cbxConsoleType = new QComboBox(cardSys);
    cbxConsoleType->addItems({tr("Nintendo DS (Original)"), tr("Nintendo DSi")});
    setupInput(cbxConsoleType);
    grid->addWidget(cbxConsoleType, 0, 1);

    grid->addWidget(new QLabel(tr("Idioma del Sistema:"), cardSys), 1, 0);
    cbxLanguage = new QComboBox(cardSys);
    cbxLanguage->addItems({QStringLiteral("Japonés"), QStringLiteral("Inglés"), QStringLiteral("Francés"), QStringLiteral("Alemán"), QStringLiteral("Italiano"), QStringLiteral("Español")});
    setupInput(cbxLanguage);
    grid->addWidget(cbxLanguage, 1, 1);

    grid->addWidget(new QLabel(tr("Nombre de Usuario:"), cardSys), 2, 0);
    txtUsername = new QLineEdit(cardSys);
    setupInput(txtUsername);
    grid->addWidget(txtUsername, 2, 1);

    // Color Favorito with Color Swatch Icons
    grid->addWidget(new QLabel(tr("Color Favorito:"), cardSys), 3, 0);
    cbxFavColour = new QComboBox(cardSys);
    setupInput(cbxFavColour);

    static const struct {
        const char* name;
        QColor color;
    } s_dsColors[16] = {
        {"Gris",           QColor(97, 130, 154)},
        {"Marrón",         QColor(186, 73, 0)},
        {"Rojo",           QColor(251, 0, 24)},
        {"Rosa Claro",     QColor(251, 138, 251)},
        {"Naranja",        QColor(251, 146, 0)},
        {"Amarillo",       QColor(243, 227, 0)},
        {"Verde Lima",     QColor(170, 251, 0)},
        {"Verde",          QColor(0, 251, 0)},
        {"Verde Oscuro",   QColor(0, 162, 56)},
        {"Turquesa",       QColor(0, 251, 178)},
        {"Azul Claro",     QColor(0, 235, 251)},
        {"Azul",           QColor(0, 138, 251)},
        {"Azul Oscuro",    QColor(0, 0, 251)},
        {"Púrpura Oscuro", QColor(138, 0, 219)},
        {"Violeta",        QColor(219, 0, 219)},
        {"Magenta",        QColor(251, 0, 138)}
    };

    for (int i = 0; i < 16; ++i) {
        QPixmap pix(18, 18);
        pix.fill(Qt::transparent);
        {
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(s_dsColors[i].color);
            p.setPen(QPen(QColor(255, 255, 255, 60), 1));
            p.drawRoundedRect(1, 1, 16, 16, 4, 4);
        }
        cbxFavColour->addItem(QIcon(pix), QString::fromUtf8(s_dsColors[i].name));
    }
    grid->addWidget(cbxFavColour, 3, 1);

    // Cumpleaños: Day + Month (Properly proportioned!)
    grid->addWidget(new QLabel(tr("Cumpleaños:"), cardSys), 4, 0);
    auto* bdayLay = new QHBoxLayout();
    bdayLay->setSpacing(10);

    spnBirthDay = new QSpinBox(cardSys);
    spnBirthDay->setRange(1, 31);
    setupInput(spnBirthDay, 68);
    bdayLay->addWidget(spnBirthDay);

    cbxBirthMonth = new QComboBox(cardSys);
    setupInput(cbxBirthMonth, 125);
    cbxBirthMonth->addItems({
        QStringLiteral("Enero"), QStringLiteral("Febrero"), QStringLiteral("Marzo"), QStringLiteral("Abril"),
        QStringLiteral("Mayo"), QStringLiteral("Junio"), QStringLiteral("Julio"), QStringLiteral("Agosto"),
        QStringLiteral("Septiembre"), QStringLiteral("Octubre"), QStringLiteral("Noviembre"), QStringLiteral("Diciembre")
    });
    bdayLay->addWidget(cbxBirthMonth);
    bdayLay->addStretch();
    grid->addLayout(bdayLay, 4, 1);

    grid->addWidget(new QLabel(tr("Mensaje Personal:"), cardSys), 5, 0);
    txtMessage = new QLineEdit(cardSys);
    setupInput(txtMessage);
    grid->addWidget(txtMessage, 5, 1);

    lay->addWidget(cardSys);

    // BIOS Files Section
    lay->addWidget(createSectionTitle(tr("Archivos de BIOS y Firmware"), content));
    auto* cardBios = createCard(content);
    auto* biosLay = new QVBoxLayout(cardBios);
    biosLay->setContentsMargins(14, 12, 14, 12);
    biosLay->setSpacing(12);

    swExternalBios = new WatermelonSwitch(cardBios);
    biosLay->addLayout(createSwitchRow(swExternalBios, tr("Usar BIOS/Firmware externo personalizado"), tr("Permite seleccionar archivos binarios originales extraídos de tu Nintendo DS."), cardBios));

    // Expandable container for BIOS paths
    biosFilesContainer = new QWidget(cardBios);
    auto* biosFilesLay = new QVBoxLayout(biosFilesContainer);
    biosFilesLay->setContentsMargins(6, 6, 6, 6);
    biosFilesLay->setSpacing(10);

    const QString binFilter = tr("Archivos binarios (*.bin *.rom);;Todos los archivos (*.*)");

    biosFilesLay->addLayout(createPathRow(tr("BIOS ARM9 (DS):"), txtBios9, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(tr("BIOS ARM7 (DS):"), txtBios7, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(tr("Firmware (DS):"), txtFirmware, binFilter, biosFilesContainer));

    // DSi paths
    auto* lblDSiSep = new QLabel(tr("<b>Archivos adicionales Nintendo DSi:</b>"), biosFilesContainer);
    lblDSiSep->setStyleSheet(QStringLiteral("color: #a09ea8; margin-top: 6px;"));
    biosFilesLay->addWidget(lblDSiSep);

    biosFilesLay->addLayout(createPathRow(tr("BIOS ARM9 (DSi):"), txtDSiBios9, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(tr("BIOS ARM7 (DSi):"), txtDSiBios7, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(tr("Firmware (DSi):"), txtDSiFirmware, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(tr("NAND (DSi):"), txtDSiNAND, binFilter, biosFilesContainer));

    biosLay->addWidget(biosFilesContainer);

    connect(swExternalBios, &WatermelonSwitch::toggled, biosFilesContainer, &QWidget::setVisible);

    lay->addWidget(cardBios);

    // Opciones de Nintendo DSi
    lay->addWidget(createSectionTitle(tr("Opciones de Nintendo DSi"), content));
    auto* cardDSi = createCard(content);
    auto* dsiLay = new QVBoxLayout(cardDSi);
    dsiLay->setContentsMargins(14, 12, 14, 12);
    dsiLay->setSpacing(12);

    swDSiFullBIOSBoot = new WatermelonSwitch(cardDSi);
    dsiLay->addLayout(createSwitchRow(swDSiFullBIOSBoot,
        tr("Arranque completo de BIOS DSi (Full BIOS Boot)"),
        tr("Muestra la pantalla de inicio y reproduce la melodía original de Nintendo DSi."),
        cardDSi));

    swDSPHLE = new WatermelonSwitch(cardDSi);
    dsiLay->addLayout(createSwitchRow(swDSPHLE,
        tr("Emulación rápida de audio DSP (DSP HLE)"),
        tr("Proporciona mayor rendimiento al emular el DSP. Desactívalo para emulación precisa por ciclo (LLE)."),
        cardDSi));

    auto* sepDSi = new QFrame(cardDSi);
    sepDSi->setFrameShape(QFrame::HLine);
    sepDSi->setStyleSheet(QStringLiteral("color: #272632; background: #272632; max-height: 1px; border: none; margin: 4px 0;"));
    dsiLay->addWidget(sepDSi);

    swDSiSDEnable = new WatermelonSwitch(cardDSi);
    dsiLay->addLayout(createSwitchRow(swDSiSDEnable,
        tr("Habilitar tarjeta SD de Nintendo DSi"),
        tr("Permite a las aplicaciones y juegos de DSi acceder a una tarjeta SD interna virtual."),
        cardDSi));

    dsiSDContainer = new QWidget(cardDSi);
    auto* dsiSDContainerLay = new QVBoxLayout(dsiSDContainer);
    dsiSDContainerLay->setContentsMargins(6, 6, 6, 6);
    dsiSDContainerLay->setSpacing(10);

    const QString sdFilter = tr("Imágenes de tarjeta SD (*.bin *.rom *.img *.sd *.dmg);;Todos los archivos (*.*)");
    dsiSDContainerLay->addLayout(createPathRow(tr("Imagen SD DSi:"), txtDSiSDPath, sdFilter, dsiSDContainer));

    auto* dsiSizeRow = new QHBoxLayout();
    dsiSizeRow->setSpacing(10);
    auto* lblDSiSize = new QLabel(tr("Tamaño de imagen:"), dsiSDContainer);
    lblDSiSize->setFixedWidth(120);
    dsiSizeRow->addWidget(lblDSiSize);
    cbxDSiSDSize = new QComboBox(dsiSDContainer);
    cbxDSiSDSize->addItems({tr("Auto"), QStringLiteral("256 MB"), QStringLiteral("512 MB"), QStringLiteral("1 GB"), QStringLiteral("2 GB"), QStringLiteral("4 GB")});
    setupInput(cbxDSiSDSize, 140);
    dsiSizeRow->addWidget(cbxDSiSDSize);
    dsiSizeRow->addStretch();
    dsiSDContainerLay->addLayout(dsiSizeRow);

    swDSiSDReadOnly = new WatermelonSwitch(dsiSDContainer);
    dsiSDContainerLay->addLayout(createSwitchRow(swDSiSDReadOnly,
        tr("Tarjeta SD en modo solo lectura"),
        tr("Impide que la emulación modifique el contenido de la tarjeta SD."),
        dsiSDContainer));

    swDSiSDFolder = new WatermelonSwitch(dsiSDContainer);
    dsiSDContainerLay->addLayout(createSwitchRow(swDSiSDFolder,
        tr("Sincronizar contenido con una carpeta del sistema"),
        tr("Sincroniza automáticamente los archivos de la imagen SD con una carpeta local en tu ordenador."),
        dsiSDContainer));

    dsiSDFolderContainer = new QWidget(dsiSDContainer);
    auto* dsiSDFolderLay = new QVBoxLayout(dsiSDFolderContainer);
    dsiSDFolderLay->setContentsMargins(0, 4, 0, 0);
    dsiSDFolderLay->addLayout(createFolderRow(tr("Carpeta sincronizada:"), txtDSiSDFolder, dsiSDFolderContainer));
    dsiSDContainerLay->addWidget(dsiSDFolderContainer);

    connect(swDSiSDFolder, &WatermelonSwitch::toggled, dsiSDFolderContainer, &QWidget::setVisible);
    connect(swDSiSDEnable, &WatermelonSwitch::toggled, dsiSDContainer, &QWidget::setVisible);

    dsiLay->addWidget(dsiSDContainer);
    lay->addWidget(cardDSi);

    // Cámaras de Nintendo DSi
    lay->addWidget(createSectionTitle(tr("Cámaras de Nintendo DSi"), content));
    auto* cardCam = createCard(content);
    auto* camLay = new QVBoxLayout(cardCam);
    camLay->setContentsMargins(14, 12, 14, 12);
    camLay->setSpacing(12);

    auto* lblCam0Title = new QLabel(tr("<b>Cámara Exterior (Outer Camera):</b>"), cardCam);
    lblCam0Title->setStyleSheet(QStringLiteral("color: #ff5477; font-size: 12px;"));
    camLay->addWidget(lblCam0Title);

    auto* cam0Row = new QHBoxLayout();
    cam0Row->addWidget(new QLabel(tr("Modo de entrada:"), cardCam));
    cbxCam0Type = new QComboBox(cardCam);
    cbxCam0Type->addItems({tr("Desactivada (Imagen negra)"), tr("Imagen fija"), tr("Cámara web del sistema")});
    setupInput(cbxCam0Type, 220);
    cam0Row->addWidget(cbxCam0Type);
    cam0Row->addStretch();
    camLay->addLayout(cam0Row);

    cam0ImageContainer = new QWidget(cardCam);
    auto* c0ImgLay = new QVBoxLayout(cam0ImageContainer);
    c0ImgLay->setContentsMargins(0, 0, 0, 0);
    const QString imgFilter = tr("Imágenes (*.png *.bmp *.jpg *.jpeg);;Todos los archivos (*.*)");
    c0ImgLay->addLayout(createPathRow(tr("Ruta de imagen:"), txtCam0ImagePath, imgFilter, cam0ImageContainer));
    camLay->addWidget(cam0ImageContainer);

    cam0DeviceContainer = new QWidget(cardCam);
    auto* c0DevLay = new QHBoxLayout(cam0DeviceContainer);
    c0DevLay->setContentsMargins(0, 0, 0, 0);
    c0DevLay->addWidget(new QLabel(tr("Dispositivo webcam:"), cam0DeviceContainer));
    cbxCam0Device = new QComboBox(cam0DeviceContainer);
    setupInput(cbxCam0Device);
    c0DevLay->addWidget(cbxCam0Device, 1);
    camLay->addWidget(cam0DeviceContainer);

    swCam0XFlip = new WatermelonSwitch(cardCam);
    camLay->addLayout(createSwitchRow(swCam0XFlip, tr("Invertir horizontalmente"), QString(), cardCam));

    auto* sepCam = new QFrame(cardCam);
    sepCam->setFrameShape(QFrame::HLine);
    sepCam->setStyleSheet(QStringLiteral("color: #272632; background: #272632; max-height: 1px; border: none; margin: 4px 0;"));
    camLay->addWidget(sepCam);

    auto* lblCam1Title = new QLabel(tr("<b>Cámara Interior (Inner Camera):</b>"), cardCam);
    lblCam1Title->setStyleSheet(QStringLiteral("color: #ff5477; font-size: 12px;"));
    camLay->addWidget(lblCam1Title);

    auto* cam1Row = new QHBoxLayout();
    cam1Row->addWidget(new QLabel(tr("Modo de entrada:"), cardCam));
    cbxCam1Type = new QComboBox(cardCam);
    cbxCam1Type->addItems({tr("Desactivada (Imagen negra)"), tr("Imagen fija"), tr("Cámara web del sistema")});
    setupInput(cbxCam1Type, 220);
    cam1Row->addWidget(cbxCam1Type);
    cam1Row->addStretch();
    camLay->addLayout(cam1Row);

    cam1ImageContainer = new QWidget(cardCam);
    auto* c1ImgLay = new QVBoxLayout(cam1ImageContainer);
    c1ImgLay->setContentsMargins(0, 0, 0, 0);
    c1ImgLay->addLayout(createPathRow(tr("Ruta de imagen:"), txtCam1ImagePath, imgFilter, cam1ImageContainer));
    camLay->addWidget(cam1ImageContainer);

    cam1DeviceContainer = new QWidget(cardCam);
    auto* c1DevLay = new QHBoxLayout(cam1DeviceContainer);
    c1DevLay->setContentsMargins(0, 0, 0, 0);
    c1DevLay->addWidget(new QLabel(tr("Dispositivo webcam:"), cam1DeviceContainer));
    cbxCam1Device = new QComboBox(cam1DeviceContainer);
    setupInput(cbxCam1Device);
    c1DevLay->addWidget(cbxCam1Device, 1);
    camLay->addWidget(cam1DeviceContainer);

    swCam1XFlip = new WatermelonSwitch(cardCam);
    camLay->addLayout(createSwitchRow(swCam1XFlip, tr("Invertir horizontalmente"), QString(), cardCam));

#if QT_VERSION >= 0x060000
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraInfo : cameras) {
        cbxCam0Device->addItem(cameraInfo.description());
        cbxCam1Device->addItem(cameraInfo.description());
    }
#endif

    auto updateCamVisibility = [this]() {
        int t0 = cbxCam0Type->currentIndex();
        cam0ImageContainer->setVisible(t0 == 1);
        cam0DeviceContainer->setVisible(t0 == 2);

        int t1 = cbxCam1Type->currentIndex();
        cam1ImageContainer->setVisible(t1 == 1);
        cam1DeviceContainer->setVisible(t1 == 2);
    };

    connect(cbxCam0Type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateCamVisibility);
    connect(cbxCam1Type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateCamVisibility);
    updateCamVisibility();

    lay->addWidget(cardCam);

    // Almacenamiento DLDI (Slot-1 / Homebrew)
    lay->addWidget(createSectionTitle(tr("Almacenamiento DLDI (Slot-1 / Homebrew)"), content));
    auto* cardDLDI = createCard(content);
    auto* dldiLay = new QVBoxLayout(cardDLDI);
    dldiLay->setContentsMargins(14, 12, 14, 12);
    dldiLay->setSpacing(12);

    swDLDIEnable = new WatermelonSwitch(cardDLDI);
    dldiLay->addLayout(createSwitchRow(swDLDIEnable,
        tr("Habilitar emulación DLDI (Slot-1 SD)"),
        tr("Permite que los juegos y aplicaciones homebrew accedan a una tarjeta SD emulada."),
        cardDLDI));

    dldiContainer = new QWidget(cardDLDI);
    auto* dldiContainerLay = new QVBoxLayout(dldiContainer);
    dldiContainerLay->setContentsMargins(6, 6, 6, 6);
    dldiContainerLay->setSpacing(10);

    const QString dldiFilter = tr("Imágenes de disco (*.bin *.rom *.img *.dmg);;Todos los archivos (*.*)");
    dldiContainerLay->addLayout(createPathRow(tr("Imagen SD DLDI:"), txtDLDISDPath, dldiFilter, dldiContainer));

    auto* dldiSizeRow = new QHBoxLayout();
    dldiSizeRow->setSpacing(10);
    auto* lblDLDISize = new QLabel(tr("Tamaño de imagen:"), dldiContainer);
    lblDLDISize->setFixedWidth(120);
    dldiSizeRow->addWidget(lblDLDISize);
    cbxDLDISize = new QComboBox(dldiContainer);
    cbxDLDISize->addItems({tr("Auto"), QStringLiteral("256 MB"), QStringLiteral("512 MB"), QStringLiteral("1 GB"), QStringLiteral("2 GB"), QStringLiteral("4 GB")});
    setupInput(cbxDLDISize, 140);
    dldiSizeRow->addWidget(cbxDLDISize);
    dldiSizeRow->addStretch();
    dldiContainerLay->addLayout(dldiSizeRow);

    swDLDIReadOnly = new WatermelonSwitch(dldiContainer);
    dldiContainerLay->addLayout(createSwitchRow(swDLDIReadOnly,
        tr("Tarjeta DLDI en modo solo lectura"),
        tr("Impide que la emulación modifique el contenido de la imagen DLDI."),
        dldiContainer));

    swDLDIFolder = new WatermelonSwitch(dldiContainer);
    dldiContainerLay->addLayout(createSwitchRow(swDLDIFolder,
        tr("Sincronizar contenido con una carpeta del sistema"),
        tr("Sincroniza automáticamente los archivos de la imagen DLDI con una carpeta local en tu ordenador."),
        dldiContainer));

    dldiFolderContainer = new QWidget(dldiContainer);
    auto* dldiFolderLay = new QVBoxLayout(dldiFolderContainer);
    dldiFolderLay->setContentsMargins(0, 4, 0, 0);
    dldiFolderLay->addLayout(createFolderRow(tr("Carpeta sincronizada:"), txtDLDIFolder, dldiFolderContainer));
    dldiContainerLay->addWidget(dldiFolderContainer);

    connect(swDLDIFolder, &WatermelonSwitch::toggled, dldiFolderContainer, &QWidget::setVisible);
    connect(swDLDIEnable, &WatermelonSwitch::toggled, dldiContainer, &QWidget::setVisible);

    dldiLay->addWidget(dldiContainer);
    lay->addWidget(cardDLDI);

    // Fecha y Hora (Reloj RTC)
    lay->addWidget(createSectionTitle(tr("Fecha y Hora del Sistema (RTC)"), content));
    auto* cardRTC = createCard(content);
    auto* rtcLay = new QVBoxLayout(cardRTC);
    rtcLay->setContentsMargins(14, 12, 14, 12);
    rtcLay->setSpacing(12);

    swCustomRTC = new WatermelonSwitch(cardRTC);
    rtcLay->addLayout(createSwitchRow(swCustomRTC, tr("Usar fecha y hora personalizada"),
        tr("Si está desactivado, el reloj interno de la consola se sincroniza automáticamente con el de tu ordenador."), cardRTC));

    rtcContainer = new QWidget(cardRTC);
    auto* rtcContainerLay = new QHBoxLayout(rtcContainer);
    rtcContainerLay->setContentsMargins(6, 4, 6, 4);
    rtcContainerLay->setSpacing(10);

    auto* lblRTC = new QLabel(tr("Fecha y hora fijada:"), rtcContainer);
    lblRTC->setFixedWidth(140);
    rtcContainerLay->addWidget(lblRTC);

    dteCustomRTC = new QDateTimeEdit(QDateTime::currentDateTime(), rtcContainer);
    dteCustomRTC->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    dteCustomRTC->setCalendarPopup(true);
    setupInput(dteCustomRTC, 180);
    rtcContainerLay->addWidget(dteCustomRTC);

    btnRTCNow = new QPushButton(tr("Ahora"), rtcContainer);
    btnRTCNow->setCursor(Qt::PointingHandCursor);
    setupInput(btnRTCNow, 70);
    btnRTCNow->setStyleSheet(QStringLiteral(
        "QPushButton { background: #252430; border: 1px solid #363444; border-radius: 6px; "
        "color: #f6f5f3; padding: 2px 10px; font-size: 12px; } "
        "QPushButton:hover { background: #31303e; border-color: #ff5477; color: #ff5477; }"));
    connect(btnRTCNow, &QPushButton::clicked, this, [this]() {
        if (dteCustomRTC) dteCustomRTC->setDateTime(QDateTime::currentDateTime());
    });
    rtcContainerLay->addWidget(btnRTCNow);
    rtcContainerLay->addStretch();

    rtcLay->addWidget(rtcContainer);
    connect(swCustomRTC, &WatermelonSwitch::toggled, rtcContainer, &QWidget::setVisible);
    lay->addWidget(cardRTC);

    // Gestión de Energía y Batería
    lay->addWidget(createSectionTitle(tr("Gestión de Energía y Batería"), content));
    auto* cardPower = createCard(content);
    auto* powerLay = new QVBoxLayout(cardPower);
    powerLay->setContentsMargins(14, 12, 14, 12);
    powerLay->setSpacing(12);

    auto* powerGrid = new QGridLayout();
    powerGrid->setContentsMargins(6, 4, 6, 4);
    powerGrid->setHorizontalSpacing(16);
    powerGrid->setVerticalSpacing(12);

    powerGrid->addWidget(new QLabel(tr("Batería Nintendo DS:"), cardPower), 0, 0);
    cbxDSBattery = new QComboBox(cardPower);
    cbxDSBattery->addItem(tr("Normal (Carga suficiente / LED verde)"), true);
    cbxDSBattery->addItem(tr("Batería baja (LED rojo)"), false);
    setupInput(cbxDSBattery);
    powerGrid->addWidget(cbxDSBattery, 0, 1);

    powerGrid->addWidget(new QLabel(tr("Nivel de batería Nintendo DSi:"), cardPower), 1, 0);
    cbxDSiBattery = new QComboBox(cardPower);
    cbxDSiBattery->addItem(tr("Nivel 4 - Lleno (100%)"), 0xF);
    cbxDSiBattery->addItem(tr("Nivel 3 - Tres cuartos (75%)"), 0xB);
    cbxDSiBattery->addItem(tr("Nivel 2 - Medio (50%)"), 0x7);
    cbxDSiBattery->addItem(tr("Nivel 1 - Bajo (25%)"), 0x3);
    cbxDSiBattery->addItem(tr("Nivel 0 - Casi agotado"), 0x1);
    setupInput(cbxDSiBattery);
    powerGrid->addWidget(cbxDSiBattery, 1, 1);

    powerLay->addLayout(powerGrid);

    swDSiCharging = new WatermelonSwitch(cardPower);
    powerLay->addLayout(createSwitchRow(swDSiCharging, tr("DSi conectada al cargador"),
        tr("Simula que la consola Nintendo DSi está enchufada a la corriente y cargando."), cardPower));

    lay->addWidget(cardPower);

    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 3: CPU
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createCpuPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(18);

    lay->addWidget(createSectionTitle(tr("Recompilador JIT (Just-In-Time)"), content));

    auto* cardJit = createCard(content);
    auto* jitLay = new QVBoxLayout(cardJit);
    jitLay->setContentsMargins(14, 12, 14, 12);
    jitLay->setSpacing(12);

    swJitEnable = new WatermelonSwitch(cardJit);
    jitLay->addLayout(createSwitchRow(swJitEnable, tr("Habilitar Recompilador JIT"), tr("Aumenta drásticamente el rendimiento compilando código ARM al vuelo."), cardJit));

    auto* blockRow = new QHBoxLayout();
    blockRow->setContentsMargins(6, 4, 6, 4);
    auto* lblBlock = new QLabel(tr("Tamaño Máximo de Bloque JIT:"), cardJit);
    lblBlock->setStyleSheet(QStringLiteral("font-weight: 600; color: #f6f5f3;"));
    blockRow->addWidget(lblBlock);
    blockRow->addStretch();
    spnJitBlockSize = new QSpinBox(cardJit);
    spnJitBlockSize->setRange(1, 128);
    spnJitBlockSize->setValue(32);
    setupInput(spnJitBlockSize, 70);
    blockRow->addWidget(spnJitBlockSize);
    jitLay->addLayout(blockRow);

    swJitBranchOpt = new WatermelonSwitch(cardJit);
    jitLay->addLayout(createSwitchRow(swJitBranchOpt, tr("Optimizaciones de saltos (Branch Optimisations)"), tr("Optimiza saltos condicionales en el código emulado."), cardJit));

    swJitLiteralOpt = new WatermelonSwitch(cardJit);
    jitLay->addLayout(createSwitchRow(swJitLiteralOpt, tr("Optimizaciones de literales (Literal Optimisations)"), tr("Optimiza constantes y literales en memoria."), cardJit));

    swJitFastMem = new WatermelonSwitch(cardJit);
    jitLay->addLayout(createSwitchRow(swJitFastMem, tr("Memoria Rápida (Fast Memory)"), tr("Acelera los accesos a la memoria principal de la consola."), cardJit));

    lay->addWidget(cardJit);
    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 4: Gráficos
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createGraphicsPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(18);

    lay->addWidget(createSectionTitle(tr("Motor de Renderizado 3D"), content));

    auto* cardGfx = createCard(content);
    auto* grid = new QGridLayout(cardGfx);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(tr("Renderizador 3D:"), cardGfx), 0, 0);
    cbxRenderer3D = new QComboBox(cardGfx);
    cbxRenderer3D->addItem(tr("Software (Renderizado por CPU)"), 0);
    cbxRenderer3D->addItem(QStringLiteral("OpenGL"), 1);
    cbxRenderer3D->addItem(QStringLiteral("OpenGL Compute"), 2);
    setupInput(cbxRenderer3D);
    grid->addWidget(cbxRenderer3D, 0, 1);

    grid->addWidget(new QLabel(tr("Multiplicador de Resolución:"), cardGfx), 1, 0);
    cbxScaleFactor = new QComboBox(cardGfx);
    cbxScaleFactor->addItems({
        tr("1x Nativo (256x192)"),
        tr("2x (512x384)"),
        tr("3x (768x576)"),
        tr("4x HD (1024x768)"),
        tr("5x (1280x960)"),
        tr("6x (1536x1152)"),
        tr("8x UHD (2048x1536)")
    });
    setupInput(cbxScaleFactor);
    grid->addWidget(cbxScaleFactor, 1, 1);

    grid->addWidget(new QLabel(tr("Shader de Postprocesado:"), cardGfx), 2, 0);
    cbxShader = new QComboBox(cardGfx);
    cbxShader->addItems({
        tr("Ninguno (Directo)"),
        tr("CRT-Lottes (Efecto Retro)"),
        tr("LCD3x (Rejilla Nintendo DS)"),
        tr("ScaleFx (Suavizado HD)"),
        tr("xBRZ (Filtro Pixel Art)")
    });
    setupInput(cbxShader);
    grid->addWidget(cbxShader, 2, 1);

    lay->addWidget(cardGfx);

    lay->addWidget(createSectionTitle(tr("Opciones de Pantalla y Polígonos"), content));

    auto* cardOpts = createCard(content);
    auto* optsLay = new QVBoxLayout(cardOpts);
    optsLay->setContentsMargins(14, 12, 14, 12);
    optsLay->setSpacing(12);

    swVSync = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swVSync, tr("Sincronización Vertical (VSync)"), tr("Evita el desgarro de pantalla (tearing) sincronizando con la tasa de refresco del monitor."), cardOpts));

    swBetterPolygons = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swBetterPolygons, tr("Mejorar Polígonos (Better Polygons)"), tr("Reduce artefactos de costuras y deformaciones poligonales en 3D."), cardOpts));

    swHiresCoords = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swHiresCoords, tr("Coordenadas de Alta Resolución"), tr("Permite que los vértices utilicen precisión subpíxel al aumentar la resolución."), cardOpts));

    swSoftThreaded = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swSoftThreaded, tr("Renderizado por Software Multihilo"), tr("Utiliza múltiples núcleos de CPU al usar el motor por Software."), cardOpts));

    lay->addWidget(cardOpts);
    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 5: Pantalla
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createDisplayPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(18);

    lay->addWidget(createSectionTitle(tr("Disposición de Pantallas"), content));

    auto* cardLayout = createCard(content);
    auto* grid = new QGridLayout(cardLayout);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(tr("Tamaño de ventana:"), cardLayout), 0, 0);
    cbxWindowSize = new QComboBox(cardLayout);
    cbxWindowSize->addItem(QStringLiteral("1x"), 1);
    cbxWindowSize->addItem(QStringLiteral("2x"), 2);
    cbxWindowSize->addItem(QStringLiteral("3x"), 3);
    cbxWindowSize->addItem(QStringLiteral("4x"), 4);
    setupInput(cbxWindowSize);
    grid->addWidget(cbxWindowSize, 0, 1);

    grid->addWidget(new QLabel(tr("Rotación de pantalla:"), cardLayout), 1, 0);
    cbxScreenRotation = new QComboBox(cardLayout);
    cbxScreenRotation->addItem(QStringLiteral("0°"), 0);
    cbxScreenRotation->addItem(QStringLiteral("90°"), 1);
    cbxScreenRotation->addItem(QStringLiteral("180°"), 2);
    cbxScreenRotation->addItem(QStringLiteral("270°"), 3);
    setupInput(cbxScreenRotation);
    grid->addWidget(cbxScreenRotation, 1, 1);

    grid->addWidget(new QLabel(tr("Separación entre pantallas:"), cardLayout), 2, 0);
    spnScreenGap = new QSpinBox(cardLayout);
    spnScreenGap->setRange(0, 500);
    spnScreenGap->setSuffix(tr(" px"));
    setupInput(spnScreenGap);
    grid->addWidget(spnScreenGap, 2, 1);

    grid->addWidget(new QLabel(tr("Disposición:"), cardLayout), 3, 0);
    cbxScreenLayout = new QComboBox(cardLayout);
    cbxScreenLayout->addItem(tr("Natural"), screenLayout_Natural);
    cbxScreenLayout->addItem(tr("Vertical"), screenLayout_Vertical);
    cbxScreenLayout->addItem(tr("Horizontal"), screenLayout_Horizontal);
    cbxScreenLayout->addItem(tr("Híbrida"), screenLayout_Hybrid);
    setupInput(cbxScreenLayout);
    grid->addWidget(cbxScreenLayout, 3, 1);

    grid->addWidget(new QLabel(tr("Tamaño de pantallas:"), cardLayout), 4, 0);
    cbxScreenSizing = new QComboBox(cardLayout);
    cbxScreenSizing->addItem(tr("Iguales"), screenSizing_Even);
    cbxScreenSizing->addItem(tr("Énfasis superior"), screenSizing_EmphTop);
    cbxScreenSizing->addItem(tr("Énfasis inferior"), screenSizing_EmphBot);
    cbxScreenSizing->addItem(tr("Automático"), screenSizing_Auto);
    cbxScreenSizing->addItem(tr("Solo superior"), screenSizing_TopOnly);
    cbxScreenSizing->addItem(tr("Solo inferior"), screenSizing_BotOnly);
    setupInput(cbxScreenSizing);
    grid->addWidget(cbxScreenSizing, 4, 1);

    grid->addWidget(new QLabel(tr("Aspecto pantalla superior:"), cardLayout), 5, 0);
    cbxScreenAspectTop = new QComboBox(cardLayout);
    for (int i = 0; i < AspectRatiosNum; i++)
        cbxScreenAspectTop->addItem(QString::fromLatin1(aspectRatios[i].label), aspectRatios[i].id);
    setupInput(cbxScreenAspectTop);
    grid->addWidget(cbxScreenAspectTop, 5, 1);

    grid->addWidget(new QLabel(tr("Aspecto pantalla inferior:"), cardLayout), 6, 0);
    cbxScreenAspectBot = new QComboBox(cardLayout);
    for (int i = 0; i < AspectRatiosNum; i++)
        cbxScreenAspectBot->addItem(QString::fromLatin1(aspectRatios[i].label), aspectRatios[i].id);
    setupInput(cbxScreenAspectBot);
    grid->addWidget(cbxScreenAspectBot, 6, 1);

    lay->addWidget(cardLayout);

    lay->addWidget(createSectionTitle(tr("Opciones de Visualización"), content));

    auto* cardOpts = createCard(content);
    auto* optsLay = new QVBoxLayout(cardOpts);
    optsLay->setContentsMargins(14, 12, 14, 12);
    optsLay->setSpacing(12);

    swScreenSwap = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swScreenSwap, tr("Intercambiar pantallas"),
        tr("Invierte la pantalla superior y la inferior."), cardOpts));

    swIntegerScaling = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swIntegerScaling, tr("Forzar escalado entero"),
        tr("Mantiene los píxeles nítidos evitando interpolación al escalar."), cardOpts));

    swScreenFilter = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swScreenFilter, tr("Filtro de pantalla"),
        tr("Suaviza la imagen al ampliarla."), cardOpts));

    lay->addWidget(cardOpts);
    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 6: Audio
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createAudioPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(18);

    lay->addWidget(createSectionTitle(tr("Salida de Sonido"), content));

    auto* cardAud = createCard(content);
    auto* grid = new QGridLayout(cardAud);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(tr("Volumen Maestro:"), cardAud), 0, 0);
    auto* volLay = new QHBoxLayout();
    sldVolume = new QSlider(Qt::Horizontal, cardAud);
    sldVolume->setRange(0, 256);
    sldVolume->setValue(256);
    volLay->addWidget(sldVolume);

    lblVolumeVal = new QLabel(tr("100%"), cardAud);
    lblVolumeVal->setFixedWidth(46);
    lblVolumeVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    volLay->addWidget(lblVolumeVal);
    grid->addLayout(volLay, 0, 1);

    connect(sldVolume, &QSlider::valueChanged, this, [this](int v) {
        int pct = qRound((v / 256.0) * 100.0);
        lblVolumeVal->setText(tr("%1%").arg(pct));
    });

    grid->addWidget(new QLabel(tr("Interpolación de Audio:"), cardAud), 1, 0);
    cbxInterpolation = new QComboBox(cardAud);
    cbxInterpolation->addItems({
        tr("Ninguna (Sonido original DS)"),
        tr("Lineal (Suavizado rápido)"),
        QStringLiteral("Coseno"),
        tr("Cúbica (Mayor calidad)")
    });
    setupInput(cbxInterpolation);
    grid->addWidget(cbxInterpolation, 1, 1);

    grid->addWidget(new QLabel(tr("Profundidad de Bits:"), cardAud), 2, 0);
    cbxBitDepth = new QComboBox(cardAud);
    cbxBitDepth->addItems({
        tr("Automático (10 bits en DS, 16 bits en DSi)"),
        tr("Forzar 10 bits (Sonido DS auténtico)"),
        tr("Forzar 16 bits (Alta fidelidad)")
    });
    setupInput(cbxBitDepth);
    grid->addWidget(cbxBitDepth, 2, 1);

    swDSiVolumeSync = new WatermelonSwitch(cardAud);
    auto* dsiVolLay = createSwitchRow(swDSiVolumeSync,
        tr("Sincronizar volumen maestro con la consola DSi"),
        tr("Sincroniza el nivel de volumen emulado con los controles internos de DSi."),
        cardAud);
    auto* dsiVolWrap = new QWidget(cardAud);
    dsiVolWrap->setLayout(dsiVolLay);
    grid->addWidget(dsiVolWrap, 3, 0, 1, 2);

    lay->addWidget(cardAud);

    // Entrada de Micrófono
    lay->addWidget(createSectionTitle(tr("Entrada de Micrófono"), content));
    auto* cardMic = createCard(content);
    auto* micLay = new QVBoxLayout(cardMic);
    micLay->setContentsMargins(14, 12, 14, 12);
    micLay->setSpacing(10);

    auto* micTypeRow = new QHBoxLayout();
    micTypeRow->addWidget(new QLabel(tr("Modo de entrada de micrófono:"), cardMic));
    cbxMicInputType = new QComboBox(cardMic);
    cbxMicInputType->addItems({
        tr("Silencio (Sin entrada)"),
        tr("Micrófono del sistema (Físico)"),
        tr("Ruido blanco (Soplido constante)"),
        tr("Archivo de audio WAV personalizado")
    });
    setupInput(cbxMicInputType, 260);
    micTypeRow->addWidget(cbxMicInputType);
    micTypeRow->addStretch();
    micLay->addLayout(micTypeRow);

    micDeviceContainer = new QWidget(cardMic);
    auto* devLay = new QHBoxLayout(micDeviceContainer);
    devLay->setContentsMargins(0, 0, 0, 0);
    devLay->addWidget(new QLabel(tr("Dispositivo de micrófono:"), micDeviceContainer));
    cbxMicDevice = new QComboBox(micDeviceContainer);
    setupInput(cbxMicDevice);
    devLay->addWidget(cbxMicDevice, 1);
    micLay->addWidget(micDeviceContainer);

    // Populate SDL audio input devices
    const int numDevs = SDL_GetNumAudioDevices(true);
    for (int i = 0; i < numDevs; i++) {
        cbxMicDevice->addItem(QString::fromUtf8(SDL_GetAudioDeviceName(i, true)));
    }

    micWavContainer = new QWidget(cardMic);
    auto* wavLay = new QVBoxLayout(micWavContainer);
    wavLay->setContentsMargins(0, 0, 0, 0);
    const QString wavFilter = tr("Archivos de audio (*.wav);;Todos los archivos (*.*)");
    wavLay->addLayout(createPathRow(tr("Archivo WAV:"), txtMicWavPath, wavFilter, micWavContainer));
    micLay->addWidget(micWavContainer);

    auto updateMicVisibility = [this]() {
        int t = cbxMicInputType->currentIndex();
        micDeviceContainer->setVisible(t == 1);
        micWavContainer->setVisible(t == 3);
    };
    connect(cbxMicInputType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateMicVisibility);
    updateMicVisibility();

    lay->addWidget(cardMic);
    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 6: Controles (Original melonDS Nintendo DS Visual Controller Mapping!)
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createControlsPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(16, 12, 16, 12);
    lay->setSpacing(12);

    MainWindow* mw = findMainWindow(this);
    if (mw) {
        inputConfigWidget = new InputConfigDialog(mw);
        inputConfigWidget->setWindowFlags(Qt::Widget);
        inputConfigWidget->setAttribute(Qt::WA_DeleteOnClose, false);

        // Hide the OK/Cancel buttonbox from the embedded dialog so only WatermelonSettingsDialog's buttons are used
        if (auto* bbox = inputConfigWidget->findChild<QDialogButtonBox*>()) {
            bbox->hide();
        }

        // Apply dark styling to the embedded InputConfigDialog
        inputConfigWidget->setStyleSheet(QStringLiteral(
            "QGroupBox { font-weight: 700; color: #ff5477; border: 1px solid #2d2c39; border-radius: 8px; margin-top: 8px; padding-top: 10px; }"
            "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 6px; background-color: #141317; }"
            "QTabWidget::pane { border: 1px solid #2d2c39; border-radius: 8px; background: #16151c; }"
            "QTabBar::tab { background: #1f1e27; color: #a09ea8; padding: 8px 18px; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; font-weight: 600; }"
            "QTabBar::tab:selected { background: #281920; color: #ff5477; border-bottom: 2px solid #ff5477; }"
            "QPushButton { background: #22212c; border: 1px solid #32303e; border-radius: 6px; color: #f6f5f3; padding: 5px 12px; font-weight: 600; }"
            "QPushButton:hover { background: #2e2c3b; border-color: #ff5477; color: #ff5477; }"
            "QComboBox { background: #1c1b24; border: 1px solid #2e2d3c; border-radius: 6px; color: #f6f5f3; padding: 4px 10px; }"
        ));

        lay->addWidget(inputConfigWidget);
    } else {
        auto* cardFallback = createCard(content);
        auto* fbLay = new QVBoxLayout(cardFallback);
        auto* lblErr = new QLabel(tr("Abre el emulador con la ventana principal para configurar los controles."), cardFallback);
        fbLay->addWidget(lblErr);
        lay->addWidget(cardFallback);
    }

    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 8: Multijugador y Red (Local Multiplayer & Nintendo WFC / LAN)
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createMultiplayerPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(18);

    // Section 1: Multijugador Local (melonDS)
    lay->addWidget(createSectionTitle(tr("Multijugador Local (melonDS)"), content));
    auto* cardMP = createCard(content);
    auto* gridMP = new QGridLayout(cardMP);
    gridMP->setContentsMargins(16, 16, 16, 16);
    gridMP->setHorizontalSpacing(16);
    gridMP->setVerticalSpacing(14);

    gridMP->addWidget(new QLabel(tr("Modo de audio multijugador:"), cardMP), 0, 0);
    cbxMPAudioMode = new QComboBox(cardMP);
    cbxMPAudioMode->addItem(tr("Todas las instancias"), 0);
    cbxMPAudioMode->addItem(tr("Solo la primera instancia"), 1);
    cbxMPAudioMode->addItem(tr("Solo la instancia enfocada"), 2);
    setupInput(cbxMPAudioMode);
    gridMP->addWidget(cbxMPAudioMode, 0, 1);

    gridMP->addWidget(new QLabel(tr("Tiempo límite de recepción:"), cardMP), 1, 0);
    spnMPRecvTimeout = new QSpinBox(cardMP);
    spnMPRecvTimeout->setRange(1, 1000);
    spnMPRecvTimeout->setSuffix(QStringLiteral(" ms"));
    setupInput(spnMPRecvTimeout, 100);
    gridMP->addWidget(spnMPRecvTimeout, 1, 1);
    lay->addWidget(cardMP);

    // Section 2: Wi-Fi y Conexión de Red (Nintendo WFC / LAN)
    lay->addWidget(createSectionTitle(tr("Wi-Fi y Conexión de Red (Nintendo WFC / LAN)"), content));
    auto* cardNet = createCard(content);
    auto* netLay = new QVBoxLayout(cardNet);
    netLay->setContentsMargins(16, 16, 16, 16);
    netLay->setSpacing(12);

    auto* grpMode = new QButtonGroup(cardNet);
    rbIndirectMode = new QRadioButton(tr("Modo Indirecto / Slirp (Recomendado)"), cardNet);
    rbIndirectMode->setToolTip(tr("Emula una red virtual NAT. No requiere privilegios de administrador ni configuración del adaptador físico."));
    rbIndirectMode->setCursor(Qt::PointingHandCursor);
    grpMode->addButton(rbIndirectMode);
    netLay->addWidget(rbIndirectMode);

    auto* lblIndirectDesc = new QLabel(tr("Emula una red NAT virtual. Recomendado para la mayoría de juegos y servicios WFC alternativos."), cardNet);
    lblIndirectDesc->setStyleSheet(QStringLiteral("color: #7b788a; font-size: 11px; margin-left: 24px; margin-bottom: 4px;"));
    lblIndirectDesc->setWordWrap(true);
    netLay->addWidget(lblIndirectDesc);

    rbDirectMode = new QRadioButton(tr("Modo Directo / PCap (Requiere %1 y conexión ethernet por cable)").arg(QStringLiteral(PCAP_NAME)), cardNet);
    rbDirectMode->setToolTip(tr("Conecta la consola emulada directamente al adaptador físico."));
    rbDirectMode->setCursor(Qt::PointingHandCursor);
    grpMode->addButton(rbDirectMode);
    netLay->addWidget(rbDirectMode);

    adapterContainer = new QWidget(cardNet);
    auto* adapterLay = new QGridLayout(adapterContainer);
    adapterLay->setContentsMargins(24, 6, 0, 0);
    adapterLay->setHorizontalSpacing(16);
    adapterLay->setVerticalSpacing(10);

    adapterLay->addWidget(new QLabel(tr("Adaptador de red:"), adapterContainer), 0, 0);
    cbxNetAdapter = new QComboBox(adapterContainer);
    setupInput(cbxNetAdapter);
    adapterLay->addWidget(cbxNetAdapter, 0, 1);

    adapterLay->addWidget(new QLabel(tr("Dirección MAC:"), adapterContainer), 1, 0);
    lblAdapterMAC = new QLabel(QStringLiteral("(desconocida)"), adapterContainer);
    lblAdapterMAC->setStyleSheet(QStringLiteral("color: #a09ea8; font-weight: 600;"));
    adapterLay->addWidget(lblAdapterMAC, 1, 1);

    adapterLay->addWidget(new QLabel(tr("Dirección IP:"), adapterContainer), 2, 0);
    lblAdapterIP = new QLabel(QStringLiteral("(desconocida)"), adapterContainer);
    lblAdapterIP->setStyleSheet(QStringLiteral("color: #a09ea8; font-weight: 600;"));
    adapterLay->addWidget(lblAdapterIP, 2, 1);

    netLay->addWidget(adapterContainer);

    if (!hasNetPcap)
    {
        auto* lblNoPcap = new QLabel(tr("⚠️ %1 no está disponible en este sistema. El modo directo está desactivado.").arg(QStringLiteral(PCAP_NAME)), cardNet);
        lblNoPcap->setStyleSheet(QStringLiteral("color: #e5a04d; font-size: 11px; margin-top: 6px;"));
        lblNoPcap->setWordWrap(true);
        netLay->addWidget(lblNoPcap);
    }

    connect(rbDirectMode, &QRadioButton::toggled, this, &WatermelonSettingsDialog::updateNetAdapterControls);
    connect(rbIndirectMode, &QRadioButton::toggled, this, &WatermelonSettingsDialog::updateNetAdapterControls);
    connect(cbxNetAdapter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WatermelonSettingsDialog::onNetAdapterIndexChanged);

    lay->addWidget(cardNet);
    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

void WatermelonSettingsDialog::updateNetAdapterControls()
{
    bool enableDirect = hasNetPcap && rbDirectMode && rbDirectMode->isChecked();
    if (adapterContainer)
        adapterContainer->setEnabled(enableDirect);
}

void WatermelonSettingsDialog::onNetAdapterIndexChanged(int sel)
{
    if (!hasNetPcap || sel < 0 || sel >= (int)netAdapters.size() || netAdapters.empty())
    {
        if (lblAdapterMAC) lblAdapterMAC->setText(QStringLiteral("(ninguno)"));
        if (lblAdapterIP) lblAdapterIP->setText(QStringLiteral("(ninguno)"));
        return;
    }

    const melonDS::AdapterData& adapter = netAdapters[sel];
    char macBuf[64];
    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
             adapter.MAC[0], adapter.MAC[1], adapter.MAC[2],
             adapter.MAC[3], adapter.MAC[4], adapter.MAC[5]);
    if (lblAdapterMAC) lblAdapterMAC->setText(QString::fromLatin1(macBuf));

    char ipBuf[64];
    snprintf(ipBuf, sizeof(ipBuf), "%d.%d.%d.%d",
             adapter.IP_v4[0], adapter.IP_v4[1],
             adapter.IP_v4[2], adapter.IP_v4[3]);
    if (lblAdapterIP) lblAdapterIP->setText(QString::fromLatin1(ipBuf));
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 9: RetroAchievements (Full Options!)
// ───────────────────────────────────────────────────────────────────────────

QWidget* WatermelonSettingsDialog::createRAPage()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget();
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(18);

    lay->addWidget(createSectionTitle(tr("RetroAchievements - Opciones de Juego"), content));

    auto* cardRA = createCard(content);
    auto* raLay = new QVBoxLayout(cardRA);
    raLay->setContentsMargins(14, 12, 14, 12);
    raLay->setSpacing(12);

    swRAEnabled = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRAEnabled, tr("Habilitar RetroAchievements"), tr("Desbloquea logros mientras juegas y sincroniza tu perfil con retroachievements.org."), cardRA));

    swRAHardcore = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRAHardcore, tr("Modo Hardcore"), tr("Deshabilita trucos y guardados rápidos (Savestates) para ganar el doble de puntuación oficial."), cardRA));

    swRARichPresence = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRARichPresence, tr("Presencia Enriquecida (Rich Presence)"), tr("Muestra en tiempo real en tu perfil qué nivel, zona o capítulo estás jugando."), cardRA));

    swRASound = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRASound, tr("Efectos de Sonido"), tr("Reproduce el sonido clásico de logro desbloqueado al conseguir una insignia."), cardRA));

    swRAUnofficial = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRAUnofficial, tr("Logros No Oficiales (Unofficial)"), tr("Permite cargar conjuntos de logros en fase de pruebas o desarrollo."), cardRA));

    swRAEncore = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRAEncore, tr("Modo Encore"), tr("Permite volver a desbloquear logros que ya habías completado anteriormente para practicar."), cardRA));

    lay->addWidget(cardRA);

    lay->addWidget(createSectionTitle(tr("Cuenta y Credenciales"), content));

    auto* cardAcc = createCard(content);
    auto* grid = new QGridLayout(cardAcc);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(tr("Usuario:"), cardAcc), 0, 0);
    txtRAUser = new QLineEdit(cardAcc);
    setupInput(txtRAUser);
    grid->addWidget(txtRAUser, 0, 1);

    grid->addWidget(new QLabel(tr("Token de Conexión:"), cardAcc), 1, 0);
    txtRAToken = new QLineEdit(cardAcc);
    txtRAToken->setEchoMode(QLineEdit::Password);
    setupInput(txtRAToken);
    grid->addWidget(txtRAToken, 1, 1);

    lblRAStatus = new QLabel(cardAcc);
    lblRAStatus->setStyleSheet(QStringLiteral("color: #6fbf4a; font-weight: 600; font-size: 12px;"));
    grid->addWidget(lblRAStatus, 2, 0, 1, 2);

    lay->addWidget(cardAcc);
    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Load / Save / Defaults
// ───────────────────────────────────────────────────────────────────────────

void WatermelonSettingsDialog::loadSettings()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    Config::Table localCfg0 = Config::GetLocalTable(0);

    // General
    swPauseLostFocus->setChecked(globalCfg.GetBool("PauseLostFocus"));
    swMouseHide->setChecked(globalCfg.GetBool("Mouse.Hide"));
    spnMouseHideSeconds->setValue(qBound(1, globalCfg.GetInt("Mouse.HideSeconds"), 30));
    double targetFps = globalCfg.GetDouble("TargetFPS");
    spnTargetFPS->setValue(targetFps > 0 ? targetFps : 60.0);
    double ffwdFps = globalCfg.GetDouble("FastForwardFPS");
    spnFastForwardFPS->setValue(ffwdFps > 0 ? ffwdFps : 120.0);
    double slowFps = globalCfg.GetDouble("SlowmoFPS");
    spnSlowmoFPS->setValue(slowFps > 0 ? slowFps : 30.0);
    swDirectBoot->setChecked(globalCfg.GetBool("Emu.DirectBoot"));
    cbxUILanguage->setCurrentIndex(qMax(0, cbxUILanguage->findData(globalCfg.GetQString("UI.Language"))));

    listRomDirs->clear();
    QString dirsStr = globalCfg.GetQString("WatermelonRomDirs");
    if (!dirsStr.isEmpty()) {
        const QStringList dirs = dirsStr.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        for (const QString& d : dirs) {
            listRomDirs->addItem(d.trimmed());
        }
    }

    txtSaveFilePath->setText(localCfg0.GetQString("SaveFilePath"));
    txtSavestatePath->setText(localCfg0.GetQString("SavestatePath"));
    txtCheatFilePath->setText(localCfg0.GetQString("CheatFilePath"));
    txtCheatDbPath->setText(globalCfg.GetQString("WatermelonCheatDbPath"));
    swSavestateRelocSRAM->setChecked(globalCfg.GetBool("Savestate.RelocSRAM"));

    // System
    cbxConsoleType->setCurrentIndex(globalCfg.GetInt("Emu.ConsoleType"));
    bool extBios = globalCfg.GetBool("Emu.ExternalBIOSEnable");
    swExternalBios->setChecked(extBios);
    biosFilesContainer->setVisible(extBios);

    txtBios9->setText(globalCfg.GetQString("DS.BIOS9Path"));
    txtBios7->setText(globalCfg.GetQString("DS.BIOS7Path"));
    txtFirmware->setText(globalCfg.GetQString("DS.FirmwarePath"));
    txtDSiBios9->setText(globalCfg.GetQString("DSi.BIOS9Path"));
    txtDSiBios7->setText(globalCfg.GetQString("DSi.BIOS7Path"));
    txtDSiFirmware->setText(globalCfg.GetQString("DSi.FirmwarePath"));
    txtDSiNAND->setText(globalCfg.GetQString("DSi.NANDPath"));

    Config::Table firmCfg = globalCfg.GetTable("Firmware");
    cbxLanguage->setCurrentIndex(firmCfg.GetInt("Language"));
    txtUsername->setText(firmCfg.GetQString("Username"));
    cbxFavColour->setCurrentIndex(qBound(0, firmCfg.GetInt("FavouriteColour"), 15));
    spnBirthDay->setValue(qMax(1, firmCfg.GetInt("BirthdayDay")));
    cbxBirthMonth->setCurrentIndex(qMax(0, firmCfg.GetInt("BirthdayMonth") - 1));
    txtMessage->setText(firmCfg.GetQString("Message"));

    // DSi options
    swDSiFullBIOSBoot->setChecked(globalCfg.GetBool("DSi.FullBIOSBoot"));
    swDSPHLE->setChecked(globalCfg.GetBool("DSi.DSP.HLE"));
    bool dsiSD = globalCfg.GetBool("DSi.SD.Enable");
    swDSiSDEnable->setChecked(dsiSD);
    dsiSDContainer->setVisible(dsiSD);
    txtDSiSDPath->setText(globalCfg.GetQString("DSi.SD.ImagePath"));
    cbxDSiSDSize->setCurrentIndex(qBound(0, globalCfg.GetInt("DSi.SD.ImageSize"), 5));
    swDSiSDReadOnly->setChecked(globalCfg.GetBool("DSi.SD.ReadOnly"));
    bool dsiFolder = globalCfg.GetBool("DSi.SD.FolderSync");
    swDSiSDFolder->setChecked(dsiFolder);
    dsiSDFolderContainer->setVisible(dsiFolder);
    txtDSiSDFolder->setText(globalCfg.GetQString("DSi.SD.FolderPath"));

    // DLDI options
    bool dldi = globalCfg.GetBool("DLDI.Enable");
    swDLDIEnable->setChecked(dldi);
    dldiContainer->setVisible(dldi);
    txtDLDISDPath->setText(globalCfg.GetQString("DLDI.ImagePath"));
    cbxDLDISize->setCurrentIndex(qBound(0, globalCfg.GetInt("DLDI.ImageSize"), 5));
    swDLDIReadOnly->setChecked(globalCfg.GetBool("DLDI.ReadOnly"));
    bool dldiFolder = globalCfg.GetBool("DLDI.FolderSync");
    swDLDIFolder->setChecked(dldiFolder);
    dldiFolderContainer->setVisible(dldiFolder);
    txtDLDIFolder->setText(globalCfg.GetQString("DLDI.FolderPath"));

    // DSi Cameras
    cbxCam0Type->setCurrentIndex(qBound(0, globalCfg.GetInt("DSi.Camera0.InputType"), 2));
    txtCam0ImagePath->setText(globalCfg.GetQString("DSi.Camera0.ImagePath"));
    QString dev0 = globalCfg.GetQString("DSi.Camera0.DeviceName");
    if (!dev0.isEmpty()) {
        int idx = cbxCam0Device->findData(dev0);
        if (idx < 0) idx = cbxCam0Device->findText(dev0);
        if (idx >= 0) cbxCam0Device->setCurrentIndex(idx);
    }
    swCam0XFlip->setChecked(globalCfg.GetBool("DSi.Camera0.XFlip"));

    cbxCam1Type->setCurrentIndex(qBound(0, globalCfg.GetInt("DSi.Camera1.InputType"), 2));
    txtCam1ImagePath->setText(globalCfg.GetQString("DSi.Camera1.ImagePath"));
    QString dev1 = globalCfg.GetQString("DSi.Camera1.DeviceName");
    if (!dev1.isEmpty()) {
        int idx = cbxCam1Device->findData(dev1);
        if (idx < 0) idx = cbxCam1Device->findText(dev1);
        if (idx >= 0) cbxCam1Device->setCurrentIndex(idx);
    }
    swCam1XFlip->setChecked(globalCfg.GetBool("DSi.Camera1.XFlip"));

    // RTC & Battery (System Page)
    qint64 rtcOffset = localCfg0.GetInt64("RTC.Offset");
    bool hasCustomRTC = (rtcOffset != 0);
    swCustomRTC->setChecked(hasCustomRTC);
    rtcContainer->setVisible(hasCustomRTC);
    dteCustomRTC->setDateTime(hasCustomRTC
        ? QDateTime::currentDateTime().addSecs(rtcOffset)
        : QDateTime::currentDateTime());

    bool dsBatOk = localCfg0.GetBool("DS.Battery.LevelOkay");
    cbxDSBattery->setCurrentIndex(dsBatOk ? 0 : 1);

    int dsiBat = localCfg0.GetInt("DSi.Battery.Level");
    int dsiIdx = cbxDSiBattery->findData(dsiBat);
    cbxDSiBattery->setCurrentIndex(dsiIdx >= 0 ? dsiIdx : 0);

    swDSiCharging->setChecked(localCfg0.GetBool("DSi.Battery.Charging"));

    // CPU
    swJitEnable->setChecked(globalCfg.GetBool("JIT.Enable"));
    spnJitBlockSize->setValue(qMax(1, globalCfg.GetInt("JIT.MaxBlockSize")));
    swJitBranchOpt->setChecked(globalCfg.GetBool("JIT.BranchOptimisations"));
    swJitLiteralOpt->setChecked(globalCfg.GetBool("JIT.LiteralOptimisations"));
    swJitFastMem->setChecked(globalCfg.GetBool("JIT.FastMemory"));

    // Graphics
    int rIdx = cbxRenderer3D->findData(globalCfg.GetInt("3D.Renderer"));
    cbxRenderer3D->setCurrentIndex(rIdx >= 0 ? rIdx : 1);
    int scale = globalCfg.GetInt("3D.GL.ScaleFactor");
    if (scale >= 1 && scale <= 8) {
        cbxScaleFactor->setCurrentIndex(scale - 1);
    }
    swVSync->setChecked(globalCfg.GetBool("Screen.VSync"));
    swBetterPolygons->setChecked(globalCfg.GetBool("3D.GL.BetterPolygons"));
    swHiresCoords->setChecked(globalCfg.GetBool("3D.GL.HiresCoordinates"));
    swSoftThreaded->setChecked(globalCfg.GetBool("3D.Soft.Threaded"));

    QString curShader = globalCfg.GetQString("WatermelonShader");
    if (curShader.contains(QStringLiteral("crt"), Qt::CaseInsensitive))
        cbxShader->setCurrentIndex(1);
    else if (curShader.contains(QStringLiteral("lcd"), Qt::CaseInsensitive))
        cbxShader->setCurrentIndex(2);
    else if (curShader.contains(QStringLiteral("scale"), Qt::CaseInsensitive))
        cbxShader->setCurrentIndex(3);
    else if (curShader.contains(QStringLiteral("xbrz"), Qt::CaseInsensitive))
        cbxShader->setCurrentIndex(4);
    else
        cbxShader->setCurrentIndex(0);

    // Display
    Config::Table winCfg = Config::GetLocalTable(0).GetTable("Window0");
    cbxWindowSize->setCurrentIndex(qMax(0, cbxWindowSize->findData(2)));
    cbxScreenRotation->setCurrentIndex(qMax(0, cbxScreenRotation->findData(winCfg.GetInt("ScreenRotation"))));
    spnScreenGap->setValue(qBound(0, winCfg.GetInt("ScreenGap"), 500));
    cbxScreenLayout->setCurrentIndex(qMax(0, cbxScreenLayout->findData(winCfg.GetInt("ScreenLayout"))));
    cbxScreenSizing->setCurrentIndex(qMax(0, cbxScreenSizing->findData(winCfg.GetInt("ScreenSizing"))));
    cbxScreenAspectTop->setCurrentIndex(qMax(0, cbxScreenAspectTop->findData(winCfg.GetInt("ScreenAspectTop"))));
    cbxScreenAspectBot->setCurrentIndex(qMax(0, cbxScreenAspectBot->findData(winCfg.GetInt("ScreenAspectBot"))));
    swScreenSwap->setChecked(winCfg.GetBool("ScreenSwap"));
    swIntegerScaling->setChecked(winCfg.GetBool("IntegerScaling"));
    swScreenFilter->setChecked(winCfg.GetBool("ScreenFilter"));

    // Audio
    int vol = globalCfg.GetInt("Audio.Volume");
    if (vol <= 0 && vol != 0) vol = 256;
    sldVolume->setValue(vol);
    int pct = qRound((vol / 256.0) * 100.0);
    lblVolumeVal->setText(tr("%1%").arg(pct));
    cbxInterpolation->setCurrentIndex(qBound(0, globalCfg.GetInt("Audio.Interpolation"), 3));
    cbxBitDepth->setCurrentIndex(qBound(0, globalCfg.GetInt("Audio.BitDepth"), 2));
    swDSiVolumeSync->setChecked(localCfg0.GetBool("Audio.DSiVolumeSync"));
    cbxMicInputType->setCurrentIndex(qBound(0, globalCfg.GetInt("Mic.InputType"), 3));
    QString micDev = globalCfg.GetQString("Mic.Device");
    if (!micDev.isEmpty()) {
        int idx = cbxMicDevice->findText(micDev);
        if (idx >= 0) cbxMicDevice->setCurrentIndex(idx);
    }
    txtMicWavPath->setText(globalCfg.GetQString("Mic.WavPath"));

    // Multiplayer & Network
    cbxMPAudioMode->setCurrentIndex(qBound(0, globalCfg.GetInt("MP.AudioMode"), 2));
    spnMPRecvTimeout->setValue(qBound(1, globalCfg.GetInt("MP.RecvTimeout"), 1000));

    cbxNetAdapter->clear();
    int selAdapter = 0;
    std::string currentDev = globalCfg.GetString("LAN.Device");
    for (int i = 0; i < (int)netAdapters.size(); i++)
    {
        cbxNetAdapter->addItem(QString::fromStdString(netAdapters[i].FriendlyName));
        if (!strncmp(netAdapters[i].DeviceName, currentDev.c_str(), 128))
            selAdapter = i;
    }
    if (!netAdapters.empty())
        cbxNetAdapter->setCurrentIndex(selAdapter);

    bool direct = globalCfg.GetBool("LAN.DirectMode");
    rbDirectMode->setChecked(direct && hasNetPcap);
    rbIndirectMode->setChecked(!direct || !hasNetPcap);
    rbDirectMode->setEnabled(hasNetPcap);
    updateNetAdapterControls();
    onNetAdapterIndexChanged(cbxNetAdapter->currentIndex());

    // RetroAchievements
    swRAEnabled->setChecked(globalCfg.GetBool("RA.Enabled"));
    swRAHardcore->setChecked(globalCfg.GetBool("RA.Hardcore"));
    swRARichPresence->setChecked(globalCfg.GetBool("RA.RichPresence"));
    swRASound->setChecked(globalCfg.GetBool("RA.SoundEnabled"));
    swRAUnofficial->setChecked(globalCfg.GetBool("RA.Unofficial"));
    swRAEncore->setChecked(globalCfg.GetBool("RA.Encore"));

    QString raUser = globalCfg.GetQString("RA.Username");
    QString raTok  = globalCfg.GetQString("RA.Token");
    txtRAUser->setText(raUser);
    txtRAToken->setText(raTok);
    if (!raUser.isEmpty()) {
        lblRAStatus->setText(tr("Sesión activa con el usuario: %1").arg(raUser));
    } else {
        lblRAStatus->setText(tr("No has iniciado sesión en RetroAchievements."));
        lblRAStatus->setStyleSheet(QStringLiteral("color: #a09ea8; font-size: 11px;"));
    }
}

void WatermelonSettingsDialog::saveSettings()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    Config::Table localCfg0 = Config::GetLocalTable(0);

    // General
    globalCfg.SetBool("PauseLostFocus", swPauseLostFocus->isChecked());
    globalCfg.SetBool("Mouse.Hide", swMouseHide->isChecked());
    globalCfg.SetInt("Mouse.HideSeconds", spnMouseHideSeconds->value());
    globalCfg.SetDouble("TargetFPS", spnTargetFPS->value());
    globalCfg.SetDouble("FastForwardFPS", spnFastForwardFPS->value());
    globalCfg.SetDouble("SlowmoFPS", spnSlowmoFPS->value());
    globalCfg.SetBool("Emu.DirectBoot", swDirectBoot->isChecked());

    const QString oldUILang = globalCfg.GetQString("UI.Language");
    const QString newUILang = cbxUILanguage->currentData().toString();
    globalCfg.SetQString("UI.Language", newUILang);

    QStringList dirList;
    for (int i = 0; i < listRomDirs->count(); ++i) {
        dirList.append(listRomDirs->item(i)->text());
    }
    globalCfg.SetQString("WatermelonRomDirs", dirList.join(QLatin1Char(';')));

    localCfg0.SetQString("SaveFilePath", txtSaveFilePath->text().trimmed());
    localCfg0.SetQString("SavestatePath", txtSavestatePath->text().trimmed());
    localCfg0.SetQString("CheatFilePath", txtCheatFilePath->text().trimmed());
    globalCfg.SetQString("WatermelonCheatDbPath", txtCheatDbPath->text().trimmed());
    globalCfg.SetBool("Savestate.RelocSRAM", swSavestateRelocSRAM->isChecked());

    // System
    globalCfg.SetInt("Emu.ConsoleType", cbxConsoleType->currentIndex());
    globalCfg.SetBool("Emu.ExternalBIOSEnable", swExternalBios->isChecked());

    globalCfg.SetQString("DS.BIOS9Path", txtBios9->text().trimmed());
    globalCfg.SetQString("DS.BIOS7Path", txtBios7->text().trimmed());
    globalCfg.SetQString("DS.FirmwarePath", txtFirmware->text().trimmed());
    globalCfg.SetQString("DSi.BIOS9Path", txtDSiBios9->text().trimmed());
    globalCfg.SetQString("DSi.BIOS7Path", txtDSiBios7->text().trimmed());
    globalCfg.SetQString("DSi.FirmwarePath", txtDSiFirmware->text().trimmed());
    globalCfg.SetQString("DSi.NANDPath", txtDSiNAND->text().trimmed());

    Config::Table firmCfg = globalCfg.GetTable("Firmware");
    firmCfg.SetInt("Language", cbxLanguage->currentIndex());
    firmCfg.SetQString("Username", txtUsername->text());
    firmCfg.SetInt("FavouriteColour", cbxFavColour->currentIndex());
    firmCfg.SetInt("BirthdayDay", spnBirthDay->value());
    firmCfg.SetInt("BirthdayMonth", cbxBirthMonth->currentIndex() + 1);
    firmCfg.SetQString("Message", txtMessage->text());
    firmCfg.SetBool("OverrideSettings", true);

    // DSi options
    globalCfg.SetBool("DSi.FullBIOSBoot", swDSiFullBIOSBoot->isChecked());
    globalCfg.SetBool("DSi.DSP.HLE", swDSPHLE->isChecked());
    globalCfg.SetBool("DSi.SD.Enable", swDSiSDEnable->isChecked());
    globalCfg.SetQString("DSi.SD.ImagePath", txtDSiSDPath->text().trimmed());
    globalCfg.SetInt("DSi.SD.ImageSize", cbxDSiSDSize->currentIndex());
    globalCfg.SetBool("DSi.SD.ReadOnly", swDSiSDReadOnly->isChecked());
    globalCfg.SetBool("DSi.SD.FolderSync", swDSiSDFolder->isChecked());
    globalCfg.SetQString("DSi.SD.FolderPath", txtDSiSDFolder->text().trimmed());

    // DLDI options
    globalCfg.SetBool("DLDI.Enable", swDLDIEnable->isChecked());
    globalCfg.SetQString("DLDI.ImagePath", txtDLDISDPath->text().trimmed());
    globalCfg.SetInt("DLDI.ImageSize", cbxDLDISize->currentIndex());
    globalCfg.SetBool("DLDI.ReadOnly", swDLDIReadOnly->isChecked());
    globalCfg.SetBool("DLDI.FolderSync", swDLDIFolder->isChecked());
    globalCfg.SetQString("DLDI.FolderPath", txtDLDIFolder->text().trimmed());

    // DSi Cameras
    globalCfg.SetInt("DSi.Camera0.InputType", cbxCam0Type->currentIndex());
    globalCfg.SetQString("DSi.Camera0.ImagePath", txtCam0ImagePath->text().trimmed());
    globalCfg.SetQString("DSi.Camera0.DeviceName", cbxCam0Device->count() > 0 ? cbxCam0Device->currentData().toString() : QString());
    globalCfg.SetBool("DSi.Camera0.XFlip", swCam0XFlip->isChecked());

    globalCfg.SetInt("DSi.Camera1.InputType", cbxCam1Type->currentIndex());
    globalCfg.SetQString("DSi.Camera1.ImagePath", txtCam1ImagePath->text().trimmed());
    globalCfg.SetQString("DSi.Camera1.DeviceName", cbxCam1Device->count() > 0 ? cbxCam1Device->currentData().toString() : QString());
    globalCfg.SetBool("DSi.Camera1.XFlip", swCam1XFlip->isChecked());

    // RTC & Battery (System Page)
    if (swCustomRTC->isChecked())
    {
        qint64 offset = QDateTime::currentDateTime().secsTo(dteCustomRTC->dateTime());
        localCfg0.SetInt64("RTC.Offset", offset);
    }
    else
    {
        localCfg0.SetInt64("RTC.Offset", 0);
    }

    localCfg0.SetBool("DS.Battery.LevelOkay", cbxDSBattery->currentData().toBool());
    localCfg0.SetInt("DSi.Battery.Level", cbxDSiBattery->currentData().toInt());
    localCfg0.SetBool("DSi.Battery.Charging", swDSiCharging->isChecked());

    // CPU
    globalCfg.SetBool("JIT.Enable", swJitEnable->isChecked());
    globalCfg.SetInt("JIT.MaxBlockSize", spnJitBlockSize->value());
    globalCfg.SetBool("JIT.BranchOptimisations", swJitBranchOpt->isChecked());
    globalCfg.SetBool("JIT.LiteralOptimisations", swJitLiteralOpt->isChecked());
    globalCfg.SetBool("JIT.FastMemory", swJitFastMem->isChecked());

    // Graphics
    globalCfg.SetInt("3D.Renderer", cbxRenderer3D->currentData().toInt());
    globalCfg.SetInt("3D.GL.ScaleFactor", cbxScaleFactor->currentIndex() + 1);
    globalCfg.SetBool("Screen.VSync", swVSync->isChecked());
    globalCfg.SetBool("3D.GL.BetterPolygons", swBetterPolygons->isChecked());
    globalCfg.SetBool("3D.GL.HiresCoordinates", swHiresCoords->isChecked());
    globalCfg.SetBool("3D.Soft.Threaded", swSoftThreaded->isChecked());

    QString shaderName;
    switch (cbxShader->currentIndex()) {
        case 1: shaderName = QStringLiteral("crt-lottes"); break;
        case 2: shaderName = QStringLiteral("lcd3x"); break;
        case 3: shaderName = QStringLiteral("scalefx"); break;
        case 4: shaderName = QStringLiteral("xbrz"); break;
        default: shaderName = QString(); break;
    }
    globalCfg.SetQString("WatermelonShader", shaderName);

    // Display
    Config::Table winCfg = Config::GetLocalTable(0).GetTable("Window0");
    winCfg.SetInt("ScreenRotation", cbxScreenRotation->currentData().toInt());
    winCfg.SetInt("ScreenGap", spnScreenGap->value());
    winCfg.SetInt("ScreenLayout", cbxScreenLayout->currentData().toInt());
    winCfg.SetInt("ScreenSizing", cbxScreenSizing->currentData().toInt());
    winCfg.SetInt("ScreenAspectTop", cbxScreenAspectTop->currentData().toInt());
    winCfg.SetInt("ScreenAspectBot", cbxScreenAspectBot->currentData().toInt());
    winCfg.SetBool("ScreenSwap", swScreenSwap->isChecked());
    winCfg.SetBool("IntegerScaling", swIntegerScaling->isChecked());
    winCfg.SetBool("ScreenFilter", swScreenFilter->isChecked());

    // Audio
    globalCfg.SetInt("Audio.Volume", sldVolume->value());
    globalCfg.SetInt("Audio.Interpolation", cbxInterpolation->currentIndex());
    globalCfg.SetInt("Audio.BitDepth", cbxBitDepth->currentIndex());
    localCfg0.SetBool("Audio.DSiVolumeSync", swDSiVolumeSync->isChecked());
    globalCfg.SetInt("Mic.InputType", cbxMicInputType->currentIndex());
    globalCfg.SetQString("Mic.Device", cbxMicDevice->currentText());
    globalCfg.SetQString("Mic.WavPath", txtMicWavPath->text().trimmed());

    // Controls
    if (inputConfigWidget) {
        inputConfigWidget->accept();
    }

    // Multiplayer & Network
    globalCfg.SetInt("MP.AudioMode", cbxMPAudioMode->currentIndex());
    globalCfg.SetInt("MP.RecvTimeout", spnMPRecvTimeout->value());
    globalCfg.SetBool("LAN.DirectMode", rbDirectMode->isChecked());
    int selAdapter = cbxNetAdapter->currentIndex();
    if (selAdapter < 0 || selAdapter >= (int)netAdapters.size()) selAdapter = 0;
    if (netAdapters.empty())
    {
        globalCfg.SetString("LAN.Device", "");
    }
    else
    {
        globalCfg.SetString("LAN.Device", netAdapters[selAdapter].DeviceName);
    }

    // RetroAchievements
    globalCfg.SetBool("RA.Enabled", swRAEnabled->isChecked());
    globalCfg.SetBool("RA.Hardcore", swRAHardcore->isChecked());
    globalCfg.SetBool("RA.RichPresence", swRARichPresence->isChecked());
    globalCfg.SetBool("RA.SoundEnabled", swRASound->isChecked());
    globalCfg.SetBool("RA.Unofficial", swRAUnofficial->isChecked());
    globalCfg.SetBool("RA.Encore", swRAEncore->isChecked());
    globalCfg.SetQString("RA.Username", txtRAUser->text().trimmed());
    globalCfg.SetQString("RA.Token", txtRAToken->text().trimmed());

    if (oldUILang != newUILang)
    {
        QMessageBox::information(this, tr("Idioma de la Interfaz"),
                                 tr("El idioma de la interfaz se aplicará al reiniciar el emulador."));
    }

    Config::Save();

    // Reapply runtime camera configurations if instances exist
    for (int i = 0; i < 2; i++)
    {
        if (camManager[i])
        {
            bool wasStarted = camManager[i]->isStarted();
            if (wasStarted) camManager[i]->stop();
            camManager[i]->deInit();
            camManager[i]->init();
            camManager[i]->setXFlip(i == 0 ? swCam0XFlip->isChecked() : swCam1XFlip->isChecked());
            if (wasStarted) camManager[i]->start();
        }
    }

    if (auto* mw = findMainWindow(this))
    {
        mw->applyScreenConfig();
        mw->applyScreenSizeFactor(cbxWindowSize->currentData().toInt());
        mw->applyMPSettings();
        mw->applyInterfaceSettings();
        mw->applyAudioSettings();
    }
    else
    {
        melonDS::MPInterface::Get().SetRecvTimeout(globalCfg.GetInt("MP.RecvTimeout"));
    }

    NetInit();

    emit settingsSaved();
}

void WatermelonSettingsDialog::onAddRomDirClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Seleccionar Carpeta de ROMs"));
    if (!dir.isEmpty()) {
        for (int i = 0; i < listRomDirs->count(); ++i) {
            if (listRomDirs->item(i)->text() == dir) return;
        }
        listRomDirs->addItem(dir);
    }
}

void WatermelonSettingsDialog::onRemoveRomDirClicked()
{
    auto* cur = listRomDirs->currentItem();
    if (cur) {
        delete listRomDirs->takeItem(listRomDirs->row(cur));
    }
}

void WatermelonSettingsDialog::onResetDefaultsClicked()
{
    auto res = QMessageBox::question(
        this,
        tr("Restablecer Ajustes"),
        tr("¿Estás seguro de que deseas restablecer todos los ajustes a sus valores por defecto?"),
        QMessageBox::Yes | QMessageBox::No
    );
    if (res == QMessageBox::Yes) {
        swPauseLostFocus->setChecked(true);
        swMouseHide->setChecked(true);
        spnMouseHideSeconds->setValue(5);
        spnTargetFPS->setValue(60.0);
        spnFastForwardFPS->setValue(120.0);
        spnSlowmoFPS->setValue(30.0);
        swDirectBoot->setChecked(true);
        swSavestateRelocSRAM->setChecked(false);
        cbxConsoleType->setCurrentIndex(0);
        swExternalBios->setChecked(false);
        biosFilesContainer->setVisible(false);
        swJitEnable->setChecked(true);
        spnJitBlockSize->setValue(32);
        swJitBranchOpt->setChecked(true);
        swJitLiteralOpt->setChecked(true);
        swJitFastMem->setChecked(true);
        cbxRenderer3D->setCurrentIndex(1); // OpenGL
        cbxScaleFactor->setCurrentIndex(1); // 2x
        cbxShader->setCurrentIndex(0);
        swVSync->setChecked(false);
        swBetterPolygons->setChecked(false);
        swHiresCoords->setChecked(false);
        swSoftThreaded->setChecked(true);
        sldVolume->setValue(256);
        cbxInterpolation->setCurrentIndex(1);
        cbxBitDepth->setCurrentIndex(0);
        swDSiVolumeSync->setChecked(false);
        cbxMicInputType->setCurrentIndex(1);
        txtMicWavPath->clear();
        micDeviceContainer->setVisible(true);
        micWavContainer->setVisible(false);
        cbxWindowSize->setCurrentIndex(1); // 2x
        cbxScreenRotation->setCurrentIndex(0); // 0°
        spnScreenGap->setValue(0);
        cbxScreenLayout->setCurrentIndex(0); // Natural
        cbxScreenSizing->setCurrentIndex(0); // Even
        cbxScreenAspectTop->setCurrentIndex(0); // 4:3
        cbxScreenAspectBot->setCurrentIndex(0); // 4:3
        swScreenSwap->setChecked(false);
        swIntegerScaling->setChecked(false);
        swScreenFilter->setChecked(true);
        swCustomRTC->setChecked(false);
        dteCustomRTC->setDateTime(QDateTime::currentDateTime());
        rtcContainer->setVisible(false);
        cbxDSBattery->setCurrentIndex(0);
        cbxDSiBattery->setCurrentIndex(0);
        swDSiCharging->setChecked(true);
        cbxCam0Type->setCurrentIndex(0);
        txtCam0ImagePath->clear();
        swCam0XFlip->setChecked(false);
        cam0ImageContainer->setVisible(false);
        cam0DeviceContainer->setVisible(false);
        cbxCam1Type->setCurrentIndex(0);
        txtCam1ImagePath->clear();
        swCam1XFlip->setChecked(false);
        cam1ImageContainer->setVisible(false);
        cam1DeviceContainer->setVisible(false);
        swRAHardcore->setChecked(true);
        swRARichPresence->setChecked(true);
        swRASound->setChecked(true);
        swRAUnofficial->setChecked(false);
        swRAEncore->setChecked(false);
        cbxMPAudioMode->setCurrentIndex(1);
        spnMPRecvTimeout->setValue(25);
        rbIndirectMode->setChecked(true);
        rbDirectMode->setChecked(false);
        updateNetAdapterControls();
        swDSiFullBIOSBoot->setChecked(false);
        swDSPHLE->setChecked(true);
        swDSiSDEnable->setChecked(false);
        dsiSDContainer->setVisible(false);
        txtDSiSDPath->setText(QStringLiteral("dsisd.bin"));
        cbxDSiSDSize->setCurrentIndex(0);
        swDSiSDReadOnly->setChecked(false);
        swDSiSDFolder->setChecked(false);
        dsiSDFolderContainer->setVisible(false);
        txtDSiSDFolder->clear();
        swDLDIEnable->setChecked(false);
        dldiContainer->setVisible(false);
        txtDLDISDPath->setText(QStringLiteral("dldi.bin"));
        cbxDLDISize->setCurrentIndex(0);
        swDLDIReadOnly->setChecked(false);
        swDLDIFolder->setChecked(false);
        dldiFolderContainer->setVisible(false);
        txtDLDIFolder->clear();
        txtSaveFilePath->clear();
        txtSavestatePath->clear();
        txtCheatFilePath->clear();
        txtCheatDbPath->clear();
    }
}

void WatermelonSettingsDialog::onApplyClicked()
{
    saveSettings();
}

void WatermelonSettingsDialog::onOkClicked()
{
    saveSettings();
    accept();
}
