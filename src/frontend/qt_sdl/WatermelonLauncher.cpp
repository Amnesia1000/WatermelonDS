#include "WatermelonLauncher.h"
#include "WatermelonShaderDialog.h"
#include "WatermelonRADialog.h"
#include "WatermelonGameDialog.h"
#include "WatermelonSettingsDialog.h"
#include "Config.h"

#include <cstdint>
#include <functional>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QPainterPath>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QStandardPaths>
#include <QMouseEvent>
#include <QDateTime>
#include <QRegularExpression>
#include <QtEndian>
#include <QTimer>
#include <QMenu>
#include <QDesktopServices>

namespace
{

class ClickableCard : public QWidget
{
public:
    std::function<void()> onClick;
    std::function<void(const QPoint&)> onContextMenu;

    ClickableCard(const QString& filePath, QWidget* parent = nullptr)
        : QWidget(parent), romPath(filePath)
    {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        setContextMenuPolicy(Qt::CustomContextMenu);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            if (onClick) onClick();
        }
        else if (event->button() == Qt::RightButton)
        {
            if (onContextMenu) onContextMenu(event->globalPosition().toPoint());
        }
        QWidget::mousePressEvent(event);
    }

private:
    QString romPath;
};

}

WatermelonLauncher::WatermelonLauncher(QWidget* parent)
    : QWidget(parent)
{
    networkManager = new QNetworkAccessManager(this);
    setupUI();
    refresh();
}

WatermelonLauncher::~WatermelonLauncher() = default;

void WatermelonLauncher::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 14, 18, 14);
    mainLayout->setSpacing(14);

    // --- Header Top Bar ---
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);

    titleLogo = new QLabel(this);
    titleLogo->setTextFormat(Qt::RichText);
    headerLayout->addWidget(titleLogo);

    searchInput = new QLineEdit(this);
    searchInput->setClearButtonEnabled(true);
    searchInput->setMinimumWidth(80);
    searchInput->setMaximumWidth(260);
    searchInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(searchInput, &QLineEdit::textChanged, this, &WatermelonLauncher::onSearchTextChanged);
    headerLayout->addWidget(searchInput);

    headerLayout->addStretch(1);

    btnOpen = new QPushButton(this);
    btnOpen->setProperty("primary", true);
    btnOpen->setCursor(Qt::PointingHandCursor);
    btnOpen->setToolTip(QStringLiteral("Abrir un archivo ROM..."));
    connect(btnOpen, &QPushButton::clicked, this, &WatermelonLauncher::onOpenClicked);
    headerLayout->addWidget(btnOpen);

    btnShaders = new QPushButton(this);
    btnShaders->setCursor(Qt::PointingHandCursor);
    btnShaders->setToolTip(QStringLiteral("Catálogo y configuración de shaders"));
    connect(btnShaders, &QPushButton::clicked, this, &WatermelonLauncher::onShadersClicked);
    headerLayout->addWidget(btnShaders);

    btnRA = new QPushButton(this);
    btnRA->setCursor(Qt::PointingHandCursor);
    btnRA->setToolTip(QStringLiteral("RetroAchievements - Resumen y perfil"));
    connect(btnRA, &QPushButton::clicked, this, &WatermelonLauncher::onRetroAchievementsClicked);
    headerLayout->addWidget(btnRA);

    btnSettings = new QPushButton(this);
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setToolTip(QStringLiteral("Ajustes generales, gráficos, emulación y controles"));
    connect(btnSettings, &QPushButton::clicked, this, &WatermelonLauncher::onSettingsClicked);
    headerLayout->addWidget(btnSettings);

    btnAddDir = new QPushButton(this);
    btnAddDir->setCursor(Qt::PointingHandCursor);
    btnAddDir->setToolTip(QStringLiteral("Añadir carpeta de juegos"));
    connect(btnAddDir, &QPushButton::clicked, this, &WatermelonLauncher::onAddDirectoryClicked);
    headerLayout->addWidget(btnAddDir);

    btnRefresh = new QPushButton(this);
    btnRefresh->setIcon(QIcon(QStringLiteral(":/refresh-icon")));
    btnRefresh->setIconSize(QSize(16, 16));
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setToolTip(QStringLiteral("Actualizar biblioteca"));
    btnRefresh->setFixedSize(34, 32);
    btnRefresh->setStyleSheet(QStringLiteral("QPushButton { padding: 4px; }"));
    connect(btnRefresh, &QPushButton::clicked, this, &WatermelonLauncher::onRefreshClicked);
    headerLayout->addWidget(btnRefresh);

    updateHeaderResponsiveness(width());

    mainLayout->addLayout(headerLayout);

    // --- Scroll Area for Game Content ---
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* contentWidget = new QWidget(scrollArea);
    contentWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 8, 0, 8);
    contentLayout->setSpacing(24);

    // Section 1: Recent Games ("Continuar Jugando")
    recentShelfSection = new QWidget(contentWidget);
    auto* recentSectionLayout = new QVBoxLayout(recentShelfSection);
    recentSectionLayout->setContentsMargins(0, 0, 0, 0);
    recentSectionLayout->setSpacing(12);

    auto* recentHeader = new QLabel(QStringLiteral("<span style='font-size:16px; font-weight:700; color:#6fbf4a;'>▶ Continuar Jugando</span>"), recentShelfSection);
    recentHeader->setTextFormat(Qt::RichText);
    recentSectionLayout->addWidget(recentHeader);

    auto* recentScroll = new QScrollArea(recentShelfSection);
    recentScroll->setWidgetResizable(true);
    recentScroll->setFrameShape(QFrame::NoFrame);
    recentScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    recentScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    recentScroll->setFixedHeight(246);
    recentScroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    recentShelfContainer = new QWidget(recentScroll);
    recentShelfContainer->setStyleSheet(QStringLiteral("background: transparent;"));
    recentShelfLayout = new QHBoxLayout(recentShelfContainer);
    recentShelfLayout->setContentsMargins(0, 0, 0, 0);
    recentShelfLayout->setSpacing(16);
    recentShelfLayout->addStretch();
    recentScroll->setWidget(recentShelfContainer);

    recentSectionLayout->addWidget(recentScroll);
    contentLayout->addWidget(recentShelfSection);

    // Section 2: Main Games Grid ("Biblioteca de Juegos")
    auto* gridSection = new QWidget(contentWidget);
    auto* gridSectionLayout = new QVBoxLayout(gridSection);
    gridSectionLayout->setContentsMargins(0, 0, 0, 0);
    gridSectionLayout->setSpacing(12);

    auto* gridHeader = new QLabel(QStringLiteral("<span style='font-size:16px; font-weight:700; color:#f6f5f3;'>🎮 Biblioteca de Juegos</span>"), gridSection);
    gridHeader->setTextFormat(Qt::RichText);
    gridSectionLayout->addWidget(gridHeader);

    gridContainer = new QWidget(gridSection);
    gridContainer->setStyleSheet(QStringLiteral("background: transparent;"));
    gridLayout = new QGridLayout(gridContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(16);
    gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    gridSectionLayout->addWidget(gridContainer);

    emptyLabel = new QLabel(gridSection);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setTextFormat(Qt::RichText);
    emptyLabel->setStyleSheet(QStringLiteral(
        "background-color: #1a1921; border: 2px dashed #2e2d38; border-radius: 16px; padding: 40px;"
    ));
    emptyLabel->setText(QStringLiteral(
        "<div style='font-size:16px; font-weight:700; color:#f6f5f3; margin-bottom:8px;'>¡Bienvenido a WatermelonDS para Linux! 🍉</div>"
        "<div style='font-size:13px; color:#b3b1bd; margin-bottom:16px;'>No se encontraron juegos en tus carpetas de ROMs.</div>"
        "<div style='font-size:12px; color:#6fbf4a;'>Haz clic en '➕ Añadir Carpeta' o '📁 Abrir ROM...' para comenzar a jugar.</div>"
    ));
    gridSectionLayout->addWidget(emptyLabel);

    contentLayout->addWidget(gridSection);
    contentLayout->addStretch();

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void WatermelonLauncher::refresh()
{
    loadRecent();
    scanDirectories();
    renderGames();
}

void WatermelonLauncher::loadRecent()
{
    recentRoms.clear();
    Config::Table globalCfg = Config::GetGlobalTable();
    Config::Array recentArr = globalCfg.GetArray("RecentROM");
    size_t count = recentArr.Size();

    for (size_t i = 0; i < count && i < 10; ++i)
    {
        QString path = recentArr.GetQString((int)i);
        if (path.isEmpty() || !QFile::exists(path)) continue;

        WatermelonRom rom;
        rom.filePath = path;
        rom.fileName = QFileInfo(path).fileName();
        rom.isRecent = true;
        rom.cover = extractRomInfo(path, rom.title, rom.gameCode, rom.icon);
        recentRoms.append(rom);
    }
}

void WatermelonLauncher::scanDirectories()
{
    allRoms.clear();
    QSet<QString> addedPaths;

    // 1. Add recent files first
    for (const auto& r : recentRoms)
    {
        allRoms.append(r);
        addedPaths.insert(r.filePath);
    }

    // 2. Add configured directories
    Config::Table globalCfg = Config::GetGlobalTable();
    QString dirsStr = globalCfg.GetQString("WatermelonRomDirs");
    QStringList dirs = dirsStr.split(QLatin1Char(';'), Qt::SkipEmptyParts);

    // Also check standard home roms directory if empty
    QString defaultRomDir = QDir::homePath() + QStringLiteral("/Roms/NDS");
    if (QDir(defaultRomDir).exists() && !dirs.contains(defaultRomDir))
    {
        dirs.append(defaultRomDir);
    }

    const QStringList filters = { "*.nds", "*.dsi", "*.srl" };

    for (const QString& dirPath : dirs)
    {
        QDir dir(dirPath);
        if (!dir.exists()) continue;

        QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);
        for (const auto& fi : files)
        {
            QString absPath = fi.absoluteFilePath();
            if (addedPaths.contains(absPath)) continue;

            WatermelonRom rom;
            rom.filePath = absPath;
            rom.fileName = fi.fileName();
            rom.cover = extractRomInfo(absPath, rom.title, rom.gameCode, rom.icon);
            allRoms.append(rom);
            addedPaths.insert(absPath);
        }
    }
}

void WatermelonLauncher::renderGames()
{
    // Render Recent Shelf
    while (recentShelfLayout->count() > 1)
    {
        auto* item = recentShelfLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }

    if (recentRoms.isEmpty())
    {
        recentShelfSection->hide();
    }
    else
    {
        recentShelfSection->show();
        for (const auto& rom : recentRoms)
        {
            QWidget* card = createGameCard(rom, true);
            recentShelfLayout->insertWidget(recentShelfLayout->count() - 1, card);
        }
    }

    // Render Grid
    while (gridLayout->count() > 0)
    {
        auto* item = gridLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }

    QList<WatermelonRom> filtered;
    for (const auto& rom : allRoms)
    {
        if (currentFilter.isEmpty() ||
            rom.title.contains(currentFilter, Qt::CaseInsensitive) ||
            rom.fileName.contains(currentFilter, Qt::CaseInsensitive) ||
            rom.gameCode.contains(currentFilter, Qt::CaseInsensitive))
        {
            filtered.append(rom);
        }
    }

    if (filtered.isEmpty())
    {
        emptyLabel->show();
        gridContainer->hide();
    }
    else
    {
        emptyLabel->hide();
        gridContainer->show();

        int availableWidth = scrollArea ? scrollArea->viewport()->width() : width();
        if (availableWidth < 300) availableWidth = 900;
        int cardWidth = 160;
        int spacing = 16;
        int columns = qMax(1, (availableWidth + spacing) / (cardWidth + spacing));

        // Clear previous column stretches
        for (int c = 0; c <= columns + 2; ++c)
            gridLayout->setColumnStretch(c, 0);

        for (int i = 0; i < filtered.size(); ++i)
        {
            int row = i / columns;
            int col = i % columns;
            QWidget* card = createGameCard(filtered[i], false);
            gridLayout->addWidget(card, row, col, Qt::AlignLeft | Qt::AlignTop);
        }

        // Add stretch after the last column so cards stay left-aligned and match recent cards
        gridLayout->setColumnStretch(columns, 1);
    }
}

void WatermelonLauncher::updateHeaderResponsiveness(int w)
{
    if (!titleLogo || !btnOpen) return;

    if (w >= 1080) {
        titleLogo->setText(QStringLiteral("🍉 <span style='font-size:20px; font-weight:800; color:#f6f5f3;'>Watermelon<span style='color:#6fbf4a;'>DS</span></span>"));
        searchInput->setPlaceholderText(QStringLiteral("🔍 Buscar en tu biblioteca..."));
        searchInput->setMaximumWidth(280);
        btnOpen->setText(QStringLiteral("📁 Abrir ROM..."));
        btnShaders->setText(QStringLiteral("🎨 Shaders"));
        btnRA->setText(QStringLiteral("🏆 Logros"));
        btnSettings->setText(QStringLiteral("⚙ Ajustes"));
        btnAddDir->setText(QStringLiteral("➕ Carpeta"));
        btnAddDir->setVisible(true);
    } else if (w >= 880) {
        titleLogo->setText(QStringLiteral("🍉 <span style='font-size:18px; font-weight:800; color:#f6f5f3;'>Watermelon<span style='color:#6fbf4a;'>DS</span></span>"));
        searchInput->setPlaceholderText(QStringLiteral("🔍 Buscar..."));
        searchInput->setMaximumWidth(200);
        btnOpen->setText(QStringLiteral("📁 Abrir"));
        btnShaders->setText(QStringLiteral("🎨"));
        btnRA->setText(QStringLiteral("🏆 Logros"));
        btnSettings->setText(QStringLiteral("⚙ Ajustes"));
        btnAddDir->setVisible(false);
    } else {
        titleLogo->setText(QStringLiteral("🍉"));
        searchInput->setPlaceholderText(QStringLiteral("🔍"));
        searchInput->setMaximumWidth(140);
        btnOpen->setText(QStringLiteral("📁"));
        btnShaders->setText(QStringLiteral("🎨"));
        btnRA->setText(QStringLiteral("🏆"));
        btnSettings->setText(QStringLiteral("⚙"));
        btnAddDir->setVisible(false);
    }
}

void WatermelonLauncher::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateHeaderResponsiveness(width());
    if (!allRoms.isEmpty() && gridContainer && gridContainer->isVisible())
    {
        renderGames();
    }
}

QWidget* WatermelonLauncher::createGameCard(const WatermelonRom& rom, bool isRecentShelf)
{
    auto* card = new ClickableCard(rom.filePath, this);
    const int cardWidth = 160;
    const int cardHeight = 230;
    card->setFixedSize(cardWidth, cardHeight);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    card->setStyleSheet(QStringLiteral(
        "QWidget {"
        "   background-color: #1e1d24;"
        "   border: 1px solid #2e2d38;"
        "   border-radius: 14px;"
        "}"
        "QWidget:hover {"
        "   background-color: #272630;"
        "   border: 2px solid #6fbf4a;"
        "}"
    ));

    // Cover Image with optional achievement badge overlay
    int coverH = isRecentShelf ? 130 : 140;
    auto* coverContainer = new QWidget(card);
    coverContainer->setFixedSize(cardWidth - 20, coverH);
    coverContainer->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto* coverLabel = new QLabel(coverContainer);
    coverLabel->setGeometry(0, 0, cardWidth - 20, coverH);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setStyleSheet(QStringLiteral("background: transparent; border: none; border-radius: 8px;"));

    QPixmap scaled = rom.cover.scaled(coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    coverLabel->setPixmap(scaled);

    auto* raBadge = new QLabel(QStringLiteral("🏆"), coverContainer);
    raBadge->setToolTip(QStringLiteral("Soporte para RetroAchievements"));
    raBadge->setStyleSheet(QStringLiteral(
        "background: rgba(18, 17, 25, 0.85); border-radius: 9px; padding: 1px 4px; font-size: 11px; border: 1px solid rgba(255,255,255,0.12);"
    ));
    raBadge->adjustSize();
    raBadge->move(coverContainer->width() - raBadge->width() - 4, 4);
    raBadge->raise();

    layout->addWidget(coverContainer);

    // Title (takes all remaining space for large titles without cutting off)
    auto* titleLabel = new QLabel(card);
    QString fullTitle = rom.title.isEmpty() ? rom.fileName : rom.title;
    titleLabel->setText(fullTitle);
    titleLabel->setStyleSheet(QStringLiteral(
        "color: #f6f5f3; font-weight: 600; font-size: 11.5px; background: transparent; border: none;"
    ));
    titleLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    titleLabel->setWordWrap(true);
    titleLabel->setToolTip(fullTitle);
    layout->addWidget(titleLabel, 1);

    card->onClick = [this, rom]() {
        WatermelonGameDialog::applyGameConfig(rom.gameCode);
        emit romSelected(rom.filePath);
    };

    card->onContextMenu = [this, rom](const QPoint& pos) {
        showContextMenu(rom, pos);
    };

    return card;
}

void WatermelonLauncher::showContextMenu(const WatermelonRom& rom, const QPoint& globalPos)
{
    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(
        "QMenu {"
        "   background-color: #1e1d24;"
        "   color: #f6f5f3;"
        "   border: 1px solid #2e2d38;"
        "   border-radius: 10px;"
        "   padding: 6px;"
        "}"
        "QMenu::item {"
        "   padding: 8px 24px;"
        "   border-radius: 6px;"
        "   font-size: 13px;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #272630;"
        "   color: #6fbf4a;"
        "}"
        "QMenu::separator {"
        "   height: 1px;"
        "   background: #2e2d38;"
        "   margin: 4px 8px;"
        "}"
    ));

    auto* actPlay = menu.addAction(QStringLiteral("▶  Iniciar juego"));
    menu.addSeparator();
    auto* actConfig = menu.addAction(QStringLiteral("⚙️  Configuración del juego..."));
    auto* actCheats = menu.addAction(QStringLiteral("👾  Trucos (Cheats)..."));
    auto* actRA = menu.addAction(QStringLiteral("🏆  RetroAchievements..."));
    auto* actOffline = menu.addAction(QStringLiteral("📴  Modo Offline..."));
    menu.addSeparator();
    auto* actOpenDir = menu.addAction(QStringLiteral("📁  Abrir carpeta del archivo"));
    auto* actReloadCover = menu.addAction(QIcon(QStringLiteral(":/refresh-icon")), QStringLiteral("  Forzar recarga de carátula"));

    QAction* selected = menu.exec(globalPos);
    if (!selected) return;

    if (selected == actPlay)
    {
        WatermelonGameDialog::applyGameConfig(rom.gameCode);
        emit romSelected(rom.filePath);
    }
    else if (selected == actConfig)
    {
        openGameDialog(rom, 0); // Tab 0: Configuración
    }
    else if (selected == actCheats)
    {
        openGameDialog(rom, 1); // Tab 1: Trucos
    }
    else if (selected == actRA)
    {
        openGameDialog(rom, 2); // Tab 2: RetroAchievements
    }
    else if (selected == actOffline)
    {
        openGameDialog(rom, 3); // Tab 3: Modo Offline
    }
    else if (selected == actOpenDir)
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(rom.filePath).absolutePath()));
    }
    else if (selected == actReloadCover)
    {
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/melonDS/boxart");
        QFile::remove(cacheDir + QStringLiteral("/") + rom.gameCode + QStringLiteral(".png"));
        QFile::remove(cacheDir + QStringLiteral("/") + rom.title + QStringLiteral(".png"));
        pendingDownloads.remove(rom.gameCode);
        fetchCoverArt(rom.title, rom.gameCode, QFileInfo(rom.filePath).completeBaseName());
    }
}

void WatermelonLauncher::openGameDialog(const WatermelonRom& rom, int initialTab)
{
    WatermelonGameDialog dlg(rom, initialTab, this);
    dlg.exec();
}

QPixmap WatermelonLauncher::extractRomInfo(const QString& path, QString& outTitle, QString& outCode, QPixmap& outIcon)
{
    QFileInfo fi(path);
    QString rawBaseName = fi.completeBaseName();
    QString cleanBaseName = rawBaseName.trimmed();

    // Strip trailing region, language and translation tags: e.g. (Europe), (USA), (Spain), (En,Fr), (Spa), [T], (Rev 1)
    static const QRegularExpression tagRegex(QStringLiteral("\\s*(\\([^\\)]*\\)|\\[[^\\]]*\\])\\s*$"));
    while (tagRegex.match(cleanBaseName).hasMatch())
    {
        cleanBaseName = cleanBaseName.replace(tagRegex, QString()).trimmed();
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        outTitle = cleanBaseName.isEmpty() ? rawBaseName : cleanBaseName;
        return generateDefaultCover(outTitle, outCode, outIcon);
    }

    char header[512];
    qint64 readBytes = file.read(header, sizeof(header));

    QString rawHeaderTitle;
    quint32 bannerOffset = 0;

    if (readBytes >= 0x70)
    {
        char titleBuf[13] = {0};
        memcpy(titleBuf, header, 12);
        rawHeaderTitle = QString::fromLatin1(titleBuf).trimmed();

        char codeBuf[5] = {0};
        memcpy(codeBuf, header + 12, 4);
        outCode = QString::fromLatin1(codeBuf).trimmed();

        bannerOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(header + 0x68));
    }

    QString bannerTitle;
    if (bannerOffset >= 0x200 && (qint64)bannerOffset + 0x840 <= file.size())
    {
        if (file.seek(bannerOffset))
        {
            QByteArray bannerData = file.read(0xA00); // 2560 bytes
            if (bannerData.size() >= 0x840)
            {
                // Decode 32x32 NDS icon
                quint32 paletteRGBA[16];
                for (int i = 0; i < 16; i++)
                {
                    quint16 c = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(bannerData.constData() + 0x220 + i * 2));
                    uint8_t r = ((c >> 0)  & 0x1F) * 255 / 31;
                    uint8_t g = ((c >> 5)  & 0x1F) * 255 / 31;
                    uint8_t b = ((c >> 10) & 0x1F) * 255 / 31;
                    uint8_t a = (i == 0) ? 0 : 255;
                    paletteRGBA[i] = (a << 24) | (r << 16) | (g << 8) | b;
                }

                QImage iconImg(32, 32, QImage::Format_ARGB32);
                int count = 0;
                const uint8_t* iconTiles = reinterpret_cast<const uint8_t*>(bannerData.constData() + 0x20);
                for (int ytile = 0; ytile < 4; ytile++)
                {
                    for (int xtile = 0; xtile < 4; xtile++)
                    {
                        for (int ypixel = 0; ypixel < 8; ypixel++)
                        {
                            for (int xpixel = 0; xpixel < 8; xpixel++)
                            {
                                uint8_t byteVal = iconTiles[count / 2];
                                uint8_t palIndex = (count % 2) ? (byteVal >> 4) : (byteVal & 0x0F);
                                iconImg.setPixel(xtile * 8 + xpixel, ytile * 8 + ypixel, paletteRGBA[palIndex]);
                                count++;
                            }
                        }
                    }
                }
                outIcon = QPixmap::fromImage(iconImg);

                // Titles order: Spanish (0x740), English (0x340), Japanese (0x240), French (0x440), German (0x540), Italian (0x640)
                const int offsets[] = { 0x740, 0x340, 0x240, 0x440, 0x540, 0x640 };
                for (int off : offsets)
                {
                    QString t = QString::fromUtf16(reinterpret_cast<const char16_t*>(bannerData.constData() + off), 128);
                    int nullIdx = t.indexOf(QChar('\0'));
                    if (nullIdx >= 0) t = t.left(nullIdx);
                    t = t.trimmed();
                    if (!t.isEmpty())
                    {
                        QStringList lines = t.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                        if (lines.size() >= 3)
                            bannerTitle = lines[0].trimmed() + QStringLiteral(": ") + lines[1].trimmed();
                        else if (!lines.isEmpty())
                            bannerTitle = lines[0].trimmed();
                        break;
                    }
                }
            }
        }
    }
    file.close();

    // Determine user-friendly title
    bool isGenericName = cleanBaseName.isEmpty() ||
                         cleanBaseName.compare(outCode, Qt::CaseInsensitive) == 0 ||
                         cleanBaseName.compare(QStringLiteral("game"), Qt::CaseInsensitive) == 0 ||
                         cleanBaseName.compare(QStringLiteral("rom"), Qt::CaseInsensitive) == 0;

    if (!isGenericName && cleanBaseName.length() >= 3)
    {
        outTitle = cleanBaseName;
    }
    else if (!bannerTitle.isEmpty())
    {
        outTitle = bannerTitle;
    }
    else if (!cleanBaseName.isEmpty())
    {
        outTitle = cleanBaseName;
    }
    else
    {
        outTitle = rawHeaderTitle.isEmpty() ? rawBaseName : rawHeaderTitle;
    }

    // Check cached boxart from disk
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/melonDS/boxart");
    QDir().mkpath(cacheDir);

    QStringList cacheCandidates;
    if (!outCode.isEmpty())
    {
        cacheCandidates << cacheDir + QStringLiteral("/") + outCode + QStringLiteral(".png");
        cacheCandidates << cacheDir + QStringLiteral("/") + outCode + QStringLiteral(".jpg");
    }
    if (!outTitle.isEmpty())
    {
        cacheCandidates << cacheDir + QStringLiteral("/") + outTitle + QStringLiteral(".png");
        cacheCandidates << cacheDir + QStringLiteral("/") + outTitle + QStringLiteral(".jpg");
    }
    if (!rawBaseName.isEmpty() && rawBaseName != outTitle)
    {
        cacheCandidates << cacheDir + QStringLiteral("/") + rawBaseName + QStringLiteral(".png");
        cacheCandidates << cacheDir + QStringLiteral("/") + rawBaseName + QStringLiteral(".jpg");
    }

    for (const QString& cPath : cacheCandidates)
    {
        if (QFile::exists(cPath))
        {
            QPixmap cached(cPath);
            if (!cached.isNull()) return cached;
        }
    }

    // Request cover art download asynchronously
    fetchCoverArt(outTitle, outCode, rawBaseName);

    return generateDefaultCover(outTitle, outCode, outIcon);
}

QPixmap WatermelonLauncher::generateDefaultCover(const QString& title, const QString& gameCode, const QPixmap& iconPix)
{
    Q_UNUSED(title);
    Q_UNUSED(gameCode);

    int w = 140;
    int h = 130;
    QPixmap pix(w, h);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // DS Cartridge Shape
    QPainterPath path;
    path.addRoundedRect(4, 4, w - 8, h - 8, 12, 12);

    // Gradient background
    QLinearGradient grad(0, 0, 0, h);
    grad.setColorAt(0.0, QColor(0x2a, 0x28, 0x33));
    grad.setColorAt(1.0, QColor(0x18, 0x17, 0x1d));
    p.fillPath(path, grad);

    p.setPen(QPen(QColor(0x3e, 0x3d, 0x4a), 1.5));
    p.drawPath(path);

    // Watermelon header bar
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x6f, 0xbf, 0x4a));
    p.drawRoundedRect(QRect(14, 12, w - 28, 5), 2, 2);

    // Centered Icon / Cartridge Art (only the icon, no text inside the cartridge)
    if (!iconPix.isNull())
    {
        QPixmap scaledIcon = iconPix.scaled(48, 48, Qt::KeepAspectRatio, Qt::FastTransformation);
        int iconX = (w - scaledIcon.width()) / 2;
        int iconY = 16 + (h - 20 - scaledIcon.height()) / 2;
        p.drawPixmap(iconX, iconY, scaledIcon);
    }
    else
    {
        p.setFont(QFont(QStringLiteral("sans-serif"), 28));
        p.setPen(QColor(0x6f, 0xbf, 0x4a));
        p.drawText(QRect(0, 18, w, h - 22), Qt::AlignCenter, QStringLiteral("🎮"));
    }

    return pix;
}

void WatermelonLauncher::fetchCoverArt(const QString& title, const QString& gameCode, const QString& rawBaseName)
{
    if (gameCode.isEmpty() && title.isEmpty()) return;
    QString key = !gameCode.isEmpty() ? gameCode : title;
    if (pendingDownloads.contains(key)) return;

    pendingDownloads.insert(key);

    QStringList candidates;
    if (gameCode.length() == 4)
    {
        char regChar = gameCode.at(3).toLatin1();
        QString reg = QStringLiteral("EN");
        switch (regChar)
        {
            case 'E': case 'T': reg = QStringLiteral("US"); break;
            case 'J': reg = QStringLiteral("JA"); break;
            case 'K': reg = QStringLiteral("KO"); break;
            case 'C': reg = QStringLiteral("ZH"); break;
            case 'D': reg = QStringLiteral("DE"); break;
            case 'F': reg = QStringLiteral("FR"); break;
            case 'I': reg = QStringLiteral("IT"); break;
            case 'S': reg = QStringLiteral("ES"); break;
            case 'H': reg = QStringLiteral("NL"); break;
            case 'R': reg = QStringLiteral("RU"); break;
            default:  reg = QStringLiteral("EN"); break;
        }

        // 1. GameTDB primary region (HTTP port 80 is reliable and fast for art.gametdb.com)
        candidates << QStringLiteral("http://art.gametdb.com/ds/coverHQ/%1/%2.jpg").arg(reg, gameCode);
        candidates << QStringLiteral("http://art.gametdb.com/ds/coverM/%1/%2.jpg").arg(reg, gameCode);
        candidates << QStringLiteral("http://art.gametdb.com/ds/cover/%1/%2.jpg").arg(reg, gameCode);

        // 2. GameTDB EN fallback
        if (reg != QLatin1String("EN"))
        {
            candidates << QStringLiteral("http://art.gametdb.com/ds/coverHQ/EN/%1.jpg").arg(gameCode);
            candidates << QStringLiteral("http://art.gametdb.com/ds/coverM/EN/%1.jpg").arg(gameCode);
            candidates << QStringLiteral("http://art.gametdb.com/ds/cover/EN/%1.jpg").arg(gameCode);
        }

        // 3. GameTDB US fallback
        if (reg != QLatin1String("US"))
        {
            candidates << QStringLiteral("http://art.gametdb.com/ds/coverHQ/US/%1.jpg").arg(gameCode);
            candidates << QStringLiteral("http://art.gametdb.com/ds/coverM/US/%1.jpg").arg(gameCode);
        }
    }

    // 4. LibRetro raw.githubusercontent.com fallback (by full filename and clean title)
    if (!rawBaseName.isEmpty())
    {
        QString cleanLibretro = rawBaseName;
        cleanLibretro.replace(QLatin1Char('&'), QLatin1Char('_'));
        cleanLibretro.replace(QLatin1Char('*'), QLatin1Char('_'));
        cleanLibretro.replace(QLatin1Char('/'), QLatin1Char('-'));
        cleanLibretro.replace(QLatin1Char(':'), QLatin1Char('_'));
        candidates << QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_DS/master/Named_Boxarts/%1.png")
            .arg(QString::fromUtf8(QUrl::toPercentEncoding(cleanLibretro)));
    }
    if (!title.isEmpty() && title != rawBaseName)
    {
        QString cleanLibretro = title;
        cleanLibretro.replace(QLatin1Char('&'), QLatin1Char('_'));
        cleanLibretro.replace(QLatin1Char('*'), QLatin1Char('_'));
        cleanLibretro.replace(QLatin1Char('/'), QLatin1Char('-'));
        cleanLibretro.replace(QLatin1Char(':'), QLatin1Char('_'));
        candidates << QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_DS/master/Named_Boxarts/%1.png")
            .arg(QString::fromUtf8(QUrl::toPercentEncoding(cleanLibretro)));
    }

    tryDownloadCandidate(key, candidates, 0);
}

void WatermelonLauncher::tryDownloadCandidate(const QString& key, const QStringList& urls, int index)
{
    if (index >= urls.size())
    {
        pendingDownloads.remove(key);
        return;
    }

    QUrl url(urls.at(index));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QByteArrayLiteral("Mozilla/5.0 (X11; Linux x86_64) WatermelonDS/1.0"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(5000);

    auto* reply = networkManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, key, urls, index]() {
        reply->deleteLater();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && statusCode == 200)
        {
            QByteArray data = reply->readAll();
            QPixmap pix;
            if (pix.loadFromData(data) && !pix.isNull())
            {
                QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/melonDS/boxart");
                QDir().mkpath(cacheDir);
                pix.save(cacheDir + QStringLiteral("/") + key + QStringLiteral(".png"), "PNG");
                pendingDownloads.remove(key);
                onCoverDownloaded(key, pix);
                return;
            }
        }

        // Try next candidate
        tryDownloadCandidate(key, urls, index + 1);
    });
}

void WatermelonLauncher::onCoverDownloaded(const QString& key, const QPixmap& pixmap)
{
    bool updated = false;
    for (auto& r : allRoms)
    {
        if (r.gameCode == key || r.title == key)
        {
            r.cover = pixmap;
            updated = true;
        }
    }
    for (auto& r : recentRoms)
    {
        if (r.gameCode == key || r.title == key)
        {
            r.cover = pixmap;
            updated = true;
        }
    }
    if (updated)
    {
        if (!renderDebounceTimer)
        {
            renderDebounceTimer = new QTimer(this);
            renderDebounceTimer->setSingleShot(true);
            renderDebounceTimer->setInterval(60);
            connect(renderDebounceTimer, &QTimer::timeout, this, &WatermelonLauncher::renderGames);
        }
        if (!renderDebounceTimer->isActive())
        {
            renderDebounceTimer->start();
        }
    }
}

void WatermelonLauncher::onSearchTextChanged(const QString& text)
{
    currentFilter = text.trimmed();
    renderGames();
}

void WatermelonLauncher::onOpenClicked()
{
    emit openFileRequested();
}

void WatermelonLauncher::onAddDirectoryClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Seleccionar carpeta de juegos de Nintendo DS"));
    if (dir.isEmpty()) return;

    Config::Table globalCfg = Config::GetGlobalTable();
    QString dirsStr = globalCfg.GetQString("WatermelonRomDirs");
    QStringList dirs = dirsStr.split(QLatin1Char(';'), Qt::SkipEmptyParts);

    if (!dirs.contains(dir))
    {
        dirs.append(dir);
        globalCfg.SetQString("WatermelonRomDirs", dirs.join(QLatin1Char(';')));
        Config::Save();
        refresh();
    }
}

void WatermelonLauncher::onRefreshClicked()
{
    refresh();
}

void WatermelonLauncher::onShadersClicked()
{
    WatermelonShaderDialog dlg(this);
    dlg.exec();
}

void WatermelonLauncher::onRetroAchievementsClicked()
{
    WatermelonRADialog dlg(this);
    dlg.exec();
}

void WatermelonLauncher::onSettingsClicked()
{
    WatermelonSettingsDialog dlg(this);
    connect(&dlg, &WatermelonSettingsDialog::settingsSaved, this, &WatermelonLauncher::refresh);
    dlg.exec();
}
