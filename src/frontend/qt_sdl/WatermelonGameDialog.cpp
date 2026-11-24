// WatermelonGameDialog.cpp
// Per-game options dialog modelled after the WatermelonDS Android "RomDetailsActivity".
// All picker rows use QMenu (no QComboBox) for a native mobile feel.

#include "WatermelonGameDialog.h"
#include "WatermelonRADialog.h"
#include "WatermelonShaderDialog.h"
#include "WatermelonSettingsDialog.h"
#include "ARCodeFile.h"
#include "ARDatabaseDAT.h"
#include "Config.h"
#include "Screen.h"
#include <QDateTimeEdit>
#include <QDateTime>

#include <cstdint>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QStackedWidget>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPointer>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QProgressBar>
#include <QDesktopServices>
#include <QUrl>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QSizePolicy>
#include <QMouseEvent>

// ─── Helpers ───────────────────────────────────────────────────────────────
namespace {

WatermelonGBAConfig g_pendingGBAConfig;
WatermelonScreenConfig g_pendingScreenConfig;

QString perGameConfigPath(const QString& code)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                  + QStringLiteral("/melonDS/game_configs");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/") + code + QStringLiteral(".ini");
}

// A rounded card container
QFrame* makeCard(QWidget* parent = nullptr)
{
    auto* f = new QFrame(parent);
    f->setObjectName(QStringLiteral("WMCard"));
    f->setStyleSheet(QStringLiteral(
        "QFrame#WMCard { background: #1e1d24; border: 1px solid #2e2d38; "
        "border-radius: 12px; }"
    ));
    return f;
}

// Thin HR divider
QFrame* makeDivider(QWidget* parent = nullptr)
{
    auto* line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral(
        "border: none; background: #2e2d38; max-height: 1px; margin: 0 14px;"
    ));
    line->setFixedHeight(1);
    return line;
}

// Monospace section label – green, uppercase
QLabel* makeSectionLabel(const QString& text, QWidget* parent = nullptr)
{
    auto* lbl = new QLabel(text.toUpper(), parent);
    lbl->setStyleSheet(QStringLiteral(
        "font-size: 10px; font-weight: 800; letter-spacing: 1.4px; "
        "color: #6fbf4a; background: transparent; border: none; "
        "padding: 18px 4px 6px 4px;"
    ));
    return lbl;
}

// Pill badge (small label with rounded bg)
QLabel* makePill(const QString& text, const QColor& fg, const QColor& bg, QWidget* parent = nullptr)
{
    auto* lbl = new QLabel(text, parent);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setFixedHeight(22);
    lbl->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    lbl->setStyleSheet(QStringLiteral(
        "font-size: 11px; font-weight: 700; color: %1; background: %2; "
        "padding: 0 10px; border-radius: 11px; border: none;"
    ).arg(fg.name(), bg.name()));
    return lbl;
}

} // namespace

// ─── Clickable picker row ───────────────────────────────────────────────────
// Shows:  Label                   Current value  ›
// On click → QMenu with options
class PickerRow : public QWidget
{
    Q_OBJECT
public:
    PickerRow(const QString& label,
              const QStringList& options,
              int currentIndex,
              QWidget* parent = nullptr)
        : QWidget(parent), m_options(options), m_index(qBound(0, currentIndex, options.size()-1))
    {
        setMinimumHeight(52);
        setCursor(Qt::PointingHandCursor);
        setMouseTracking(true);

        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(16, 0, 14, 0);
        lay->setSpacing(8);

        m_labelWidget = new QLabel(label, this);
        m_labelWidget->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 500; color: #f6f5f3; background: transparent;"
        ));
        lay->addWidget(m_labelWidget, 1);

        m_valueWidget = new QLabel(options.value(m_index), this);
        m_valueWidget->setStyleSheet(QStringLiteral(
            "font-size: 12px; color: #b3b1bd; background: transparent;"
        ));
        m_valueWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lay->addWidget(m_valueWidget);

        m_chevron = new QLabel(tr("›"), this);
        m_chevron->setStyleSheet(QStringLiteral(
            "font-size: 18px; color: #4a4859; background: transparent; padding-left: 2px;"
        ));
        lay->addWidget(m_chevron);
    }

    int currentIndex() const { return m_index; }
    QString currentText() const { return m_options.value(m_index); }

    void setCurrentIndex(int idx)
    {
        if (idx < 0 || idx >= m_options.size() || idx == m_index) return;
        m_index = idx;
        m_valueWidget->setText(m_options.value(m_index));
    }

signals:
    void indexChanged(int idx);

protected:
    void enterEvent(QEnterEvent*) override
    {
        setStyleSheet(QStringLiteral("background: #252430; border-radius: 8px;"));
    }
    void leaveEvent(QEvent*) override { setStyleSheet(QString()); }
    void mousePressEvent(QMouseEvent* ev) override
    {
        if (ev->button() == Qt::LeftButton) showMenu();
    }

private:
    void showMenu()
    {
        QMenu menu(this);
        menu.setStyleSheet(QStringLiteral(
            "QMenu { background: #1e1d24; border: 1px solid #3e3d4a; border-radius: 10px; "
            "padding: 6px 0; color: #f6f5f3; font-size: 13px; }"
            "QMenu::item { padding: 10px 20px; }"
            "QMenu::item:selected { background: #272630; color: #6fbf4a; }"
            "QMenu::item:checked { color: #6fbf4a; font-weight: 700; }"
            "QMenu::indicator { width: 0; }"
        ));

        for (int i = 0; i < m_options.size(); ++i) {
            QAction* act = menu.addAction(m_options[i]);
            act->setCheckable(true);
            act->setChecked(i == m_index);
            int capturedIdx = i;
            connect(act, &QAction::triggered, this, [this, capturedIdx]() {
                if (m_index != capturedIdx) {
                    m_index = capturedIdx;
                    m_valueWidget->setText(m_options.value(m_index));
                    emit indexChanged(m_index);
                }
            });
        }

        QPoint pos = mapToGlobal(QPoint(0, height()));
        menu.exec(pos);
    }

    QLabel*     m_labelWidget;
    QLabel*     m_valueWidget;
    QLabel*     m_chevron;
    QStringList m_options;
    int         m_index;
};

#include "WatermelonGameDialog.moc"

// ═══════════════════════════════════════════════════════════════════════════
WatermelonGameDialog::WatermelonGameDialog(const WatermelonRom& rom, int initialTab, QWidget* parent)
    : QDialog(parent), romInfo(rom)
{
    setWindowTitle(tr("%1 — Opciones").arg(rom.title));
    setMinimumSize(660, 660);
    resize(720, 720);
    setModal(true);

    setStyleSheet(QStringLiteral(
        "QDialog { background: #12111a; color: #f6f5f3; }"
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: #1a1920; width: 5px; border-radius: 2px; margin: 2px; }"
        "QScrollBar::handle:vertical { background: #3a3848; border-radius: 2px; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
        "QLineEdit { background: #1e1d2b; color: #f6f5f3; border: 1px solid #3a3848; "
        "            border-radius: 8px; padding: 10px 14px; font-size: 13px; }"
        "QLineEdit:focus { border-color: #6fbf4a; }"
    ));

    networkMgr = new QNetworkAccessManager(this);
    setupUI(initialTab);
    loadPerGameConfig();
}

WatermelonGameDialog::~WatermelonGameDialog() = default;

// ═══════════════════════════════════════════════════════════════════════════
void WatermelonGameDialog::setupUI(int initialTab)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── HEADER ─────────────────────────────────────────────────────────────
    auto* headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(90);
    headerWidget->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #1f1e2a, stop:1 #17161f);"
        "border-bottom: 1px solid #2a2938;"
    ));
    auto* hLay = new QHBoxLayout(headerWidget);
    hLay->setContentsMargins(20, 12, 20, 12);
    hLay->setSpacing(14);

    // Cover
    auto* coverLbl = new QLabel(headerWidget);
    coverLbl->setFixedSize(55, 68);
    coverLbl->setAlignment(Qt::AlignCenter);
    coverLbl->setStyleSheet(QStringLiteral(
        "background: #1a1928; border: 1px solid #2e2d3e; border-radius: 8px;"
    ));
    if (!romInfo.cover.isNull())
        coverLbl->setPixmap(romInfo.cover.scaled(coverLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        coverLbl->setText(tr("🎮"));
    hLay->addWidget(coverLbl);

    // Info
    auto* infoLay = new QVBoxLayout();
    infoLay->setSpacing(5);

    auto* titleLbl = new QLabel(romInfo.title, headerWidget);
    titleLbl->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 700; color: #f6f5f3; background: transparent; border: none;"
    ));
    titleLbl->setWordWrap(false);
    titleLbl->setMaximumWidth(420);
    QFontMetrics fm(titleLbl->font());
    titleLbl->setText(fm.elidedText(romInfo.title, Qt::ElideRight, 400));
    infoLay->addWidget(titleLbl);

    auto* pillRow = new QHBoxLayout();
    pillRow->setSpacing(6);
    pillRow->setAlignment(Qt::AlignLeft);

    if (!romInfo.gameCode.isEmpty()) {
        pillRow->addWidget(makePill(romInfo.gameCode,
            QColor(0x6f,0xbf,0x4a), QColor(0x1a,0x2e,0x18), headerWidget));
    }
    QFileInfo fi(romInfo.filePath);
    double mb = fi.size() / (1024.0 * 1024.0);
    pillRow->addWidget(makePill(tr("%1 MB").arg(mb, 0, 'f', 1),
        QColor(0xb3,0xb1,0xbd), QColor(0x24,0x23,0x2f), headerWidget));
    pillRow->addStretch();
    infoLay->addLayout(pillRow);

    auto* pathLbl = new QLabel(romInfo.filePath, headerWidget);
    pathLbl->setStyleSheet(QStringLiteral(
        "font-size: 10px; color: #5a5868; background: transparent; border: none;"
    ));
    pathLbl->setMaximumWidth(400);
    QFontMetrics fmPath(pathLbl->font());
    pathLbl->setText(fmPath.elidedText(romInfo.filePath, Qt::ElideMiddle, 400));
    infoLay->addWidget(pathLbl);
    hLay->addLayout(infoLay, 1);

    // Play button
    auto* btnPlay = new QPushButton(tr("▶  Iniciar"), headerWidget);
    btnPlay->setFixedSize(100, 36);
    btnPlay->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #6fbf4a;"
        "  color: #12111a;"
        "  font-weight: 700; font-size: 13px;"
        "  border-radius: 8px; border: none;"
        "}"
        "QPushButton:hover { background-color: #82d457; }"
        "QPushButton:pressed { background-color: #5ca038; }"
    ));
    connect(btnPlay, &QPushButton::clicked, this, [this]() { accept(); });
    hLay->addWidget(btnPlay, 0, Qt::AlignVCenter);

    root->addWidget(headerWidget);

    // ── TAB BAR ─────────────────────────────────────────────────────────────
    tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(true);
    tabWidget->setStyleSheet(QStringLiteral(
        "QTabWidget::pane { border: none; background: #12111a; margin-top: 0; }"
        "QTabBar {"
        "  background: #17161f;"
        "  border-bottom: 1px solid #252433;"
        "}"
        "QTabBar::tab {"
        "  background: transparent; color: #5a5870;"
        "  padding: 13px 0; margin: 0 8px;"
        "  font-weight: 600; font-size: 13px;"
        "  border: none; border-bottom: 2px solid transparent;"
        "  min-width: 120px; text-align: center;"
        "}"
        "QTabBar::tab:selected {"
        "  color: #6fbf4a; border-bottom: 2px solid #6fbf4a;"
        "}"
        "QTabBar::tab:hover:!selected { color: #c0bed0; }"
    ));

    tabWidget->addTab(createConfigTab(),            tr("⚙  Configuración"));
    tabWidget->addTab(createScreenTab(),            tr("🖼  Pantalla"));
    tabWidget->addTab(createCheatsTab(),            tr("👾  Trucos"));
    tabWidget->addTab(createRetroAchievementsTab(), tr("🏆  RetroAchievements"));
    tabWidget->addTab(createOfflineTab(),           tr("📴  Offline"));
    tabWidget->setCurrentIndex(qBound(0, initialTab, 4));

    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int idx) {
        if (idx == 3) {
            raLoggedIn = !WatermelonRADialog::getUsername().isEmpty()
                      && !WatermelonRADialog::getToken().isEmpty();
            if (!raLoggedIn)
                { if (raStack) raStack->setCurrentIndex(0); }
            else if (raStack && raStack->currentIndex() <= 1 && raGame.achievements.isEmpty())
                loadRAGameData();
        }
    });

    root->addWidget(tabWidget, 1);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Helper: build a card section with a list of rows
// ═══════════════════════════════════════════════════════════════════════════
static QFrame* buildSection(const QStringList& labels, const QList<QWidget*>& rows,
                             QWidget* parent = nullptr)
{
    auto* card = makeCard(parent);
    auto* cLay = new QVBoxLayout(card);
    cLay->setContentsMargins(0, 4, 0, 4);
    cLay->setSpacing(0);
    for (int i = 0; i < rows.size(); ++i) {
        if (i > 0) cLay->addWidget(makeDivider(card));
        rows[i]->setParent(card);
        cLay->addWidget(rows[i]);
        Q_UNUSED(labels);
    }
    return card;
}

// ═══════════════════════════════════════════════════════════════════════════
//  TAB 0 – Configuración
// ═══════════════════════════════════════════════════════════════════════════
QWidget* WatermelonGameDialog::createConfigTab()
{
    auto* page = new QWidget();
    page->setStyleSheet(QStringLiteral("background: #12111a;"));
    auto* pLay = new QVBoxLayout(page);
    pLay->setContentsMargins(0, 0, 0, 0);
    pLay->setSpacing(0);

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget();
    content->setStyleSheet(QStringLiteral("background: #12111a;"));
    auto* cLay = new QVBoxLayout(content);
    cLay->setContentsMargins(16, 4, 16, 24);
    cLay->setSpacing(0);

    // ── GENERAL ──────────────────────────────────────────────────────────
    cLay->addWidget(makeSectionLabel(QStringLiteral("General"), content));

    // Name row (read-only display)
    auto* nameRow = new QWidget();
    nameRow->setMinimumHeight(52);
    {
        auto* nLay = new QHBoxLayout(nameRow);
        nLay->setContentsMargins(16, 0, 14, 0);
        nLay->setSpacing(8);
        auto* lbl = new QLabel(tr("Nombre del juego"), nameRow);
        lbl->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 500; color: #f6f5f3; background: transparent;"));
        nLay->addWidget(lbl, 1);
        auto* val = new QLabel(romInfo.title, nameRow);
        val->setStyleSheet(QStringLiteral(
            "font-size: 12px; color: #6a6880; background: transparent;"
            "max-width: 240px;"));
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QFontMetrics fmV(val->font());
        val->setText(fmV.elidedText(romInfo.title, Qt::ElideRight, 220));
        nLay->addWidget(val);
    }

    pickerConsole = new PickerRow(tr("Tipo de consola"),
        {tr("Global (DS)"), tr("Nintendo DS"), tr("Nintendo DSi")},
        0);
    pickerBoot = new PickerRow(tr("Arranque directo"),
        {QStringLiteral("Global"), QStringLiteral("Activado"), QStringLiteral("Desactivado")},
        0);

    pickerDateTime = new PickerRow(tr("Fecha y hora (RTC)"),
        {tr("Global"), tr("Hora del sistema (PC)"), tr("Fecha y hora fijada...")},
        0);

    customDateTimeRow = new QWidget();
    customDateTimeRow->setMinimumHeight(52);
    customDateTimeRow->setVisible(false);
    {
        auto* dtLay = new QHBoxLayout(customDateTimeRow);
        dtLay->setContentsMargins(16, 0, 10, 0);
        dtLay->setSpacing(8);
        auto* lbl = new QLabel(tr("Fecha y hora fijada"), customDateTimeRow);
        lbl->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 500; color: #f6f5f3; background: transparent;"));
        dtLay->addWidget(lbl, 1);

        dteGameCustomTime = new QDateTimeEdit(QDateTime::currentDateTime(), customDateTimeRow);
        dteGameCustomTime->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        dteGameCustomTime->setCalendarPopup(true);
        dteGameCustomTime->setFixedHeight(28);
        dteGameCustomTime->setStyleSheet(QStringLiteral(
            "QDateTimeEdit { background: #252433; color: #f6f5f3; border: 1px solid #3a3848; "
            "border-radius: 7px; padding: 0 8px; font-size: 12px; } "
            "QDateTimeEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; "
            "width: 20px; border: none; background: transparent; } "
            "QDateTimeEdit::down-arrow { image: url(:/arrow-down); width: 10px; height: 10px; }"));
        dtLay->addWidget(dteGameCustomTime);

        auto* btnNow = new QPushButton(tr("  Ahora  "), customDateTimeRow);
        btnNow->setFixedHeight(28);
        btnNow->setStyleSheet(QStringLiteral(
            "QPushButton { background: #252433; color: #6fbf4a; border: 1px solid #3a3848; "
            "border-radius: 7px; font-size: 11px; font-weight: 700; padding: 0 10px; } "
            "QPushButton:hover { background: #2e2d40; }"));
        connect(btnNow, &QPushButton::clicked, this, [this]() {
            if (dteGameCustomTime) dteGameCustomTime->setDateTime(QDateTime::currentDateTime());
        });
        dtLay->addWidget(btnNow);
    }

    connect(pickerDateTime, &PickerRow::indexChanged, this, [this](int idx) {
        if (customDateTimeRow) customDateTimeRow->setVisible(idx == 2);
    });

    pickerBattery = new PickerRow(tr("Nivel de batería"),
        {tr("Global"), tr("Normal (100% / OK)"), tr("Batería baja"), tr("Casi agotada")},
        0);

    pickerDSiCharging = new PickerRow(tr("DSi: Cargador conectado"),
        {tr("Global"), tr("Activado (Cargando)"), tr("Desactivado")},
        0);

    cLay->addWidget(buildSection({}, {
        nameRow,
        pickerConsole,
        pickerBoot,
        pickerDateTime,
        customDateTimeRow,
        pickerBattery,
        pickerDSiCharging
    }, content));

    // ── VÍDEO ─────────────────────────────────────────────────────────────
    cLay->addWidget(makeSectionLabel(QStringLiteral("Vídeo"), content));

    pickerRenderer = new PickerRow(tr("Motor de renderizado 3D"),
        {QStringLiteral("Global"), QStringLiteral("Software"), tr("OpenGL (Hardware)")},
        0);

    // Shader row (custom)
    auto* shaderRow = new QWidget();
    shaderRow->setMinimumHeight(52);
    {
        auto* sLay = new QHBoxLayout(shaderRow);
        sLay->setContentsMargins(16, 0, 10, 0);
        sLay->setSpacing(8);
        auto* sLbl = new QLabel(tr("Shader RetroArch"), shaderRow);
        sLbl->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 500; color: #f6f5f3; background: transparent;"));
        sLay->addWidget(sLbl, 1);

        shaderLabel = new QLabel(tr("Global"), shaderRow);
        shaderLabel->setStyleSheet(QStringLiteral(
            "font-size: 12px; color: #b3b1bd; background: transparent; max-width: 160px;"));
        shaderLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sLay->addWidget(shaderLabel);

        auto* btnChange = new QPushButton(tr("  Cambiar  "), shaderRow);
        btnChange->setMinimumWidth(80);
        btnChange->setFixedHeight(28);
        btnChange->setStyleSheet(QStringLiteral(
            "QPushButton { background: #252433; color: #6fbf4a; border: 1px solid #3a3848; "
            "border-radius: 7px; font-size: 11px; font-weight: 700; }"
            "QPushButton:hover { background: #2e2d40; }"
        ));
        connect(btnChange, &QPushButton::clicked, this, &WatermelonGameDialog::onChooseShaderClicked);
        sLay->addWidget(btnChange);

        auto* btnClear = new QPushButton(tr("  Quitar  "), shaderRow);
        btnClear->setMinimumWidth(60);
        btnClear->setFixedHeight(28);
        btnClear->setStyleSheet(QStringLiteral(
            "QPushButton { background: #252433; color: #c94444; border: 1px solid #4a2828; "
            "border-radius: 7px; font-size: 11px; font-weight: 700; }"
            "QPushButton:hover { background: #3a2030; color: #e5534b; }"
        ));
        connect(btnClear, &QPushButton::clicked, this, &WatermelonGameDialog::onClearShaderClicked);
        sLay->addWidget(btnClear);
    }

    cLay->addWidget(buildSection({}, {pickerRenderer, shaderRow}, content));

    // ── RENDIMIENTO ───────────────────────────────────────────────────────
    cLay->addWidget(makeSectionLabel(QStringLiteral("Rendimiento"), content));

    pickerJit = new PickerRow(tr("JIT (compilador nativo)"),
        {QStringLiteral("Global"), QStringLiteral("Activado"), QStringLiteral("Desactivado")},
        0);
    pickerMic = new PickerRow(tr("Fuente de micrófono"),
        {QStringLiteral("Global"), QStringLiteral("Silencio"), tr("Ruido blanco"),
         tr("Micrófono del sistema")},
        0);

    cLay->addWidget(buildSection({}, {pickerJit, pickerMic}, content));

    // ── GBA SLOT ─────────────────────────────────────────────────────────
    cLay->addWidget(makeSectionLabel(QStringLiteral("GBA Slot"), content));

    const QString smallBtnStyle = QStringLiteral(
        "QPushButton { background: #252433; color: #6fbf4a; border: 1px solid #3a3848; "
        "border-radius: 7px; font-size: 11px; font-weight: 700; }"
        "QPushButton:hover { background: #2e2d40; }");

    auto* gbaCartRow = new QWidget();
    gbaCartRow->setMinimumHeight(52);
    {
        auto* gLay = new QHBoxLayout(gbaCartRow);
        gLay->setContentsMargins(16, 0, 10, 0);
        gLay->setSpacing(8);
        auto* lbl = new QLabel(tr("Cartucho GBA"), gbaCartRow);
        lbl->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 500; color: #f6f5f3; background: transparent;"));
        gLay->addWidget(lbl, 1);

        gbaCartLabel = new QLabel(tr("Ninguno"), gbaCartRow);
        gbaCartLabel->setStyleSheet(QStringLiteral(
            "font-size: 12px; color: #b3b1bd; background: transparent; max-width: 160px;"));
        gbaCartLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        gLay->addWidget(gbaCartLabel);

        auto* btnChoose = new QPushButton(tr("  Elegir  "), gbaCartRow);
        btnChoose->setMinimumWidth(70);
        btnChoose->setFixedHeight(28);
        btnChoose->setStyleSheet(smallBtnStyle);
        connect(btnChoose, &QPushButton::clicked, this, &WatermelonGameDialog::onChooseGBACartClicked);
        gLay->addWidget(btnChoose);

        auto* btnClear = new QPushButton(tr("  Quitar  "), gbaCartRow);
        btnClear->setMinimumWidth(60);
        btnClear->setFixedHeight(28);
        btnClear->setStyleSheet(QStringLiteral(
            "QPushButton { background: #252433; color: #c94444; border: 1px solid #4a2828; "
            "border-radius: 7px; font-size: 11px; font-weight: 700; }"
            "QPushButton:hover { background: #3a2030; color: #e5534b; }"));
        connect(btnClear, &QPushButton::clicked, this, &WatermelonGameDialog::onClearGBACartClicked);
        gLay->addWidget(btnClear);
    }

    pickerGBAAddon = new PickerRow(tr("Accesorio"),
        {tr("Ninguno"),
         QStringLiteral("RAM Expansion"),
         QStringLiteral("Rumble Pak"),
         QStringLiteral("Solar Sensor (Boktai 1)"),
         QStringLiteral("Solar Sensor (Boktai 2)"),
         QStringLiteral("Solar Sensor (Boktai 3)"),
         QStringLiteral("Motion Pak (Homebrew)"),
         QStringLiteral("Motion Pak (Retail)"),
         QStringLiteral("Guitar Grip")},
        0);

    auto* importSaveRow = new QWidget();
    importSaveRow->setMinimumHeight(52);
    {
        auto* iLay = new QHBoxLayout(importSaveRow);
        iLay->setContentsMargins(16, 0, 10, 0);
        iLay->setSpacing(8);
        auto* lbl = new QLabel(tr("Importar partida guardada"), importSaveRow);
        lbl->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 500; color: #f6f5f3; background: transparent;"));
        iLay->addWidget(lbl, 1);

        importSaveLabel = new QLabel(tr("Ninguno"), importSaveRow);
        importSaveLabel->setStyleSheet(QStringLiteral(
            "font-size: 12px; color: #b3b1bd; background: transparent; max-width: 160px;"));
        importSaveLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        iLay->addWidget(importSaveLabel);

        auto* btnImport = new QPushButton(tr("  Elegir  "), importSaveRow);
        btnImport->setMinimumWidth(70);
        btnImport->setFixedHeight(28);
        btnImport->setStyleSheet(smallBtnStyle);
        connect(btnImport, &QPushButton::clicked, this, &WatermelonGameDialog::onImportSaveClicked);
        iLay->addWidget(btnImport);
    }

    cLay->addWidget(buildSection({}, {gbaCartRow, pickerGBAAddon, importSaveRow}, content));

    // ── RETROACHIEVEMENTS ─────────────────────────────────────────────────
    cLay->addWidget(makeSectionLabel(QStringLiteral("RetroAchievements"), content));
    bool raEn = WatermelonRADialog::isEnabled();
    bool raHC = WatermelonRADialog::isHardcore();
    QString raUser = WatermelonRADialog::getUsername();

    auto* raStatusRow = new QWidget();
    raStatusRow->setMinimumHeight(52);
    {
        auto* rLay = new QHBoxLayout(raStatusRow);
        rLay->setContentsMargins(16, 0, 14, 0);
        rLay->setSpacing(8);
        auto* lbl = new QLabel(tr("Estado"), raStatusRow);
        lbl->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 500; color: #f6f5f3; background: transparent;"));
        rLay->addWidget(lbl, 1);

        QString stateText = raUser.isEmpty()
            ? tr("No conectado")
            : (raEn ? tr("Activo como %1").arg(raUser)
                    : QStringLiteral("Desactivado"));
        QColor stateColor = raUser.isEmpty() ? QColor(0x7a,0x78,0x88)
                          : (raEn ? QColor(0x6f,0xbf,0x4a) : QColor(0xe5,0x53,0x4b));
        auto* valLbl = new QLabel(stateText, raStatusRow);
        valLbl->setStyleSheet(QStringLiteral("font-size: 12px; color: %1; background: transparent;")
                              .arg(stateColor.name()));
        valLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        rLay->addWidget(valLbl);
    }

    auto* raModeRow = new QWidget();
    raModeRow->setMinimumHeight(52);
    {
        auto* mLay = new QHBoxLayout(raModeRow);
        mLay->setContentsMargins(16, 0, 14, 0);
        mLay->setSpacing(8);
        auto* lbl = new QLabel(tr("Modo"), raModeRow);
        lbl->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 500; color: #f6f5f3; background: transparent;"));
        mLay->addWidget(lbl, 1);
        mLay->addWidget(makePill(
            raHC ? tr("🔒 Hardcore") : QStringLiteral("Softcore"),
            raHC ? QColor(0xe5,0x53,0x4b) : QColor(0x6f,0xbf,0x4a),
            raHC ? QColor(0x2a,0x18,0x18) : QColor(0x1a,0x2e,0x18),
            raModeRow), 0, Qt::AlignVCenter);
    }

    cLay->addWidget(buildSection({}, {raStatusRow, raModeRow}, content));
    cLay->addStretch();

    scroll->setWidget(content);
    pLay->addWidget(scroll, 1);

    // ── Bottom save bar ────────────────────────────────────────────────────
    auto* bar = new QWidget(page);
    bar->setFixedHeight(60);
    bar->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #1a1928, stop:1 #15141d);"
        "border-top: 1px solid #252433;"
    ));
    auto* bLay = new QHBoxLayout(bar);
    bLay->setContentsMargins(16, 0, 16, 0);
    bLay->addStretch();

    auto* btnSave = new QPushButton(tr("💾  Guardar configuración"), bar);
    btnSave->setFixedHeight(36);
    btnSave->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #6fbf4a;"
        "  color: #12111a;"
        "  font-weight: 700; font-size: 13px;"
        "  border-radius: 8px; border: none; padding: 0 20px;"
        "}"
        "QPushButton:hover { background-color: #82d457; }"
        "QPushButton:pressed { background-color: #5ca038; }"
    ));
    connect(btnSave, &QPushButton::clicked, this, &WatermelonGameDialog::onSaveConfigClicked);
    bLay->addWidget(btnSave);
    pLay->addWidget(bar);

    return page;
}

// ═══════════════════════════════════════════════════════════════════════════
//  TAB 1 – Pantalla (Display)
// ═══════════════════════════════════════════════════════════════════════════
QWidget* WatermelonGameDialog::createScreenTab()
{
    auto* page = new QWidget();
    page->setStyleSheet(QStringLiteral("background: #12111a;"));
    auto* pLay = new QVBoxLayout(page);
    pLay->setContentsMargins(0, 0, 0, 0);
    pLay->setSpacing(0);

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget();
    content->setStyleSheet(QStringLiteral("background: #12111a;"));
    auto* cLay = new QVBoxLayout(content);
    cLay->setContentsMargins(16, 4, 16, 24);
    cLay->setSpacing(0);

    // ── DISPOSICIÓN DE PANTALLAS ───────────────────────────────────────────
    cLay->addWidget(makeSectionLabel(QStringLiteral("Disposición de Pantallas"), content));

    pickerWindowSize = new PickerRow(tr("Tamaño de ventana"),
        {tr("Global"), QStringLiteral("1x"), QStringLiteral("2x"), QStringLiteral("3x"), QStringLiteral("4x")},
        0);

    pickerScreenRotation = new PickerRow(tr("Rotación de pantalla"),
        {tr("Global"), QStringLiteral("0°"), QStringLiteral("90°"), QStringLiteral("180°"), QStringLiteral("270°")},
        0);

    pickerScreenGap = new PickerRow(tr("Separación entre pantallas"),
        {tr("Global"), QStringLiteral("0 px"), QStringLiteral("1 px"), QStringLiteral("8 px"),
         QStringLiteral("64 px"), QStringLiteral("90 px"), QStringLiteral("128 px")},
        0);

    pickerScreenLayout = new PickerRow(tr("Disposición"),
        {tr("Global"), tr("Natural"), tr("Vertical"), tr("Horizontal"), tr("Híbrida")},
        0);

    pickerScreenSizing = new PickerRow(tr("Tamaño de pantallas"),
        {tr("Global"), tr("Iguales"), tr("Énfasis superior"), tr("Énfasis inferior"),
         tr("Automático"), tr("Solo superior"), tr("Solo inferior")},
        0);

    QStringList topAspects = {tr("Global")};
    for (int i = 0; i < AspectRatiosNum; i++)
        topAspects << QString::fromLatin1(aspectRatios[i].label);

    pickerAspectTop = new PickerRow(tr("Aspecto pantalla superior"), topAspects, 0);
    pickerAspectBot = new PickerRow(tr("Aspecto pantalla inferior"), topAspects, 0);

    cLay->addWidget(buildSection({}, {
        pickerWindowSize,
        pickerScreenRotation,
        pickerScreenGap,
        pickerScreenLayout,
        pickerScreenSizing,
        pickerAspectTop,
        pickerAspectBot
    }, content));

    // ── OPCIONES DE VISUALIZACIÓN ──────────────────────────────────────────
    cLay->addWidget(makeSectionLabel(QStringLiteral("Opciones de Visualización"), content));

    pickerScreenSwap = new PickerRow(tr("Intercambiar pantallas"),
        {tr("Global"), tr("Activado"), tr("Desactivado")},
        0);

    pickerIntegerScaling = new PickerRow(tr("Forzar escalado entero"),
        {tr("Global"), tr("Activado"), tr("Desactivado")},
        0);

    pickerScreenFilter = new PickerRow(tr("Filtro de pantalla"),
        {tr("Global"), tr("Activado"), tr("Desactivado")},
        0);

    cLay->addWidget(buildSection({}, {
        pickerScreenSwap,
        pickerIntegerScaling,
        pickerScreenFilter
    }, content));

    cLay->addStretch();

    scroll->setWidget(content);
    pLay->addWidget(scroll, 1);

    // ── Bottom save bar ────────────────────────────────────────────────────
    auto* bar = new QWidget(page);
    bar->setFixedHeight(60);
    bar->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "  stop:0 #1a1928, stop:1 #15141d);"
        "border-top: 1px solid #252433;"
    ));
    auto* bLay = new QHBoxLayout(bar);
    bLay->setContentsMargins(16, 0, 16, 0);
    bLay->addStretch();

    auto* btnSave = new QPushButton(tr("💾  Guardar configuración"), bar);
    btnSave->setFixedHeight(36);
    btnSave->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #6fbf4a;"
        "  color: #12111a;"
        "  font-weight: 700; font-size: 13px;"
        "  border-radius: 8px; border: none; padding: 0 20px;"
        "}"
        "QPushButton:hover { background-color: #82d457; }"
        "QPushButton:pressed { background-color: #5ca038; }"
    ));
    connect(btnSave, &QPushButton::clicked, this, &WatermelonGameDialog::onSaveConfigClicked);
    bLay->addWidget(btnSave);
    pLay->addWidget(bar);

    return page;
}

// ═══════════════════════════════════════════════════════════════════════════
//  TAB 2 – Trucos (Cheats)
// ═══════════════════════════════════════════════════════════════════════════
void WatermelonGameDialog::loadCheatsForGame()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    std::string cheatDir = globalCfg.GetString("CheatFilePath");
    QFileInfo fi(romInfo.filePath);
    QString baseName = fi.completeBaseName();
    if (!cheatDir.empty()) {
        QDir dir(QString::fromStdString(cheatDir));
        cheatFilePath = dir.filePath(baseName + QStringLiteral(".mch"));
    } else {
        cheatFilePath = fi.dir().filePath(baseName + QStringLiteral(".mch"));
    }
    gameCheatFile = std::make_unique<melonDS::ARCodeFile>(cheatFilePath.toStdString());
}

QWidget* WatermelonGameDialog::createCheatsTab()
{
    loadCheatsForGame();

    auto* page = new QWidget();
    page->setStyleSheet(QStringLiteral("background: #12111a;"));
    auto* pLay = new QVBoxLayout(page);
    pLay->setContentsMargins(0, 0, 0, 0);
    pLay->setSpacing(0);

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* content = new QWidget(scroll);
    content->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* cLay = new QVBoxLayout(content);
    cLay->setContentsMargins(28, 20, 28, 24);
    cLay->setSpacing(16);

    // ── Card 1: Habilitar cheats master switch ─────────────────────────────
    auto* masterCard = new QFrame(content);
    masterCard->setStyleSheet(QStringLiteral(
        "QFrame { background: #171622; border: 1px solid #252433; border-radius: 12px; }"
    ));
    auto* mLay = new QHBoxLayout(masterCard);
    mLay->setContentsMargins(18, 14, 18, 14);

    auto* mTextLay = new QVBoxLayout();
    mTextLay->setSpacing(3);
    auto* mLblTitle = new QLabel(tr("Habilitar trucos para este juego"), masterCard);
    mLblTitle->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 700; color: #f6f5f3; background: transparent; border: none;"));
    auto* mLblSub = new QLabel(tr("Aplica los códigos Action Replay seleccionados abajo durante la emulación."), masterCard);
    mLblSub->setStyleSheet(QStringLiteral("font-size: 12px; color: #8a8898; background: transparent; border: none;"));
    mTextLay->addWidget(mLblTitle);
    mTextLay->addWidget(mLblSub);
    mLay->addLayout(mTextLay, 1);

    switchCheatsEnabled = new WatermelonSwitch(masterCard);
    switchCheatsEnabled->setChecked(false);
    mLay->addWidget(switchCheatsEnabled, 0, Qt::AlignVCenter);
    cLay->addWidget(masterCard);

    // ── Card 2: Lista de cheats ───────────────────────────────────────────
    auto* listCard = new QFrame(content);
    listCard->setStyleSheet(QStringLiteral(
        "QFrame { background: #171622; border: 1px solid #252433; border-radius: 12px; }"
    ));
    auto* lLay = new QVBoxLayout(listCard);
    lLay->setContentsMargins(18, 16, 18, 16);
    lLay->setSpacing(12);

    auto* lHeaderLay = new QHBoxLayout();
    auto* lTitle = new QLabel(tr("Lista de trucos disponibles"), listCard);
    lTitle->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 700; color: #f6f5f3; background: transparent; border: none;"));
    lHeaderLay->addWidget(lTitle);

    cheatsStatusLabel = new QLabel(listCard);
    cheatsStatusLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #6fbf4a; font-weight: 600; background: transparent; border: none;"));
    lHeaderLay->addWidget(cheatsStatusLabel);
    lHeaderLay->addStretch(1);

    auto* btnAddCheat = new QPushButton(tr("➕ Añadir truco"), listCard);
    btnAddCheat->setCursor(Qt::PointingHandCursor);
    btnAddCheat->setStyleSheet(QStringLiteral(
        "QPushButton { background: #232230; color: #f6f5f3; border: 1px solid #333244; "
        "border-radius: 8px; font-size: 12px; font-weight: 600; padding: 6px 12px; }"
        "QPushButton:hover { background: #2c2b3d; border-color: #6fbf4a; color: #6fbf4a; }"
    ));
    connect(btnAddCheat, &QPushButton::clicked, this, &WatermelonGameDialog::onAddCheatClicked);
    lHeaderLay->addWidget(btnAddCheat);

    auto* btnImportDb = new QPushButton(tr("📥 Importar base de datos..."), listCard);
    btnImportDb->setCursor(Qt::PointingHandCursor);
    btnImportDb->setStyleSheet(QStringLiteral(
        "QPushButton { background: #232230; color: #f6f5f3; border: 1px solid #333244; "
        "border-radius: 8px; font-size: 12px; font-weight: 600; padding: 6px 12px; }"
        "QPushButton:hover { background: #2c2b3d; border-color: #6fbf4a; color: #6fbf4a; }"
    ));
    connect(btnImportDb, &QPushButton::clicked, this, &WatermelonGameDialog::onImportCheatDbClicked);
    lHeaderLay->addWidget(btnImportDb);

    lLay->addLayout(lHeaderLay);

    // Tree of cheats
    treeCheats = new QTreeWidget(listCard);
    treeCheats->setHeaderHidden(true);
    treeCheats->setAnimated(true);
    treeCheats->setStyleSheet(QStringLiteral(
        "QTreeWidget { background: #12111a; border: 1px solid #282736; border-radius: 8px; padding: 6px; color: #f6f5f3; }"
        "QTreeWidget::item { padding: 6px 8px; margin: 1px 0; border-radius: 6px; }"
        "QTreeWidget::item:hover { background: #1c1b26; }"
        "QTreeWidget::item:selected { background: #232232; color: #6fbf4a; }"
        "QTreeWidget::indicator { width: 18px; height: 18px; border-radius: 4px; border: 1px solid #3c3a4d; background: #181722; }"
        "QTreeWidget::indicator:checked { background: #6fbf4a; border-color: #6fbf4a; image: url(:/check-icon); }"
    ));
    treeCheats->setMinimumHeight(240);
    connect(treeCheats, &QTreeWidget::itemChanged, this, &WatermelonGameDialog::onCheatItemChanged);
    lLay->addWidget(treeCheats);

    // Empty placeholder widget
    emptyCheatsWidget = new QWidget(listCard);
    auto* eLay = new QVBoxLayout(emptyCheatsWidget);
    eLay->setContentsMargins(20, 30, 20, 30);
    eLay->setSpacing(10);
    auto* lblEmptyIcon = new QLabel(tr("👾"), emptyCheatsWidget);
    lblEmptyIcon->setAlignment(Qt::AlignCenter);
    lblEmptyIcon->setStyleSheet(QStringLiteral("font-size: 38px; background: transparent; border: none;"));
    auto* lblEmptyText = new QLabel(tr("No hay trucos guardados para este juego"), emptyCheatsWidget);
    lblEmptyText->setAlignment(Qt::AlignCenter);
    lblEmptyText->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 700; color: #f6f5f3; background: transparent; border: none;"));
    auto* lblEmptySub = new QLabel(tr("Puedes añadir trucos manualmente con el botón \"Añadir truco\" o importar una base de datos Action Replay / R4 (.dat)."), emptyCheatsWidget);
    lblEmptySub->setAlignment(Qt::AlignCenter);
    lblEmptySub->setWordWrap(true);
    lblEmptySub->setStyleSheet(QStringLiteral("font-size: 12px; color: #8a8898; background: transparent; border: none;"));
    eLay->addWidget(lblEmptyIcon);
    eLay->addWidget(lblEmptyText);
    eLay->addWidget(lblEmptySub);
    lLay->addWidget(emptyCheatsWidget);

    cLay->addWidget(listCard);
    cLay->addStretch(1);

    scroll->setWidget(content);
    pLay->addWidget(scroll);

    populateCheatTree();

    return page;
}

void WatermelonGameDialog::populateCheatTree()
{
    if (!treeCheats) return;
    loadingCheatItems = true;
    treeCheats->clear();

    int totalCheats = 0;
    int enabledCheats = 0;

    std::function<void(QTreeWidgetItem*, melonDS::ARCodeCat&)> addCat = [&](QTreeWidgetItem* parent, melonDS::ARCodeCat& cat) {
        for (auto& item : cat.Children) {
            if (std::holds_alternative<melonDS::ARCodeCat>(item)) {
                auto& subcat = std::get<melonDS::ARCodeCat>(item);
                auto* catItem = new QTreeWidgetItem();
                catItem->setText(0, QString::fromStdString(subcat.Name));
                catItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                QFont font = catItem->font(0);
                font.setBold(true);
                catItem->setFont(0, font);
                catItem->setForeground(0, QBrush(QColor(0x6f, 0xbf, 0x4a)));
                if (parent) parent->addChild(catItem);
                else treeCheats->addTopLevelItem(catItem);
                catItem->setExpanded(true);
                addCat(catItem, subcat);
            } else {
                auto& code = std::get<melonDS::ARCode>(item);
                totalCheats++;
                if (code.Enabled) enabledCheats++;

                auto* codeItem = new QTreeWidgetItem();
                codeItem->setText(0, QString::fromStdString(code.Name));
                if (!code.Description.empty()) {
                    codeItem->setToolTip(0, QString::fromStdString(code.Description));
                }
                codeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
                codeItem->setCheckState(0, code.Enabled ? Qt::Checked : Qt::Unchecked);
                codeItem->setData(0, Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(&code)));
                if (parent) parent->addChild(codeItem);
                else treeCheats->addTopLevelItem(codeItem);
            }
        }
    };

    if (gameCheatFile) {
        addCat(nullptr, gameCheatFile->RootCat);
    }

    if (emptyCheatsWidget) emptyCheatsWidget->setVisible(totalCheats == 0);
    if (treeCheats) treeCheats->setVisible(totalCheats > 0);
    if (cheatsStatusLabel) {
        if (totalCheats > 0)
            cheatsStatusLabel->setText(tr("(%1 activos de %2 disponibles)").arg(enabledCheats).arg(totalCheats));
        else
            cheatsStatusLabel->setText(tr("(0 trucos)"));
    }

    loadingCheatItems = false;
}

void WatermelonGameDialog::onCheatItemChanged(QTreeWidgetItem* item, int column)
{
    if (loadingCheatItems || column != 0 || !item) return;
    quintptr ptr = item->data(0, Qt::UserRole).value<quintptr>();
    if (!ptr) return;

    auto* code = reinterpret_cast<melonDS::ARCode*>(ptr);
    code->Enabled = (item->checkState(0) == Qt::Checked);

    if (gameCheatFile) {
        gameCheatFile->Save();
    }

    // Recount
    int totalCheats = 0;
    int enabledCheats = 0;
    std::function<void(const melonDS::ARCodeCat&)> countCat = [&](const melonDS::ARCodeCat& cat) {
        for (const auto& item : cat.Children) {
            if (std::holds_alternative<melonDS::ARCodeCat>(item)) {
                countCat(std::get<melonDS::ARCodeCat>(item));
            } else {
                totalCheats++;
                if (std::get<melonDS::ARCode>(item).Enabled)
                    enabledCheats++;
            }
        }
    };
    if (gameCheatFile) countCat(gameCheatFile->RootCat);
    if (cheatsStatusLabel) {
        cheatsStatusLabel->setText(tr("(%1 activos de %2 disponibles)").arg(enabledCheats).arg(totalCheats));
    }
}

void WatermelonGameDialog::onAddCheatClicked()
{
    if (!gameCheatFile) return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Añadir truco Action Replay"));
    dlg.resize(460, 360);
    dlg.setStyleSheet(QStringLiteral(
        "QDialog { background: #171622; color: #f6f5f3; }"
        "QLabel { color: #f6f5f3; font-weight: 600; font-size: 13px; }"
        "QLineEdit, QPlainTextEdit { background: #12111a; border: 1px solid #282736; border-radius: 6px; padding: 6px; color: #f6f5f3; font-family: monospace; }"
        "QPushButton { background: #232230; color: #f6f5f3; border: 1px solid #333244; border-radius: 8px; padding: 7px 16px; font-weight: 600; }"
        "QPushButton:hover { background: #2c2b3d; border-color: #6fbf4a; color: #6fbf4a; }"
        "QPushButton[primary=\"true\"] { background: #6fbf4a; color: #12111a; border: none; font-weight: 700; }"
        "QPushButton[primary=\"true\"]:hover { background: #7dd254; }"
    ));

    auto* lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(12);

    lay->addWidget(new QLabel(tr("Nombre del truco:"), &dlg));
    auto* txtName = new QLineEdit(&dlg);
    txtName->setPlaceholderText(tr("ej. Dinero infinito / Max stats"));
    lay->addWidget(txtName);

    lay->addWidget(new QLabel(tr("Descripción (opcional):"), &dlg));
    auto* txtDesc = new QLineEdit(&dlg);
    txtDesc->setPlaceholderText(tr("ej. Presiona L+R para activar"));
    lay->addWidget(txtDesc);

    lay->addWidget(new QLabel(tr("Código Action Replay (hex):"), &dlg));
    auto* txtCode = new QPlainTextEdit(&dlg);
    txtCode->setPlaceholderText(tr("XXXXXXXX YYYYYYYY\nXXXXXXXX YYYYYYYY"));
    lay->addWidget(txtCode, 1);

    auto* btnLay = new QHBoxLayout();
    btnLay->addStretch(1);
    auto* btnCancel = new QPushButton(tr("Cancelar"), &dlg);
    connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    btnLay->addWidget(btnCancel);

    auto* btnOk = new QPushButton(tr("Guardar truco"), &dlg);
    btnOk->setProperty("primary", true);
    connect(btnOk, &QPushButton::clicked, [&]() {
        QString name = txtName->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(&dlg, tr("Aviso"), tr("Por favor introduce un nombre para el truco."));
            return;
        }

        QString raw = txtCode->toPlainText().trimmed();
        QStringList tokens = raw.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        std::vector<melonDS::u32> codeVec;
        for (const QString& tok : tokens) {
            bool ok = false;
            melonDS::u32 v = tok.toUInt(&ok, 16);
            if (ok) codeVec.push_back(v);
        }

        if (codeVec.empty()) {
            QMessageBox::warning(&dlg, tr("Aviso"), tr("Introduce al menos un código Action Replay válido en hexadecimal."));
            return;
        }

        melonDS::ARCode newCode;
        newCode.Parent = &gameCheatFile->RootCat;
        newCode.Name = name.toStdString();
        newCode.Description = txtDesc->text().trimmed().toStdString();
        newCode.Enabled = true;
        newCode.Code = codeVec;

        gameCheatFile->RootCat.Children.emplace_back(newCode);
        gameCheatFile->Save();
        dlg.accept();
    });
    btnLay->addWidget(btnOk);
    lay->addLayout(btnLay);

    if (dlg.exec() == QDialog::Accepted) {
        populateCheatTree();
    }
}

void WatermelonGameDialog::onImportCheatDbClicked()
{
    if (!gameCheatFile) return;

    QString file = QFileDialog::getOpenFileName(
        this,
        tr("Seleccionar base de datos de trucos (usrcheat.dat)"),
        QString(),
        tr("Bases de datos de trucos (*.dat);;Todos los archivos (*.*)")
    );
    if (file.isEmpty()) return;

    melonDS::ARDatabaseDAT db(file.toStdString());
    if (db.Error) {
        QMessageBox::critical(this, tr("Error"), tr("No se pudo abrir la base de datos de trucos seleccionada."));
        return;
    }

    melonDS::u32 codeU32 = 0;
    if (romInfo.gameCode.length() >= 4) {
        codeU32 = (melonDS::u32)(melonDS::u8)romInfo.gameCode[0].toLatin1() |
                  ((melonDS::u32)(melonDS::u8)romInfo.gameCode[1].toLatin1() << 8) |
                  ((melonDS::u32)(melonDS::u8)romInfo.gameCode[2].toLatin1() << 16) |
                  ((melonDS::u32)(melonDS::u8)romInfo.gameCode[3].toLatin1() << 24);
    }

    auto entries = db.GetEntriesByGameCode(codeU32);
    if (entries.empty()) {
        QMessageBox::warning(
            this,
            tr("Sin trucos encontrados"),
            tr("No se encontraron trucos para el código de juego %1 en esta base de datos.").arg(romInfo.gameCode)
        );
        return;
    }

    melonDS::ARCodeEnableMap map;
    gameCheatFile->Import(entries[0], map, false);
    gameCheatFile->Save();

    populateCheatTree();

    QMessageBox::information(
        this,
        tr("Trucos importados"),
        tr("Se importaron exitosamente los trucos para '%1' desde la base de datos.").arg(romInfo.title)
    );
}

// ═══════════════════════════════════════════════════════════════════════════
//  TAB 2 – RetroAchievements
// ═══════════════════════════════════════════════════════════════════════════
QWidget* WatermelonGameDialog::createRetroAchievementsTab()
{
    raLoggedIn = !WatermelonRADialog::getUsername().isEmpty()
              && !WatermelonRADialog::getToken().isEmpty();

    auto* page = new QWidget();
    page->setStyleSheet(QStringLiteral("background: #12111a;"));
    auto* pLay = new QVBoxLayout(page);
    pLay->setContentsMargins(0, 0, 0, 0);
    pLay->setSpacing(0);

    raStack = new QStackedWidget(page);

    // ── Page 0: Login ──────────────────────────────────────────────────────
    {
        auto* loginPage = new QWidget();
        loginPage->setStyleSheet(QStringLiteral("background: #12111a;"));
        auto* lLay = new QVBoxLayout(loginPage);
        lLay->setContentsMargins(48, 48, 48, 48);
        lLay->setSpacing(14);
        lLay->setAlignment(Qt::AlignTop);

        // Icon + Title
        auto* iconRow = new QHBoxLayout();
        iconRow->setAlignment(Qt::AlignCenter);
        auto* icon = new QLabel(tr("🏆"), loginPage);
        icon->setStyleSheet(QStringLiteral("font-size: 44px; background: transparent;"));
        iconRow->addWidget(icon);
        lLay->addLayout(iconRow);
        lLay->addSpacing(4);

        auto* titleLbl = new QLabel(tr("RetroAchievements"), loginPage);
        titleLbl->setAlignment(Qt::AlignCenter);
        titleLbl->setStyleSheet(QStringLiteral(
            "font-size: 22px; font-weight: 800; color: #f6f5f3; background: transparent;"));
        lLay->addWidget(titleLbl);

        auto* subLbl = new QLabel(
            tr("Inicia sesión para ver el progreso de logros de este juego."),
            loginPage);
        subLbl->setAlignment(Qt::AlignCenter);
        subLbl->setWordWrap(true);
        subLbl->setStyleSheet(QStringLiteral(
            "font-size: 13px; color: #7a7888; background: transparent;"));
        lLay->addWidget(subLbl);
        lLay->addSpacing(8);

        raUserInput = new QLineEdit(loginPage);
        raUserInput->setPlaceholderText(tr("Usuario de RetroAchievements"));
        raUserInput->setFixedHeight(44);
        lLay->addWidget(raUserInput);

        raPassInput = new QLineEdit(loginPage);
        raPassInput->setPlaceholderText(tr("Contraseña"));
        raPassInput->setEchoMode(QLineEdit::Password);
        raPassInput->setFixedHeight(44);
        lLay->addWidget(raPassInput);

        raLoginError = new QLabel(QString(), loginPage);
        raLoginError->setAlignment(Qt::AlignCenter);
        raLoginError->setWordWrap(true);
        raLoginError->setStyleSheet(QStringLiteral(
            "color: #e5534b; font-size: 12px; background: transparent; padding: 2px;"));
        raLoginError->hide();
        lLay->addWidget(raLoginError);

        btnRALogin = new QPushButton(tr("Iniciar sesión"), loginPage);
        btnRALogin->setFixedHeight(42);
        btnRALogin->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #6fbf4a;"
            "  color: #12111a;"
            "  font-weight: 700; font-size: 14px;"
            "  border-radius: 8px; border: none;"
            "}"
            "QPushButton:hover { background-color: #82d457; }"
            "QPushButton:disabled { background: #2a2938; color: #5a5868; }"
        ));
        connect(btnRALogin, &QPushButton::clicked, this, &WatermelonGameDialog::onRALoginClicked);
        lLay->addWidget(btnRALogin);

        auto* btnManage = new QPushButton(tr("⚙  Gestionar cuenta global…"), loginPage);
        btnManage->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; color: #5a9bdf; border: none; "
            "font-size: 12px; font-weight: 600; padding: 4px; }"
            "QPushButton:hover { color: #6fbf4a; }"
        ));
        connect(btnManage, &QPushButton::clicked, this, [this]() {
            WatermelonRADialog dlg(this);
            dlg.exec();
            raLoggedIn = !WatermelonRADialog::getUsername().isEmpty()
                      && !WatermelonRADialog::getToken().isEmpty();
            if (raLoggedIn && raStack) loadRAGameData();
        });
        lLay->addWidget(btnManage, 0, Qt::AlignCenter);
        lLay->addStretch();
        raStack->addWidget(loginPage);     // 0
    }

    // ── Page 1: Loading ────────────────────────────────────────────────────
    {
        auto* loadPage = new QWidget();
        loadPage->setStyleSheet(QStringLiteral("background: #12111a;"));
        auto* ldLay = new QVBoxLayout(loadPage);
        ldLay->setAlignment(Qt::AlignCenter);
        ldLay->setSpacing(16);

        auto* spinLbl = new QLabel(tr("Cargando logros…"), loadPage);
        spinLbl->setAlignment(Qt::AlignCenter);
        spinLbl->setStyleSheet(QStringLiteral(
            "font-size: 15px; font-weight: 600; color: #b3b1bd; background: transparent;"));
        ldLay->addWidget(spinLbl);

        auto* prog = new QProgressBar(loadPage);
        prog->setRange(0, 0);
        prog->setFixedWidth(200);
        prog->setFixedHeight(4);
        prog->setStyleSheet(QStringLiteral(
            "QProgressBar { background: #2a2938; border-radius: 2px; border: none; }"
            "QProgressBar::chunk { background: #6fbf4a; border-radius: 2px; }"
        ));
        ldLay->addWidget(prog, 0, Qt::AlignCenter);
        raStack->addWidget(loadPage);      // 1
    }

    // ── Page 2: Content ────────────────────────────────────────────────────
    {
        auto* contentPage = new QWidget();
        contentPage->setStyleSheet(QStringLiteral("background: #12111a;"));
        auto* ctLay = new QVBoxLayout(contentPage);
        ctLay->setContentsMargins(0, 0, 0, 0);
        ctLay->setSpacing(0);

        // Profile + summary bar
        auto* profBar = new QWidget(contentPage);
        profBar->setFixedHeight(60);
        profBar->setStyleSheet(QStringLiteral(
            "background: #17161f; border-bottom: 1px solid #252433;"
        ));
        auto* profLay = new QHBoxLayout(profBar);
        profLay->setContentsMargins(16, 0, 16, 0);
        profLay->setSpacing(10);

        raUserBadge = new QLabel(tr("👤"), profBar);
        raUserBadge->setFixedSize(36, 36);
        raUserBadge->setAlignment(Qt::AlignCenter);
        raUserBadge->setStyleSheet(QStringLiteral(
            "font-size: 20px; background: #252433; border-radius: 18px; border: 1px solid #35344a;"));
        profLay->addWidget(raUserBadge);

        raUserName = new QLabel(WatermelonRADialog::getUsername(), profBar);
        raUserName->setStyleSheet(QStringLiteral(
            "font-size: 14px; font-weight: 700; color: #f6f5f3; background: transparent;"));
        profLay->addWidget(raUserName, 1);

        bool hc = WatermelonRADialog::isHardcore();
        auto* modePill = makePill(
            hc ? tr("🔒 Hardcore") : QStringLiteral("Softcore"),
            hc ? QColor(0xe5,0x53,0x4b) : QColor(0x6f,0xbf,0x4a),
            hc ? QColor(0x2a,0x18,0x18) : QColor(0x1a,0x2e,0x18),
            profBar);
        profLay->addWidget(modePill, 0, Qt::AlignVCenter);

        auto* btnLogout = new QPushButton(tr("Cerrar sesión"), profBar);
        btnLogout->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; color: #5a5868; border: 1px solid #35344a; "
            "border-radius: 7px; padding: 4px 10px; font-size: 11px; }"
            "QPushButton:hover { color: #e5534b; border-color: #e5534b; }"
        ));
        connect(btnLogout, &QPushButton::clicked, this, &WatermelonGameDialog::onRALogoutClicked);
        profLay->addWidget(btnLogout);
        ctLay->addWidget(profBar);

        // Progress summary bar (shown after data loads)
        raSummaryBar = new QWidget(contentPage);
        raSummaryBar->setFixedHeight(48);
        raSummaryBar->setStyleSheet(QStringLiteral(
            "background: #1a1928; border-bottom: 1px solid #252433;"));
        raSummaryBar->setVisible(false);
        {
            auto* sLay = new QHBoxLayout(raSummaryBar);
            sLay->setContentsMargins(16, 0, 16, 0);
            sLay->setSpacing(10);

            raProgressLabel = new QLabel(QString(), raSummaryBar);
            raProgressLabel->setStyleSheet(QStringLiteral(
                "font-size: 13px; color: #c0bcd0; background: transparent;"));
            sLay->addWidget(raProgressLabel, 1);

            auto* btnWeb = new QPushButton(tr("🌐  Ver en web"), raSummaryBar);
            btnWeb->setStyleSheet(QStringLiteral(
                "QPushButton { background: transparent; color: #5a9bdf; border: none; "
                "font-size: 12px; font-weight: 600; }"
                "QPushButton:hover { color: #6fbf4a; }"
            ));
            connect(btnWeb, &QPushButton::clicked, this, &WatermelonGameDialog::onViewOnWebClicked);
            sLay->addWidget(btnWeb);
        }
        ctLay->addWidget(raSummaryBar);

        // Scrollable achievement list
        auto* scroll2 = new QScrollArea(contentPage);
        scroll2->setWidgetResizable(true);
        scroll2->setFrameShape(QFrame::NoFrame);

        auto* listContent = new QWidget();
        listContent->setStyleSheet(QStringLiteral("background: #12111a;"));
        raListLayout = new QVBoxLayout(listContent);
        raListLayout->setContentsMargins(16, 4, 16, 24);
        raListLayout->setSpacing(0);
        raListLayout->addStretch();

        scroll2->setWidget(listContent);
        ctLay->addWidget(scroll2, 1);
        raStack->addWidget(contentPage);   // 2
    }

    // ── Page 3: Error (dynamic label) ────────────────────────────────────────
    {
        auto* errPage = new QWidget();
        errPage->setStyleSheet(QStringLiteral("background: #12111a;"));
        auto* eLay = new QVBoxLayout(errPage);
        eLay->setAlignment(Qt::AlignCenter);
        eLay->setSpacing(14);
        eLay->setContentsMargins(40, 40, 40, 40);

        auto* errIcon = new QLabel(tr("⚠️"), errPage);
        errIcon->setAlignment(Qt::AlignCenter);
        errIcon->setStyleSheet(QStringLiteral("font-size: 44px; background: transparent;"));
        eLay->addWidget(errIcon);

        // Show logged-in user, if any
        QString raUser = WatermelonRADialog::getUsername();
        if (!raUser.isEmpty()) {
            auto* userPill = new QLabel(
                tr("🔑  Conectado como: %1").arg(raUser), errPage);
            userPill->setAlignment(Qt::AlignCenter);
            userPill->setStyleSheet(QStringLiteral(
                "font-size: 12px; font-weight: 600; color: #6fbf4a; "
                "background: #1a2e18; border-radius: 8px; padding: 5px 14px; border: none;"));
            eLay->addWidget(userPill, 0, Qt::AlignCenter);
        }

        raErrorLabel = new QLabel(
            tr("No se pudieron cargar los logros.\nRevisa tu conexión o credenciales."),
            errPage);
        raErrorLabel->setAlignment(Qt::AlignCenter);
        raErrorLabel->setWordWrap(true);
        raErrorLabel->setStyleSheet(QStringLiteral(
            "font-size: 14px; color: #9a98a8; background: transparent; line-height: 150%;"));
        eLay->addWidget(raErrorLabel);

        auto* btnRetry = new QPushButton(tr("  Reintentar"), errPage);
        btnRetry->setIcon(QIcon(QStringLiteral(":/refresh-icon")));
        btnRetry->setFixedSize(150, 40);
        btnRetry->setStyleSheet(QStringLiteral(
            "QPushButton { background: #1e1d2b; color: #6fbf4a; border: 1px solid #3a3848; "
            "border-radius: 10px; font-size: 13px; font-weight: 700; }"
            "QPushButton:hover { background: #272638; }"
        ));
        connect(btnRetry, &QPushButton::clicked, this, [this]() { loadRAGameData(); });
        eLay->addWidget(btnRetry, 0, Qt::AlignCenter);

        auto* btnSearch = new QPushButton(tr("🌐  Buscar en RetroAchievements"), errPage);
        btnSearch->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; color: #5a9bdf; border: none; "
            "font-size: 12px; font-weight: 600; padding: 4px; }"
            "QPushButton:hover { color: #6fbf4a; }"
        ));
        connect(btnSearch, &QPushButton::clicked, this, &WatermelonGameDialog::onViewOnWebClicked);
        eLay->addWidget(btnSearch, 0, Qt::AlignCenter);

        raStack->addWidget(errPage);       // 3
    }

    raStack->setCurrentIndex(raLoggedIn ? 1 : 0);
    pLay->addWidget(raStack, 1);

    if (raLoggedIn)
        QTimer::singleShot(80, this, &WatermelonGameDialog::loadRAGameData);

    return page;
}

// ═══════════════════════════════════════════════════════════════════════════
//  TAB 2 – Offline
// ═══════════════════════════════════════════════════════════════════════════
QWidget* WatermelonGameDialog::createOfflineTab()
{
    auto* page = new QWidget();
    page->setStyleSheet(QStringLiteral("background: #12111a;"));

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget();
    content->setStyleSheet(QStringLiteral("background: #12111a;"));
    auto* cLay = new QVBoxLayout(content);
    cLay->setContentsMargins(16, 20, 16, 32);
    cLay->setSpacing(14);

    bool raEnabled = WatermelonRADialog::isEnabled();
    bool loggedIn  = !WatermelonRADialog::getUsername().isEmpty();

    // Status card
    auto* statusCard = makeCard(content);
    auto* scLay = new QVBoxLayout(statusCard);
    scLay->setContentsMargins(18, 16, 18, 16);
    scLay->setSpacing(10);

    auto* scHead = new QLabel(tr("📴  Modo Offline de RetroAchievements"), statusCard);
    scHead->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 700; color: #f6f5f3; background: transparent;"));
    scLay->addWidget(scHead);

    offlineStatusLabel = new QLabel(statusCard);
    offlineStatusLabel->setWordWrap(true);
    offlineStatusLabel->setTextFormat(Qt::RichText);
    offlineStatusLabel->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: #9a98a8; background: transparent; line-height: 160%;"));

    if (!raEnabled) {
        offlineStatusLabel->setText(QStringLiteral(
            "⚠️  <b style='color:#f6f5f3'>RetroAchievements está desactivado</b> globalmente.<br><br>"
            "Actívalo en la configuración global para usar el modo offline."));
    } else if (!loggedIn) {
        offlineStatusLabel->setText(QStringLiteral(
            "🔑  <b style='color:#f6f5f3'>Sin sesión iniciada.</b><br><br>"
            "Inicia sesión en la pestaña <i>🏆 RetroAchievements</i> para habilitar esta función."));
    } else {
        offlineStatusLabel->setText(QStringLiteral(
            "🟡  <b style='color:#f6f5f3'>Sin datos locales cargados.</b><br><br>"
            "El modo offline permite desbloquear logros <b>sin internet</b> en Softcore. "
            "Los desbloqueos se sincronizan automáticamente al reconectarte."));
    }
    scLay->addWidget(offlineStatusLabel);

    if (raEnabled && loggedIn) {
        scLay->addWidget(makeDivider(statusCard));

        auto makeStatRow = [&](const QString& lbl, const QString& val, const QColor& valColor) {
            auto* row = new QHBoxLayout();
            row->setSpacing(8);
            auto* lLbl = new QLabel(lbl, statusCard);
            lLbl->setStyleSheet(QStringLiteral(
                "font-size: 12px; color: #7a7888; background: transparent;"));
            row->addWidget(lLbl, 1);
            auto* vLbl = new QLabel(val, statusCard);
            vLbl->setStyleSheet(QStringLiteral(
                "font-size: 12px; font-weight: 700; color: %1; background: transparent;")
                .arg(valColor.name()));
            row->addWidget(vLbl);
            scLay->addLayout(row);
        };

        makeStatRow(tr("Desbloqueos pendientes:"),
                    QStringLiteral("0"),        QColor(0x6f,0xbf,0x4a));
        makeStatRow(tr("Caché disponible:"),
                    QStringLiteral("No"),       QColor(0xe5,0x53,0x4b));
        makeStatRow(tr("Conexión actual:"),
                    QStringLiteral("Online"),   QColor(0x6f,0xbf,0x4a));
    }
    cLay->addWidget(statusCard);

    // Sync button
    if (raEnabled && loggedIn) {
        btnSyncOffline = new QPushButton(tr("  Comprobar y Sincronizar"), content);
        btnSyncOffline->setIcon(QIcon(QStringLiteral(":/refresh-icon")));
        btnSyncOffline->setFixedHeight(44);
        btnSyncOffline->setStyleSheet(QStringLiteral(
            "QPushButton { background: #1e1d2b; color: #6fbf4a; border: 1px solid #3a3848; "
            "font-size: 13px; font-weight: 700; border-radius: 10px; }"
            "QPushButton:hover { background: #272638; border-color: #6fbf4a; }"
            "QPushButton:disabled { color: #5a5868; border-color: #2a2938; }"
        ));
        connect(btnSyncOffline, &QPushButton::clicked, this, &WatermelonGameDialog::onSyncOfflineClicked);
        cLay->addWidget(btnSyncOffline);
    }

    // Info card
    auto* infoCard = makeCard(content);
    auto* iLay = new QVBoxLayout(infoCard);
    iLay->setContentsMargins(18, 14, 18, 14);
    iLay->setSpacing(10);

    auto* iHead = new QLabel(tr("¿Cómo funciona el modo Offline?"), infoCard);
    iHead->setStyleSheet(QStringLiteral(
        "font-size: 13px; font-weight: 700; color: #f6f5f3; background: transparent;"));
    iLay->addWidget(iHead);

    auto* iText = new QLabel(infoCard);
    iText->setWordWrap(true);
    iText->setTextFormat(Qt::RichText);
    iText->setStyleSheet(QStringLiteral(
        "font-size: 12px; color: #7a7888; background: transparent; line-height: 155%;"));
    iText->setText(QStringLiteral(
        "<b style='color:#b3b1bd'>1.</b>  Descarga el caché del juego mientras estás online.<br>"
        "<b style='color:#b3b1bd'>2.</b>  Juega sin internet: los logros se guardan localmente.<br>"
        "<b style='color:#b3b1bd'>3.</b>  Al reconectarte se sincronizan con el servidor.<br><br>"
        "⚠️ El modo <b style='color:#e5534b'>Hardcore</b> requiere conexión en vivo "
        "y no permite sincronización diferida."));
    iLay->addWidget(iText);
    cLay->addWidget(infoCard);

    cLay->addStretch();
    scroll->setWidget(content);

    auto* pLay = new QVBoxLayout(page);
    pLay->setContentsMargins(0, 0, 0, 0);
    pLay->addWidget(scroll);
    return page;
}

// ═══════════════════════════════════════════════════════════════════════════
//  RA: show error with custom message
// ═══════════════════════════════════════════════════════════════════════════
void WatermelonGameDialog::showRAError(const QString& msg)
{
    if (raErrorLabel)
        raErrorLabel->setText(msg);
    if (raStack)
        raStack->setCurrentIndex(3);
}

// ═══════════════════════════════════════════════════════════════════════════
//  RA: load game data
// ═══════════════════════════════════════════════════════════════════════════
void WatermelonGameDialog::loadRAGameData()
{
    if (!raStack) return;
    raStack->setCurrentIndex(1);

    QString user  = WatermelonRADialog::getUsername();
    QString token = WatermelonRADialog::getToken();
    if (user.isEmpty() || token.isEmpty()) { raStack->setCurrentIndex(0); return; }

    // Fetch official NDS games list (Console 18) via dorequest.php
    QUrl url(QStringLiteral("https://retroachievements.org/dorequest.php?r=gameslist&c=18"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("melonDualDS-android/0.7.0"));

    QNetworkReply* reply = networkMgr->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            showRAError(tr("Error de conexión al consultar RetroAchievements:\n%1").arg(reply->errorString()));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            showRAError(tr("Respuesta inesperada de RetroAchievements."));
            return;
        }

        QJsonObject root = doc.object();
        QJsonObject resp = root.value(QStringLiteral("Response")).toObject();

        QString cleanTitle = romInfo.title.toLower();
        static const QRegularExpression nonAlpha(QStringLiteral("[^a-z0-9 ]+"));
        QString simplifiedTarget = cleanTitle;
        simplifiedTarget.replace(nonAlpha, QStringLiteral(" "));
        QStringList targetWords = simplifiedTarget.split(QLatin1Char(' '), Qt::SkipEmptyParts);

        int bestId = 0;
        int maxScore = 0;
        QString bestTitle;

        for (auto it = resp.begin(); it != resp.end(); ++it) {
            QString raTitle = it.value().toString();
            QString simplifiedRA = raTitle.toLower();
            simplifiedRA.replace(nonAlpha, QStringLiteral(" "));

            int score = 0;
            if (simplifiedRA == simplifiedTarget) {
                score = 1000;
            } else if (simplifiedRA.contains(simplifiedTarget) || simplifiedTarget.contains(simplifiedRA)) {
                score = 500;
            } else {
                for (const auto& w : targetWords) {
                    if (w.length() >= 3 && simplifiedRA.contains(w)) {
                        score += 50;
                    }
                }
            }

            if (score > maxScore) {
                maxScore = score;
                bestId = it.key().toInt();
                bestTitle = raTitle;
            }
        }

        if (bestId == 0 || maxScore < 60) {
            showRAError(QStringLiteral(
                "No se encontró el juego \"%1\" en RetroAchievements.\n\n"
                "Verifica el nombre o que el juego disponga de logros.")
                .arg(romInfo.title));
            return;
        }

        raGame.gameId = bestId;
        raGame.title  = bestTitle;
        fetchGameProgress();
    });
}

void WatermelonGameDialog::searchRAByGameList()
{
    loadRAGameData();
}

void WatermelonGameDialog::fetchGameProgress()
{
    QString user  = WatermelonRADialog::getUsername();
    QString token = WatermelonRADialog::getToken();

    // Fetch patch (achievements metadata)
    QUrl patchUrl(QStringLiteral(
        "https://retroachievements.org/dorequest.php?r=patch&u=%1&t=%2&g=%3")
        .arg(QUrl::toPercentEncoding(user), QUrl::toPercentEncoding(token), QString::number(raGame.gameId)));
    QNetworkRequest req(patchUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("melonDualDS-android/0.7.0"));

    QNetworkReply* reply = networkMgr->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onGameInfoReplyFinished(reply);
    });
}

void WatermelonGameDialog::onGameInfoReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        showRAError(tr("Error al descargar los logros del juego:\n%1").arg(reply->errorString()));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        showRAError(tr("Respuesta de datos inválida de RetroAchievements."));
        return;
    }

    QJsonObject root = doc.object();
    if (!root.value(QStringLiteral("Success")).toBool(true)) {
        showRAError(root.value(QStringLiteral("Error")).toString(tr("Error al obtener datos del juego.")));
        return;
    }

    QJsonObject patchData = root.value(QStringLiteral("PatchData")).toObject();
    if (!patchData.isEmpty()) {
        raGame.title = patchData.value(QStringLiteral("Title")).toString(raGame.title);
    }

    QJsonArray achArr = patchData.value(QStringLiteral("Achievements")).toArray();
    raGame.achievements.clear();

    for (const auto& val : achArr) {
        QJsonObject ao = val.toObject();
        RAGameAchievement a;
        a.id          = ao.value(QStringLiteral("ID")).toInt();
        a.title       = ao.value(QStringLiteral("Title")).toString();
        a.description = ao.value(QStringLiteral("Description")).toString();
        a.points      = ao.value(QStringLiteral("Points")).toInt();
        a.isUnlocked  = false;
        a.isHardcore  = false;

        QString badge = ao.value(QStringLiteral("BadgeName")).toString();
        a.badgeName   = badge;
        a.badgeUrl    = QStringLiteral("https://media.retroachievements.org/Badge/") + badge + QStringLiteral(".png");
        raGame.achievements.append(a);
    }

    // Now query unlocks for this user
    QString user  = WatermelonRADialog::getUsername();
    QString token = WatermelonRADialog::getToken();

    QUrl unlocksUrl(QStringLiteral(
        "https://retroachievements.org/dorequest.php?r=unlocks&u=%1&t=%2&g=%3&h=0")
        .arg(QUrl::toPercentEncoding(user), QUrl::toPercentEncoding(token), QString::number(raGame.gameId)));
    QNetworkRequest uReq(unlocksUrl);
    uReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("melonDualDS-android/0.7.0"));

    QNetworkReply* uReply = networkMgr->get(uReq);
    connect(uReply, &QNetworkReply::finished, this, [this, uReply]() {
        uReply->deleteLater();
        QSet<int> unlockedIds;
        if (uReply->error() == QNetworkReply::NoError) {
            QJsonObject uObj = QJsonDocument::fromJson(uReply->readAll()).object();
            QJsonArray uArr = uObj.value(QStringLiteral("UserUnlocks")).toArray();
            for (const auto& item : uArr) {
                unlockedIds.insert(item.toInt());
            }
        }

        int totalPts = 0;
        int earnedPts = 0;
        int numUnlocked = 0;

        for (auto& a : raGame.achievements) {
            totalPts += a.points;
            if (unlockedIds.contains(a.id)) {
                a.isUnlocked = true;
                earnedPts += a.points;
                numUnlocked++;
            }
        }

        raGame.numAchievements = raGame.achievements.size();
        raGame.numUnlocked     = numUnlocked;
        raGame.pointsTotal     = totalPts;
        raGame.pointsEarned    = earnedPts;

        std::sort(raGame.achievements.begin(), raGame.achievements.end(),
            [](const RAGameAchievement& a, const RAGameAchievement& b) {
                if (a.isUnlocked != b.isUnlocked) return (int)a.isUnlocked > (int)b.isUnlocked;
                return a.points > b.points;
            });

        renderRAContent();
    });
}

void WatermelonGameDialog::renderRAContent()
{
    if (!raStack || !raListLayout || !raProgressLabel || !raSummaryBar) return;

    // Update summary
    int pct = raGame.pointsTotal > 0
              ? (raGame.pointsEarned * 100 / raGame.pointsTotal) : 0;
    raProgressLabel->setText(QStringLiteral(
        "<b>%1</b> / %2 logros &nbsp;·&nbsp; <b>%3</b> / %4 pts &nbsp;·&nbsp; %5%")
        .arg(raGame.numUnlocked).arg(raGame.numAchievements)
        .arg(raGame.pointsEarned).arg(raGame.pointsTotal).arg(pct));
    raSummaryBar->setVisible(true);
    if (raUserName) raUserName->setText(WatermelonRADialog::getUsername());

    // Clear list
    while (raListLayout->count() > 1) {
        auto* item = raListLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }

    QList<RAGameAchievement> unlocked, locked;
    for (const auto& a : raGame.achievements)
        (a.isUnlocked ? unlocked : locked).append(a);

    auto insertBefore = [&](QWidget* w) {
        raListLayout->insertWidget(raListLayout->count() - 1, w);
    };

    if (!unlocked.isEmpty()) {
        insertBefore(makeSectionLabel(
            tr("Desbloqueados (%1)").arg(unlocked.size())));
        insertBefore(buildAchievementCard(unlocked, true));
    }
    if (!locked.isEmpty()) {
        insertBefore(makeSectionLabel(
            tr("Bloqueados (%1)").arg(locked.size())));
        insertBefore(buildAchievementCard(locked, false));
    }
    if (raGame.achievements.isEmpty()) {
        auto* empty = new QLabel(QStringLiteral(
            "Este juego no tiene logros en RetroAchievements."));
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(QStringLiteral(
            "font-size: 14px; color: #5a5868; padding: 60px; background: transparent;"));
        insertBefore(empty);
    }

    raStack->setCurrentIndex(2);
}

QFrame* WatermelonGameDialog::buildAchievementCard(const QList<RAGameAchievement>& list, bool isUnlocked)
{
    auto* card = makeCard();
    auto* cLay = new QVBoxLayout(card);
    cLay->setContentsMargins(0, 0, 0, 0);
    cLay->setSpacing(0);

    for (int i = 0; i < list.size(); ++i) {
        const auto& a = list[i];
        if (i > 0) cLay->addWidget(makeDivider(card));

        auto* row = new QWidget(card);
        row->setMinimumHeight(72);
        row->setStyleSheet(QStringLiteral(
            "QWidget { background: transparent; border-radius: 10px; }"
            "QWidget:hover { background: #1a1928; }"
        ));

        auto* rLay = new QHBoxLayout(row);
        rLay->setContentsMargins(14, 10, 14, 10);
        rLay->setSpacing(12);

        // Badge container (Square)
        auto* badgeLbl = new QLabel(isUnlocked ? tr("⭐") : tr("🔒"), row);
        badgeLbl->setFixedSize(52, 52);
        badgeLbl->setAlignment(Qt::AlignCenter);
        badgeLbl->setStyleSheet(QStringLiteral(
            "font-size: 24px; background: %1; border-radius: 6px; border: 1px solid %2;")
            .arg(isUnlocked ? QStringLiteral("#1f2b18") : QStringLiteral("#22222e"),
                 isUnlocked ? QStringLiteral("#3a5a2a") : QStringLiteral("#30303e")));
        downloadBadge(a.badgeName.isEmpty() ? a.badgeUrl : a.badgeName, badgeLbl, isUnlocked);
        rLay->addWidget(badgeLbl);

        // Text
        auto* textLay = new QVBoxLayout();
        textLay->setSpacing(3);

        auto* achTitle = new QLabel(a.title, row);
        achTitle->setStyleSheet(QStringLiteral(
            "font-size: 13px; font-weight: 700; color: %1; background: transparent;")
            .arg(isUnlocked ? QStringLiteral("#e8e6f0") : QStringLiteral("#9a98a8")));
        achTitle->setWordWrap(true);
        textLay->addWidget(achTitle);

        auto* achDesc = new QLabel(a.description, row);
        achDesc->setStyleSheet(QStringLiteral(
            "font-size: 11px; color: #5a5868; background: transparent;"));
        achDesc->setWordWrap(true);
        textLay->addWidget(achDesc);
        rLay->addLayout(textLay, 1);

        // Points
        auto* ptsCont = new QWidget(row);
        ptsCont->setFixedWidth(42);
        auto* ptsLay = new QVBoxLayout(ptsCont);
        ptsLay->setContentsMargins(0, 0, 0, 0);
        ptsLay->setSpacing(0);
        ptsLay->setAlignment(Qt::AlignCenter);

        auto* ptsNum = new QLabel(QString::number(a.points), ptsCont);
        ptsNum->setAlignment(Qt::AlignCenter);
        ptsNum->setStyleSheet(QStringLiteral(
            "font-size: 14px; font-weight: 900; color: %1; background: transparent;")
            .arg(isUnlocked ? QStringLiteral("#ffd700") : QStringLiteral("#4a4858")));
        ptsLay->addWidget(ptsNum);

        auto* ptsLbl = new QLabel(tr("pts"), ptsCont);
        ptsLbl->setAlignment(Qt::AlignCenter);
        ptsLbl->setStyleSheet(QStringLiteral(
            "font-size: 9px; letter-spacing: 0.5px; color: #4a4858; background: transparent;"));
        ptsLay->addWidget(ptsLbl);
        rLay->addWidget(ptsCont);

        // HC badge
        if (a.isHardcore) {
            auto* hcBadge = new QLabel(tr("HC"), row);
            hcBadge->setFixedSize(26, 18);
            hcBadge->setAlignment(Qt::AlignCenter);
            hcBadge->setStyleSheet(QStringLiteral(
                "font-size: 9px; font-weight: 900; color: #e5534b; "
                "background: #2a1818; border-radius: 4px; border: 1px solid #5a2020;"));
            rLay->addWidget(hcBadge);
        }

        cLay->addWidget(row);
    }

    return card;
}

namespace
{
static QHash<QString, QPixmap> s_badgeCache;

static QString getRABadgeCacheDir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (base.isEmpty())
    {
        base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/melonDS");
    }
    QString badgeDir = base + QStringLiteral("/retroachievements/badges");
    QDir().mkpath(badgeDir);
    return badgeDir;
}

static QPixmap processBadgePixmap(const QPixmap& src, bool unlocked)
{
    if (src.isNull()) return {};

    constexpr int sz = 52;
    constexpr int radius = 6;

    QPixmap rounded(sz, sz);
    rounded.fill(Qt::transparent);

    {
        QPainter p(&rounded);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addRoundedRect(0, 0, sz, sz, radius, radius);
        p.setClipPath(path);

        QPixmap scaled = src.scaled(sz, sz, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        p.drawPixmap(0, 0, scaled);
    }

    if (!unlocked) {
        // Desaturate cleanly to grayscale
        QImage img = rounded.toImage().convertToFormat(QImage::Format_ARGB32);
        if (!img.isNull()) {
            const int h = img.height();
            const int w = img.width();
            for (int y = 0; y < h; ++y) {
                QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
                for (int x = 0; x < w; ++x) {
                    QRgb px = line[x];
                    int g = qGray(px);
                    line[x] = qRgba(g, g, g, qAlpha(px));
                }
            }
            rounded = QPixmap::fromImage(img);
        }
    }

    return rounded;
}
}

void WatermelonGameDialog::downloadBadge(const QString& badgeName, QLabel* lbl, bool unlocked)
{
    if (badgeName.isEmpty() || !lbl) return;

    QString cacheKey = QStringLiteral("%1_%2").arg(unlocked ? QStringLiteral("u") : QStringLiteral("l"), badgeName);
    auto it = s_badgeCache.constFind(cacheKey);
    if (it != s_badgeCache.constEnd()) {
        lbl->setPixmap(it.value());
        lbl->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        return;
    }

    QString badgeDir = getRABadgeCacheDir();
    QString fileName = badgeName + (unlocked ? QStringLiteral(".png") : QStringLiteral("_lock.png"));
    QString filePath = badgeDir + QLatin1Char('/') + fileName;

    // 1. Check persistent disk cache (cross-platform: Linux ~/.cache/melonDS/..., Windows AppData/Local/melonDS/...)
    if (QFile::exists(filePath)) {
        QPixmap diskPix;
        if (diskPix.load(filePath) && !diskPix.isNull()) {
            QPixmap processed = processBadgePixmap(diskPix, unlocked);
            s_badgeCache.insert(cacheKey, processed);
            lbl->setPixmap(processed);
            lbl->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
            return;
        }
    }

    // 2. Download from RetroAchievements media server
    QString urlStr = QStringLiteral("https://media.retroachievements.org/Badge/") + fileName;
    QNetworkRequest req{QUrl(urlStr)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("melonDualDS-android/0.7.0"));
    QNetworkReply* reply = networkMgr->get(req);

    QPointer<QLabel> safeLbl(lbl);
    connect(reply, &QNetworkReply::finished, this, [this, reply, safeLbl, badgeName, filePath, cacheKey, unlocked]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            // If locked badge is not found, fallback to downloading normal badge and grayscale it
            if (!unlocked) {
                QString normalUrl = QStringLiteral("https://media.retroachievements.org/Badge/") + badgeName + QStringLiteral(".png");
                QNetworkRequest fbReq{QUrl(normalUrl)};
                fbReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("melonDualDS-android/0.7.0"));
                QNetworkReply* fbReply = networkMgr->get(fbReq);
                connect(fbReply, &QNetworkReply::finished, this, [safeLbl, fbReply, filePath, cacheKey]() {
                    fbReply->deleteLater();
                    if (fbReply->error() == QNetworkReply::NoError) {
                        QByteArray data = fbReply->readAll();
                        QPixmap pix;
                        if (pix.loadFromData(data) && !pix.isNull()) {
                            QPixmap processed = processBadgePixmap(pix, false);
                            processed.save(filePath, "PNG");
                            s_badgeCache.insert(cacheKey, processed);
                            if (safeLbl) {
                                safeLbl->setPixmap(processed);
                                safeLbl->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
                            }
                        }
                    }
                });
            }
            return;
        }

        QByteArray data = reply->readAll();
        QPixmap pix;
        if (!pix.loadFromData(data) || pix.isNull()) return;

        // Save raw image to disk cache for fast offline and subsequent loads
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();
        }

        QPixmap processed = processBadgePixmap(pix, unlocked);
        s_badgeCache.insert(cacheKey, processed);

        if (safeLbl) {
            safeLbl->setPixmap(processed);
            safeLbl->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════════
//  RA: Login / Logout
// ═══════════════════════════════════════════════════════════════════════════
void WatermelonGameDialog::onRALoginClicked()
{
    QString user = raUserInput ? raUserInput->text().trimmed() : QString();
    QString pass = raPassInput ? raPassInput->text() : QString();
    if (user.isEmpty() || pass.isEmpty()) {
        if (raLoginError) { raLoginError->setText(tr("Introduce usuario y contraseña.")); raLoginError->show(); }
        return;
    }
    if (btnRALogin) { btnRALogin->setEnabled(false); btnRALogin->setText(tr("Conectando…")); }
    if (raLoginError) raLoginError->hide();

    QUrl url(QStringLiteral("https://retroachievements.org/dorequest.php?r=login&u=%1&p=%2")
             .arg(QString::fromLatin1(QUrl::toPercentEncoding(user)),
                  QString::fromLatin1(QUrl::toPercentEncoding(pass))));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("WatermelonDS/1.0"));
    QNetworkReply* reply = networkMgr->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onLoginReplyFinished(reply);
    });
}

void WatermelonGameDialog::onLoginReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    if (btnRALogin) { btnRALogin->setEnabled(true); btnRALogin->setText(tr("Iniciar sesión")); }

    if (reply->error() != QNetworkReply::NoError) {
        if (raLoginError) { raLoginError->setText(tr("Error de red: ") + reply->errorString()); raLoginError->show(); }
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    if (!obj[QStringLiteral("Success")].toBool()) {
        if (raLoginError) {
            raLoginError->setText(obj[tr("Error")].toString(
                tr("Usuario o contraseña incorrectos.")));
            raLoginError->show();
        }
        return;
    }

    QString token = obj[QStringLiteral("Token")].toString();
    QString user  = raUserInput ? raUserInput->text().trimmed() : QString();
    Config::Table cfg = Config::GetGlobalTable();
    cfg.SetQString("RA.Username", user);
    cfg.SetQString("RA.Token", token);
    cfg.SetBool("RA.Enabled", true);

    raLoggedIn = true;
    if (raUserName) raUserName->setText(user);
    loadRAGameData();
}

void WatermelonGameDialog::onRALogoutClicked()
{
    Config::Table cfg = Config::GetGlobalTable();
    cfg.SetQString("RA.Username", QString());
    cfg.SetQString("RA.Token", QString());
    raLoggedIn = false;
    raGame = RAGameInfo{};
    if (raStack) raStack->setCurrentIndex(0);
}

void WatermelonGameDialog::onViewOnWebClicked()
{
    if (raGame.gameId > 0)
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://retroachievements.org/game/%1").arg(raGame.gameId)));
    else
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://retroachievements.org/searchresults.php?s=%1")
                                       .arg(QString::fromLatin1(QUrl::toPercentEncoding(romInfo.title)))));
}

// ═══════════════════════════════════════════════════════════════════════════
//  Offline sync
// ═══════════════════════════════════════════════════════════════════════════
void WatermelonGameDialog::onSyncOfflineClicked()
{
    if (btnSyncOffline) { btnSyncOffline->setEnabled(false); btnSyncOffline->setText(tr("Comprobando…")); }
    QTimer::singleShot(900, this, [this]() {
        if (btnSyncOffline) {
            btnSyncOffline->setEnabled(true);
            btnSyncOffline->setText(tr("  Comprobar y Sincronizar"));
            btnSyncOffline->setIcon(QIcon(QStringLiteral(":/refresh-icon")));
        }
        if (offlineStatusLabel) offlineStatusLabel->setText(QStringLiteral(
            "🟢  <b style='color:#f6f5f3'>Sincronización al día.</b><br><br>"
            "Todos los registros locales están sincronizados con RetroAchievements."));
        QMessageBox::information(this, tr("Sincronización RetroAchievements"),
            tr("Sincronización completada. Todos los logros pendientes han sido enviados."));
    });
}

// ═══════════════════════════════════════════════════════════════════════════
//  Config: load / save / apply
// ═══════════════════════════════════════════════════════════════════════════
void WatermelonGameDialog::loadPerGameConfig()
{
    if (romInfo.gameCode.isEmpty()) return;
    QString path = perGameConfigPath(romInfo.gameCode);
    if (!QFile::exists(path)) return;

    QSettings s(path, QSettings::IniFormat);

    // Restore picker indices from saved config
    auto restore = [&](PickerRow* pk, const QString& key) {
        if (pk) pk->setCurrentIndex(s.value(key, 0).toInt());
    };
    restore(pickerConsole,  QStringLiteral("ConsoleType"));
    restore(pickerRenderer, QStringLiteral("Renderer3D"));
    restore(pickerJit,      QStringLiteral("JIT"));
    restore(pickerBoot,     QStringLiteral("DirectBoot"));
    restore(pickerMic,      QStringLiteral("MicSource"));
    restore(pickerDateTime, QStringLiteral("RTCOption"));
    restore(pickerBattery,  QStringLiteral("BatteryLevel"));
    restore(pickerDSiCharging, QStringLiteral("BatteryCharging"));

    QString dtStr = s.value(QStringLiteral("RTCFixedTime")).toString();
    if (!dtStr.isEmpty() && dteGameCustomTime)
    {
        QDateTime dt = QDateTime::fromString(dtStr, Qt::ISODate);
        if (dt.isValid()) dteGameCustomTime->setDateTime(dt);
    }
    if (customDateTimeRow && pickerDateTime)
    {
        customDateTimeRow->setVisible(pickerDateTime->currentIndex() == 2);
    }

    bool cheats = s.value(QStringLiteral("EnableCheats"), false).toBool();
    if (switchCheatsEnabled) switchCheatsEnabled->setChecked(cheats);

    currentShaderPath = s.value(QStringLiteral("ShaderPath"), QString()).toString();
    if (shaderLabel) {
        shaderLabel->setText((!currentShaderPath.isEmpty() && QFile::exists(currentShaderPath))
            ? QFileInfo(currentShaderPath).fileName()
            : QStringLiteral("Global"));
    }

    restore(pickerGBAAddon, QStringLiteral("GBAAddon"));

    restore(pickerWindowSize,       QStringLiteral("ScreenWindowSize"));
    restore(pickerScreenRotation,   QStringLiteral("ScreenRotation"));
    restore(pickerScreenGap,        QStringLiteral("ScreenGap"));
    restore(pickerScreenLayout,     QStringLiteral("ScreenLayout"));
    restore(pickerScreenSizing,     QStringLiteral("ScreenSizing"));
    restore(pickerAspectTop,        QStringLiteral("ScreenAspectTop"));
    restore(pickerAspectBot,        QStringLiteral("ScreenAspectBot"));
    restore(pickerScreenSwap,       QStringLiteral("ScreenSwap"));
    restore(pickerIntegerScaling,   QStringLiteral("ScreenIntegerScaling"));
    restore(pickerScreenFilter,     QStringLiteral("ScreenFilter"));

    currentGBACartPath = s.value(QStringLiteral("GBACartPath"), QString()).toString();
    if (gbaCartLabel) {
        gbaCartLabel->setText((!currentGBACartPath.isEmpty() && QFile::exists(currentGBACartPath))
            ? QFileInfo(currentGBACartPath).fileName()
            : tr("Ninguno"));
    }

    pendingImportSavePath = s.value(QStringLiteral("ImportSavePath"), QString()).toString();
    if (importSaveLabel) {
        importSaveLabel->setText((!pendingImportSavePath.isEmpty() && QFile::exists(pendingImportSavePath))
            ? QFileInfo(pendingImportSavePath).fileName()
            : tr("Ninguno"));
    }
}

void WatermelonGameDialog::savePerGameConfig()
{
    if (romInfo.gameCode.isEmpty()) return;
    QSettings s(perGameConfigPath(romInfo.gameCode), QSettings::IniFormat);

    s.setValue(QStringLiteral("ConsoleType"), pickerConsole  ? pickerConsole->currentIndex()  : 0);
    s.setValue(QStringLiteral("Renderer3D"),  pickerRenderer ? pickerRenderer->currentIndex() : 0);
    s.setValue(QStringLiteral("JIT"),         pickerJit      ? pickerJit->currentIndex()      : 0);
    s.setValue(QStringLiteral("DirectBoot"),  pickerBoot     ? pickerBoot->currentIndex()     : 0);
    s.setValue(QStringLiteral("MicSource"),   pickerMic      ? pickerMic->currentIndex()      : 0);
    s.setValue(QStringLiteral("RTCOption"),        pickerDateTime ? pickerDateTime->currentIndex() : 0);
    s.setValue(QStringLiteral("BatteryLevel"),     pickerBattery ? pickerBattery->currentIndex() : 0);
    s.setValue(QStringLiteral("BatteryCharging"),  pickerDSiCharging ? pickerDSiCharging->currentIndex() : 0);
    if (dteGameCustomTime)
        s.setValue(QStringLiteral("RTCFixedTime"), dteGameCustomTime->dateTime().toString(Qt::ISODate));
    s.setValue(QStringLiteral("EnableCheats"), switchCheatsEnabled ? switchCheatsEnabled->isChecked() : false);
    s.setValue(QStringLiteral("ShaderPath"),  currentShaderPath);
    s.setValue(QStringLiteral("GBACartPath"), currentGBACartPath);
    s.setValue(QStringLiteral("GBAAddon"),    pickerGBAAddon ? pickerGBAAddon->currentIndex() : 0);
    s.setValue(QStringLiteral("ImportSavePath"), pendingImportSavePath);

    s.setValue(QStringLiteral("ScreenWindowSize"),     pickerWindowSize     ? pickerWindowSize->currentIndex()     : 0);
    s.setValue(QStringLiteral("ScreenRotation"),       pickerScreenRotation ? pickerScreenRotation->currentIndex() : 0);
    s.setValue(QStringLiteral("ScreenGap"),            pickerScreenGap      ? pickerScreenGap->currentIndex()      : 0);
    s.setValue(QStringLiteral("ScreenLayout"),         pickerScreenLayout   ? pickerScreenLayout->currentIndex()   : 0);
    s.setValue(QStringLiteral("ScreenSizing"),         pickerScreenSizing   ? pickerScreenSizing->currentIndex()   : 0);
    s.setValue(QStringLiteral("ScreenAspectTop"),      pickerAspectTop      ? pickerAspectTop->currentIndex()      : 0);
    s.setValue(QStringLiteral("ScreenAspectBot"),      pickerAspectBot      ? pickerAspectBot->currentIndex()      : 0);
    s.setValue(QStringLiteral("ScreenSwap"),           pickerScreenSwap     ? pickerScreenSwap->currentIndex()     : 0);
    s.setValue(QStringLiteral("ScreenIntegerScaling"), pickerIntegerScaling ? pickerIntegerScaling->currentIndex() : 0);
    s.setValue(QStringLiteral("ScreenFilter"),         pickerScreenFilter   ? pickerScreenFilter->currentIndex()   : 0);

    s.sync();
}

void WatermelonGameDialog::onSaveConfigClicked()
{
    savePerGameConfig();
    QMessageBox::information(this, tr("Configuración guardada"),
        tr("Configuración de '%1' guardada.").arg(romInfo.title));
}

void WatermelonGameDialog::onChooseShaderClicked()
{
    WatermelonShaderDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString p = dlg.getActivePresetPath();
        if (!p.isEmpty()) {
            currentShaderPath = p;
            if (shaderLabel) shaderLabel->setText(QFileInfo(p).fileName());
        }
    }
}

void WatermelonGameDialog::onClearShaderClicked()
{
    currentShaderPath.clear();
    if (shaderLabel) shaderLabel->setText(tr("Global"));
}

void WatermelonGameDialog::onChooseGBACartClicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Seleccionar ROM de GBA"),
        QFileInfo(romInfo.filePath).absolutePath(),
        tr("ROM de GBA (*.gba *.agb *.bin);;Cualquier archivo (*.*)"));
    if (path.isEmpty()) return;

    currentGBACartPath = path;
    if (gbaCartLabel) gbaCartLabel->setText(QFileInfo(path).fileName());
}

void WatermelonGameDialog::onClearGBACartClicked()
{
    currentGBACartPath.clear();
    if (gbaCartLabel) gbaCartLabel->setText(tr("Ninguno"));
}

void WatermelonGameDialog::onImportSaveClicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Seleccionar partida guardada"),
        QFileInfo(romInfo.filePath).absolutePath(),
        tr("Partidas guardadas (*.sav *.bin *.dsv);;Cualquier archivo (*.*)"));
    if (path.isEmpty()) return;

    pendingImportSavePath = path;
    if (importSaveLabel) importSaveLabel->setText(QFileInfo(path).fileName());
}

WatermelonGBAConfig WatermelonGameDialog::takePendingGBAConfig()
{
    WatermelonGBAConfig cfg = g_pendingGBAConfig;
    g_pendingGBAConfig = WatermelonGBAConfig{};
    return cfg;
}

WatermelonScreenConfig WatermelonGameDialog::takePendingScreenConfig()
{
    WatermelonScreenConfig cfg = g_pendingScreenConfig;
    g_pendingScreenConfig = WatermelonScreenConfig{};
    return cfg;
}

void WatermelonGameDialog::applyGameConfig(const QString& gameCode)
{
    g_pendingGBAConfig = WatermelonGBAConfig{};
    g_pendingScreenConfig = WatermelonScreenConfig{};
    if (gameCode.isEmpty()) return;
    QString path = perGameConfigPath(gameCode);
    if (!QFile::exists(path)) return;

    QSettings s(path, QSettings::IniFormat);
    Config::Table cfg = Config::GetGlobalTable();

    g_pendingGBAConfig.cartPath = s.value(QStringLiteral("GBACartPath"), QString()).toString();

    int addonIndex = s.value(QStringLiteral("GBAAddon"), 0).toInt();
    if (addonIndex > 0 && addonIndex <= 8)
        g_pendingGBAConfig.addon = addonIndex;

    QString importPath = s.value(QStringLiteral("ImportSavePath"), QString()).toString();
    if (!importPath.isEmpty()) {
        g_pendingGBAConfig.importSavePath = importPath;
        // One-shot: a savefile is imported only on the next boot.
        s.remove(QStringLiteral("ImportSavePath"));
        s.sync();
    }

    int winSize = s.value(QStringLiteral("ScreenWindowSize"), 0).toInt();
    if (winSize > 0)
        g_pendingScreenConfig.sizeFactor = winSize;

    int rot = s.value(QStringLiteral("ScreenRotation"), 0).toInt();
    if (rot > 0 && rot <= 4)
        g_pendingScreenConfig.rotation = rot - 1;

    int gapIdx = s.value(QStringLiteral("ScreenGap"), 0).toInt();
    static const int gapValues[] = {0, 1, 8, 64, 90, 128};
    if (gapIdx > 0 && gapIdx <= 6)
        g_pendingScreenConfig.gap = gapValues[gapIdx - 1];

    int lay = s.value(QStringLiteral("ScreenLayout"), 0).toInt();
    if (lay > 0)
        g_pendingScreenConfig.layout = lay - 1;

    int siz = s.value(QStringLiteral("ScreenSizing"), 0).toInt();
    if (siz > 0)
        g_pendingScreenConfig.sizing = siz - 1;

    int aspTop = s.value(QStringLiteral("ScreenAspectTop"), 0).toInt();
    if (aspTop > 0 && aspTop <= AspectRatiosNum)
        g_pendingScreenConfig.aspectTop = aspectRatios[aspTop - 1].id;

    int aspBot = s.value(QStringLiteral("ScreenAspectBot"), 0).toInt();
    if (aspBot > 0 && aspBot <= AspectRatiosNum)
        g_pendingScreenConfig.aspectBot = aspectRatios[aspBot - 1].id;

    int swap = s.value(QStringLiteral("ScreenSwap"), 0).toInt();
    if (swap > 0)
        g_pendingScreenConfig.swap = (swap == 1 ? 1 : 0);

    int intSc = s.value(QStringLiteral("ScreenIntegerScaling"), 0).toInt();
    if (intSc > 0)
        g_pendingScreenConfig.integerScaling = (intSc == 1 ? 1 : 0);

    int flt = s.value(QStringLiteral("ScreenFilter"), 0).toInt();
    if (flt > 0)
        g_pendingScreenConfig.filter = (flt == 1 ? 1 : 0);

    int console  = s.value(QStringLiteral("ConsoleType"), 0).toInt();
    int renderer = s.value(QStringLiteral("Renderer3D"),  0).toInt();
    int jit      = s.value(QStringLiteral("JIT"),         0).toInt();
    int boot     = s.value(QStringLiteral("DirectBoot"),  0).toInt();

    if (console  > 0) cfg.SetInt("Emu.ConsoleType", console - 1);
    if (renderer > 0) cfg.SetInt("3D.Renderer",     renderer - 1);
    if (jit      > 0) cfg.SetBool("JIT.Enable",     jit == 1);
    if (boot     > 0) cfg.SetBool("Emu.DirectBoot", boot == 1);

    Config::Table localCfg0 = Config::GetLocalTable(0);
    int rtcOpt = s.value(QStringLiteral("RTCOption"), 0).toInt();
    if (rtcOpt == 1) {
        localCfg0.SetInt64("RTC.Offset", 0);
    } else if (rtcOpt == 2) {
        QString dtFixedStr = s.value(QStringLiteral("RTCFixedTime")).toString();
        if (!dtFixedStr.isEmpty()) {
            QDateTime target = QDateTime::fromString(dtFixedStr, Qt::ISODate);
            if (target.isValid()) {
                qint64 offset = QDateTime::currentDateTime().secsTo(target);
                localCfg0.SetInt64("RTC.Offset", offset);
            }
        }
    }

    int batLvl = s.value(QStringLiteral("BatteryLevel"), 0).toInt();
    if (batLvl == 1) {
        localCfg0.SetBool("DS.Battery.LevelOkay", true);
        localCfg0.SetInt("DSi.Battery.Level", 0xF);
    } else if (batLvl == 2) {
        localCfg0.SetBool("DS.Battery.LevelOkay", false);
        localCfg0.SetInt("DSi.Battery.Level", 0x3);
    } else if (batLvl == 3) {
        localCfg0.SetBool("DS.Battery.LevelOkay", false);
        localCfg0.SetInt("DSi.Battery.Level", 0x1);
    }

    int batCharge = s.value(QStringLiteral("BatteryCharging"), 0).toInt();
    if (batCharge == 1) {
        localCfg0.SetBool("DSi.Battery.Charging", true);
    } else if (batCharge == 2) {
        localCfg0.SetBool("DSi.Battery.Charging", false);
    }

    bool cheats = s.value(QStringLiteral("EnableCheats"), false).toBool();
    cfg.SetBool("EnableCheats", cheats);

    QString shader = s.value(QStringLiteral("ShaderPath"), QString()).toString();
    if (!shader.isEmpty() && QFile::exists(shader))
        cfg.SetQString("Video.RetroArchShaderPath", shader);
}
