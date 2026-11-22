#include "WatermelonSettingsDialog.h"
#include "WatermelonShaderDialog.h"
#include "WatermelonRADialog.h"
#include "InputConfig/InputConfigDialog.h"
#include "Window.h"
#include "Config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
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
    setWindowTitle(QStringLiteral("Configuración de WatermelonDS"));
    resize(940, 680);
    setMinimumSize(860, 600);
    setModal(true);

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

    auto* itemGeneral  = new QListWidgetItem(QStringLiteral("🛠  General"), sidebarList);
    auto* itemSystem   = new QListWidgetItem(QStringLiteral("🪪  Sistema"), sidebarList);
    auto* itemCpu      = new QListWidgetItem(QStringLiteral("⚙  CPU"), sidebarList);
    auto* itemGraphics = new QListWidgetItem(QStringLiteral("🖥  Gráficos"), sidebarList);
    auto* itemAudio    = new QListWidgetItem(QStringLiteral("🔊  Audio"), sidebarList);
    auto* itemControls = new QListWidgetItem(QStringLiteral("🎮  Controles"), sidebarList);
    auto* itemRA       = new QListWidgetItem(QStringLiteral("🏆  Logros"), sidebarList);

    Q_UNUSED(itemGeneral);
    Q_UNUSED(itemSystem);
    Q_UNUSED(itemCpu);
    Q_UNUSED(itemGraphics);
    Q_UNUSED(itemAudio);
    Q_UNUSED(itemControls);
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
    pageStack->addWidget(createAudioPage());
    pageStack->addWidget(createControlsPage());
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

    auto* btnReset = new QPushButton(QStringLiteral("Reiniciar todos los ajustes"), bottomBar);
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

    auto* lblNote = new QLabel(QStringLiteral("Algunos ajustes solo están disponibles cuando no se estén ejecutando juegos."), bottomBar);
    lblNote->setStyleSheet(QStringLiteral("color: #63616e; font-size: 11px; margin-left: 6px; border: none; background: transparent;"));
    bottomLayout->addWidget(lblNote);

    bottomLayout->addStretch();

    auto* btnOk = new QPushButton(QStringLiteral("OK"), bottomBar);
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

    auto* btnApply = new QPushButton(QStringLiteral("Apply"), bottomBar);
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

    auto* btnCancel = new QPushButton(QStringLiteral("Cancel"), bottomBar);
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
        "QLineEdit, QComboBox, QSpinBox {"
        "  background-color: #141318; border: 1px solid #2d2c38; border-radius: 6px;"
        "  color: #f6f5f3; padding: 0px 8px; height: 26px; min-height: 26px; max-height: 26px;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus {"
        "  border-color: #ff5477; background-color: #181720;"
        "}"
        "QComboBox { padding-right: 22px; }"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding; subcontrol-position: top right; width: 22px; border: none; background: transparent;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/arrow-down); width: 10px; height: 10px;"
        "}"
        "QSpinBox { padding-right: 20px; }"
        "QSpinBox::up-button {"
        "  subcontrol-origin: border; subcontrol-position: top right; width: 18px;"
        "  border-left: 1px solid #2d2c38; border-bottom: 1px solid #2d2c38; border-top-right-radius: 5px;"
        "  background: #1c1b22;"
        "}"
        "QSpinBox::up-button:hover { background: #282732; }"
        "QSpinBox::up-arrow {"
        "  image: url(:/arrow-up); width: 8px; height: 8px;"
        "}"
        "QSpinBox::down-button {"
        "  subcontrol-origin: border; subcontrol-position: bottom right; width: 18px;"
        "  border-left: 1px solid #2d2c38; border-bottom-right-radius: 5px;"
        "  background: #1c1b22;"
        "}"
        "QSpinBox::down-button:hover { background: #282732; }"
        "QSpinBox::down-arrow {"
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

    auto* btnBrowse = new QPushButton(QStringLiteral("Examinar..."), parent);
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

    lay->addWidget(createSectionTitle(QStringLiteral("Comportamiento del Emulador"), content));

    auto* cardGen = createCard(content);
    auto* cardGenLay = new QVBoxLayout(cardGen);
    cardGenLay->setContentsMargins(14, 12, 14, 12);
    cardGenLay->setSpacing(12);

    swPauseLostFocus = new WatermelonSwitch(cardGen);
    cardGenLay->addLayout(createSwitchRow(swPauseLostFocus, QStringLiteral("Pausar cuando la ventana no esté activa"), QStringLiteral("Detiene la emulación temporalmente si cambias a otra aplicación."), cardGen));

    swMouseHide = new WatermelonSwitch(cardGen);
    cardGenLay->addLayout(createSwitchRow(swMouseHide, QStringLiteral("Ocultar el cursor por inactividad"), QStringLiteral("Oculta automáticamente la flecha del ratón mientras juegas."), cardGen));

    swDirectBoot = new WatermelonSwitch(cardGen);
    cardGenLay->addLayout(createSwitchRow(swDirectBoot, QStringLiteral("Ejecución directa (Direct Boot)"), QStringLiteral("Inicia los juegos inmediatamente saltando el menú de inicio de la consola."), cardGen));

    lay->addWidget(cardGen);

    // Contenido externo / Carpetas de ROMs
    lay->addWidget(createSectionTitle(QStringLiteral("Contenido externo (Carpetas de ROMs)"), content));

    auto* cardDirs = createCard(content);
    auto* cardDirsLay = new QVBoxLayout(cardDirs);
    cardDirsLay->setContentsMargins(14, 14, 14, 14);
    cardDirsLay->setSpacing(10);

    auto* descDirs = new QLabel(QStringLiteral("Añade directorios para buscar automáticamente juegos de Nintendo DS y carátulas:"), cardDirs);
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

    auto* btnAddDir = new QPushButton(QStringLiteral("Añadir directorio"), cardDirs);
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

    auto* btnRemDir = new QPushButton(QStringLiteral("Eliminar seleccionado"), cardDirs);
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

    lay->addWidget(createSectionTitle(QStringLiteral("Consola y Firmware"), content));

    auto* cardSys = createCard(content);
    auto* grid = new QGridLayout(cardSys);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(QStringLiteral("Tipo de Consola:"), cardSys), 0, 0);
    cbxConsoleType = new QComboBox(cardSys);
    cbxConsoleType->addItems({QStringLiteral("Nintendo DS (Original)"), QStringLiteral("Nintendo DSi")});
    setupInput(cbxConsoleType);
    grid->addWidget(cbxConsoleType, 0, 1);

    grid->addWidget(new QLabel(QStringLiteral("Idioma del Sistema:"), cardSys), 1, 0);
    cbxLanguage = new QComboBox(cardSys);
    cbxLanguage->addItems({QStringLiteral("Japonés"), QStringLiteral("Inglés"), QStringLiteral("Francés"), QStringLiteral("Alemán"), QStringLiteral("Italiano"), QStringLiteral("Español")});
    setupInput(cbxLanguage);
    grid->addWidget(cbxLanguage, 1, 1);

    grid->addWidget(new QLabel(QStringLiteral("Nombre de Usuario:"), cardSys), 2, 0);
    txtUsername = new QLineEdit(cardSys);
    setupInput(txtUsername);
    grid->addWidget(txtUsername, 2, 1);

    // Color Favorito with Color Swatch Icons
    grid->addWidget(new QLabel(QStringLiteral("Color Favorito:"), cardSys), 3, 0);
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
    grid->addWidget(new QLabel(QStringLiteral("Cumpleaños:"), cardSys), 4, 0);
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

    grid->addWidget(new QLabel(QStringLiteral("Mensaje Personal:"), cardSys), 5, 0);
    txtMessage = new QLineEdit(cardSys);
    setupInput(txtMessage);
    grid->addWidget(txtMessage, 5, 1);

    lay->addWidget(cardSys);

    // BIOS Files Section
    lay->addWidget(createSectionTitle(QStringLiteral("Archivos de BIOS y Firmware"), content));
    auto* cardBios = createCard(content);
    auto* biosLay = new QVBoxLayout(cardBios);
    biosLay->setContentsMargins(14, 12, 14, 12);
    biosLay->setSpacing(12);

    swExternalBios = new WatermelonSwitch(cardBios);
    biosLay->addLayout(createSwitchRow(swExternalBios, QStringLiteral("Usar BIOS/Firmware externo personalizado"), QStringLiteral("Permite seleccionar archivos binarios originales extraídos de tu Nintendo DS."), cardBios));

    // Expandable container for BIOS paths
    biosFilesContainer = new QWidget(cardBios);
    auto* biosFilesLay = new QVBoxLayout(biosFilesContainer);
    biosFilesLay->setContentsMargins(6, 6, 6, 6);
    biosFilesLay->setSpacing(10);

    const QString binFilter = QStringLiteral("Archivos binarios (*.bin *.rom);;Todos los archivos (*.*)");

    biosFilesLay->addLayout(createPathRow(QStringLiteral("BIOS ARM9 (DS):"), txtBios9, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(QStringLiteral("BIOS ARM7 (DS):"), txtBios7, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(QStringLiteral("Firmware (DS):"), txtFirmware, binFilter, biosFilesContainer));

    // DSi paths
    auto* lblDSiSep = new QLabel(QStringLiteral("<b>Archivos adicionales Nintendo DSi:</b>"), biosFilesContainer);
    lblDSiSep->setStyleSheet(QStringLiteral("color: #a09ea8; margin-top: 6px;"));
    biosFilesLay->addWidget(lblDSiSep);

    biosFilesLay->addLayout(createPathRow(QStringLiteral("BIOS ARM9 (DSi):"), txtDSiBios9, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(QStringLiteral("BIOS ARM7 (DSi):"), txtDSiBios7, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(QStringLiteral("Firmware (DSi):"), txtDSiFirmware, binFilter, biosFilesContainer));
    biosFilesLay->addLayout(createPathRow(QStringLiteral("NAND (DSi):"), txtDSiNAND, binFilter, biosFilesContainer));

    biosLay->addWidget(biosFilesContainer);

    connect(swExternalBios, &WatermelonSwitch::toggled, biosFilesContainer, &QWidget::setVisible);

    lay->addWidget(cardBios);

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

    lay->addWidget(createSectionTitle(QStringLiteral("Recompilador JIT (Just-In-Time)"), content));

    auto* cardJit = createCard(content);
    auto* jitLay = new QVBoxLayout(cardJit);
    jitLay->setContentsMargins(14, 12, 14, 12);
    jitLay->setSpacing(12);

    swJitEnable = new WatermelonSwitch(cardJit);
    jitLay->addLayout(createSwitchRow(swJitEnable, QStringLiteral("Habilitar Recompilador JIT"), QStringLiteral("Aumenta drásticamente el rendimiento compilando código ARM al vuelo."), cardJit));

    auto* blockRow = new QHBoxLayout();
    blockRow->setContentsMargins(6, 4, 6, 4);
    auto* lblBlock = new QLabel(QStringLiteral("Tamaño Máximo de Bloque JIT:"), cardJit);
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
    jitLay->addLayout(createSwitchRow(swJitBranchOpt, QStringLiteral("Optimizaciones de saltos (Branch Optimisations)"), QStringLiteral("Optimiza saltos condicionales en el código emulado."), cardJit));

    swJitLiteralOpt = new WatermelonSwitch(cardJit);
    jitLay->addLayout(createSwitchRow(swJitLiteralOpt, QStringLiteral("Optimizaciones de literales (Literal Optimisations)"), QStringLiteral("Optimiza constantes y literales en memoria."), cardJit));

    swJitFastMem = new WatermelonSwitch(cardJit);
    jitLay->addLayout(createSwitchRow(swJitFastMem, QStringLiteral("Memoria Rápida (Fast Memory)"), QStringLiteral("Acelera los accesos a la memoria principal de la consola."), cardJit));

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

    lay->addWidget(createSectionTitle(QStringLiteral("Motor de Renderizado 3D"), content));

    auto* cardGfx = createCard(content);
    auto* grid = new QGridLayout(cardGfx);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(QStringLiteral("Renderizador 3D:"), cardGfx), 0, 0);
    cbxRenderer3D = new QComboBox(cardGfx);
    cbxRenderer3D->addItems({QStringLiteral("Software (Renderizado por CPU)"), QStringLiteral("OpenGL"), QStringLiteral("Vulkan (Experimental)")});
    setupInput(cbxRenderer3D);
    grid->addWidget(cbxRenderer3D, 0, 1);

    grid->addWidget(new QLabel(QStringLiteral("Multiplicador de Resolución:"), cardGfx), 1, 0);
    cbxScaleFactor = new QComboBox(cardGfx);
    cbxScaleFactor->addItems({
        QStringLiteral("1x Nativo (256x192)"),
        QStringLiteral("2x (512x384)"),
        QStringLiteral("3x (768x576)"),
        QStringLiteral("4x HD (1024x768)"),
        QStringLiteral("5x (1280x960)"),
        QStringLiteral("6x (1536x1152)"),
        QStringLiteral("8x UHD (2048x1536)")
    });
    setupInput(cbxScaleFactor);
    grid->addWidget(cbxScaleFactor, 1, 1);

    grid->addWidget(new QLabel(QStringLiteral("Shader de Postprocesado:"), cardGfx), 2, 0);
    cbxShader = new QComboBox(cardGfx);
    cbxShader->addItems({
        QStringLiteral("Ninguno (Directo)"),
        QStringLiteral("CRT-Lottes (Efecto Retro)"),
        QStringLiteral("LCD3x (Rejilla Nintendo DS)"),
        QStringLiteral("ScaleFx (Suavizado HD)"),
        QStringLiteral("xBRZ (Filtro Pixel Art)")
    });
    setupInput(cbxShader);
    grid->addWidget(cbxShader, 2, 1);

    lay->addWidget(cardGfx);

    lay->addWidget(createSectionTitle(QStringLiteral("Opciones de Pantalla y Polígonos"), content));

    auto* cardOpts = createCard(content);
    auto* optsLay = new QVBoxLayout(cardOpts);
    optsLay->setContentsMargins(14, 12, 14, 12);
    optsLay->setSpacing(12);

    swVSync = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swVSync, QStringLiteral("Sincronización Vertical (VSync)"), QStringLiteral("Evita el desgarro de pantalla (tearing) sincronizando con la tasa de refresco del monitor."), cardOpts));

    swBetterPolygons = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swBetterPolygons, QStringLiteral("Mejorar Polígonos (Better Polygons)"), QStringLiteral("Reduce artefactos de costuras y deformaciones poligonales en 3D."), cardOpts));

    swHiresCoords = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swHiresCoords, QStringLiteral("Coordenadas de Alta Resolución"), QStringLiteral("Permite que los vértices utilicen precisión subpíxel al aumentar la resolución."), cardOpts));

    swSoftThreaded = new WatermelonSwitch(cardOpts);
    optsLay->addLayout(createSwitchRow(swSoftThreaded, QStringLiteral("Renderizado por Software Multihilo"), QStringLiteral("Utiliza múltiples núcleos de CPU al usar el motor por Software."), cardOpts));

    lay->addWidget(cardOpts);
    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 5: Audio
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

    lay->addWidget(createSectionTitle(QStringLiteral("Salida de Sonido"), content));

    auto* cardAud = createCard(content);
    auto* grid = new QGridLayout(cardAud);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(QStringLiteral("Volumen Maestro:"), cardAud), 0, 0);
    auto* volLay = new QHBoxLayout();
    sldVolume = new QSlider(Qt::Horizontal, cardAud);
    sldVolume->setRange(0, 256);
    sldVolume->setValue(256);
    volLay->addWidget(sldVolume);

    lblVolumeVal = new QLabel(QStringLiteral("100%"), cardAud);
    lblVolumeVal->setFixedWidth(46);
    lblVolumeVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    volLay->addWidget(lblVolumeVal);
    grid->addLayout(volLay, 0, 1);

    connect(sldVolume, &QSlider::valueChanged, this, [this](int v) {
        int pct = qRound((v / 256.0) * 100.0);
        lblVolumeVal->setText(QStringLiteral("%1%").arg(pct));
    });

    grid->addWidget(new QLabel(QStringLiteral("Interpolación de Audio:"), cardAud), 1, 0);
    cbxInterpolation = new QComboBox(cardAud);
    cbxInterpolation->addItems({
        QStringLiteral("Ninguna (Sonido original DS)"),
        QStringLiteral("Lineal (Suavizado rápido)"),
        QStringLiteral("Coseno"),
        QStringLiteral("Cúbica (Mayor calidad)")
    });
    setupInput(cbxInterpolation);
    grid->addWidget(cbxInterpolation, 1, 1);

    grid->addWidget(new QLabel(QStringLiteral("Profundidad de Bits:"), cardAud), 2, 0);
    cbxBitDepth = new QComboBox(cardAud);
    cbxBitDepth->addItems({QStringLiteral("16 bits (Estándar)")});
    setupInput(cbxBitDepth);
    grid->addWidget(cbxBitDepth, 2, 1);

    lay->addWidget(cardAud);
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
        auto* lblErr = new QLabel(QStringLiteral("Abre el emulador con la ventana principal para configurar los controles."), cardFallback);
        fbLay->addWidget(lblErr);
        lay->addWidget(cardFallback);
    }

    lay->addStretch();
    scroll->setWidget(content);
    return scroll;
}

// ───────────────────────────────────────────────────────────────────────────
//  Page 7: RetroAchievements (Full Options!)
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

    lay->addWidget(createSectionTitle(QStringLiteral("RetroAchievements - Opciones de Juego"), content));

    auto* cardRA = createCard(content);
    auto* raLay = new QVBoxLayout(cardRA);
    raLay->setContentsMargins(14, 12, 14, 12);
    raLay->setSpacing(12);

    swRAEnabled = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRAEnabled, QStringLiteral("Habilitar RetroAchievements"), QStringLiteral("Desbloquea logros mientras juegas y sincroniza tu perfil con retroachievements.org."), cardRA));

    swRAHardcore = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRAHardcore, QStringLiteral("Modo Hardcore"), QStringLiteral("Deshabilita trucos y guardados rápidos (Savestates) para ganar el doble de puntuación oficial."), cardRA));

    swRARichPresence = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRARichPresence, QStringLiteral("Presencia Enriquecida (Rich Presence)"), QStringLiteral("Muestra en tiempo real en tu perfil qué nivel, zona o capítulo estás jugando."), cardRA));

    swRASound = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRASound, QStringLiteral("Efectos de Sonido"), QStringLiteral("Reproduce el sonido clásico de logro desbloqueado al conseguir una insignia."), cardRA));

    swRAUnofficial = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRAUnofficial, QStringLiteral("Logros No Oficiales (Unofficial)"), QStringLiteral("Permite cargar conjuntos de logros en fase de pruebas o desarrollo."), cardRA));

    swRAEncore = new WatermelonSwitch(cardRA);
    raLay->addLayout(createSwitchRow(swRAEncore, QStringLiteral("Modo Encore"), QStringLiteral("Permite volver a desbloquear logros que ya habías completado anteriormente para practicar."), cardRA));

    lay->addWidget(cardRA);

    lay->addWidget(createSectionTitle(QStringLiteral("Cuenta y Credenciales"), content));

    auto* cardAcc = createCard(content);
    auto* grid = new QGridLayout(cardAcc);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(QStringLiteral("Usuario:"), cardAcc), 0, 0);
    txtRAUser = new QLineEdit(cardAcc);
    setupInput(txtRAUser);
    grid->addWidget(txtRAUser, 0, 1);

    grid->addWidget(new QLabel(QStringLiteral("Token de Conexión:"), cardAcc), 1, 0);
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

    // General
    swPauseLostFocus->setChecked(globalCfg.GetBool("PauseLostFocus"));
    swMouseHide->setChecked(globalCfg.GetBool("Mouse.Hide"));
    swDirectBoot->setChecked(globalCfg.GetBool("Emu.DirectBoot"));

    listRomDirs->clear();
    QString dirsStr = globalCfg.GetQString("WatermelonRomDirs");
    if (!dirsStr.isEmpty()) {
        const QStringList dirs = dirsStr.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        for (const QString& d : dirs) {
            listRomDirs->addItem(d.trimmed());
        }
    }

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

    // CPU
    swJitEnable->setChecked(globalCfg.GetBool("JIT.Enable"));
    spnJitBlockSize->setValue(qMax(1, globalCfg.GetInt("JIT.MaxBlockSize")));
    swJitBranchOpt->setChecked(globalCfg.GetBool("JIT.BranchOptimisations"));
    swJitLiteralOpt->setChecked(globalCfg.GetBool("JIT.LiteralOptimisations"));
    swJitFastMem->setChecked(globalCfg.GetBool("JIT.FastMemory"));

    // Graphics
    cbxRenderer3D->setCurrentIndex(qBound(0, globalCfg.GetInt("3D.Renderer"), 2));
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

    // Audio
    int vol = globalCfg.GetInt("Audio.Volume");
    if (vol <= 0 && vol != 0) vol = 256;
    sldVolume->setValue(vol);
    int pct = qRound((vol / 256.0) * 100.0);
    lblVolumeVal->setText(QStringLiteral("%1%").arg(pct));
    cbxInterpolation->setCurrentIndex(qBound(0, globalCfg.GetInt("Audio.Interpolation"), 3));

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
        lblRAStatus->setText(QStringLiteral("Sesión activa con el usuario: %1").arg(raUser));
    } else {
        lblRAStatus->setText(QStringLiteral("No has iniciado sesión en RetroAchievements."));
        lblRAStatus->setStyleSheet(QStringLiteral("color: #a09ea8; font-size: 11px;"));
    }
}

void WatermelonSettingsDialog::saveSettings()
{
    Config::Table globalCfg = Config::GetGlobalTable();

    // General
    globalCfg.SetBool("PauseLostFocus", swPauseLostFocus->isChecked());
    globalCfg.SetBool("Mouse.Hide", swMouseHide->isChecked());
    globalCfg.SetBool("Emu.DirectBoot", swDirectBoot->isChecked());

    QStringList dirList;
    for (int i = 0; i < listRomDirs->count(); ++i) {
        dirList.append(listRomDirs->item(i)->text());
    }
    globalCfg.SetQString("WatermelonRomDirs", dirList.join(QLatin1Char(';')));

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

    // CPU
    globalCfg.SetBool("JIT.Enable", swJitEnable->isChecked());
    globalCfg.SetInt("JIT.MaxBlockSize", spnJitBlockSize->value());
    globalCfg.SetBool("JIT.BranchOptimisations", swJitBranchOpt->isChecked());
    globalCfg.SetBool("JIT.LiteralOptimisations", swJitLiteralOpt->isChecked());
    globalCfg.SetBool("JIT.FastMemory", swJitFastMem->isChecked());

    // Graphics
    globalCfg.SetInt("3D.Renderer", cbxRenderer3D->currentIndex());
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

    // Audio
    globalCfg.SetInt("Audio.Volume", sldVolume->value());
    globalCfg.SetInt("Audio.Interpolation", cbxInterpolation->currentIndex());

    // Controls
    if (inputConfigWidget) {
        inputConfigWidget->accept();
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

    Config::Save();
    emit settingsSaved();
}

void WatermelonSettingsDialog::onAddRomDirClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Seleccionar Carpeta de ROMs"));
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
        QStringLiteral("Restablecer Ajustes"),
        QStringLiteral("¿Estás seguro de que deseas restablecer todos los ajustes a sus valores por defecto?"),
        QMessageBox::Yes | QMessageBox::No
    );
    if (res == QMessageBox::Yes) {
        swPauseLostFocus->setChecked(true);
        swMouseHide->setChecked(true);
        swDirectBoot->setChecked(true);
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
        swRAHardcore->setChecked(true);
        swRARichPresence->setChecked(true);
        swRASound->setChecked(true);
        swRAUnofficial->setChecked(false);
        swRAEncore->setChecked(false);
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
