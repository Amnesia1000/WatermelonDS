#include "WatermelonShaderDialog.h"
#include "Config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QProgressBar>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QProcess>
#include <QStandardPaths>

namespace
{
constexpr const char* SHADER_DOWNLOAD_URL = "https://buildbot.libretro.com/assets/frontend/shaders_slang.zip";
}

WatermelonShaderDialog::WatermelonShaderDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Catálogo de Shaders - WatermelonDS"));
    resize(780, 560);
    setModal(true);

    shadersPath = getShaderDirectory();
    networkManager = new QNetworkAccessManager(this);

    setupUI();
    loadShaderCategories();
    updateActivePresetLabel();
}

WatermelonShaderDialog::~WatermelonShaderDialog() = default;

QString WatermelonShaderDialog::getShaderDirectory()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/melonDS");
    QString dir = configDir + "/shaders";
    QDir().mkpath(dir);
    return dir;
}

QString WatermelonShaderDialog::getActivePresetPath()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    return globalCfg.GetQString("Video.RetroArchShaderPath");
}

void WatermelonShaderDialog::setActivePresetPath(const QString& path)
{
    Config::Table globalCfg = Config::GetGlobalTable();
    globalCfg.SetQString("Video.RetroArchShaderPath", path);
    Config::Save();
}

bool WatermelonShaderDialog::checkShadersInstalled() const
{
    QDir dir(shadersPath);
    if (!dir.exists()) return false;

    // Check if there are any .slangp files
    QDirIterator it(shadersPath, QStringList() << "*.slangp", QDir::Files, QDirIterator::Subdirectories);
    return it.hasNext();
}

void WatermelonShaderDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    // Title Header
    auto* titleLabel = new QLabel(QStringLiteral("🎨 <span style='font-size: 18px; font-weight: 700; color: #f6f5f3;'>Catálogo de Shaders RetroArch</span>"), this);
    titleLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(titleLabel);

    auto* subLabel = new QLabel(QStringLiteral("Aplica filtros gráficos profesionales (CRT, LCD Game Boy/NDS, Scanlines, xBRZ, ScaleFX, etc.) para mejorar la imagen del juego."), this);
    subLabel->setWordWrap(true);
    subLabel->setStyleSheet(QStringLiteral("color: #a09da8; font-size: 12px; margin-bottom: 4px;"));
    mainLayout->addWidget(subLabel);

    // Download Banner (if not installed)
    downloadBanner = new QWidget(this);
    downloadBanner->setStyleSheet(QStringLiteral(
        "QWidget {"
        "  background: #1e1d24;"
        "  border: 1px solid #363442;"
        "  border-radius: 8px;"
        "  padding: 12px;"
        "}"
    ));
    auto* bannerLayout = new QVBoxLayout(downloadBanner);
    bannerLayout->setSpacing(8);

    statusLabel = new QLabel(QStringLiteral("📦 El paquete oficial de shaders Slang de RetroArch no está instalado en este sistema."), downloadBanner);
    statusLabel->setStyleSheet(QStringLiteral("color: #e5e4e8; font-weight: 600; border: none; background: transparent;"));
    bannerLayout->addWidget(statusLabel);

    auto* bannerBtnLayout = new QHBoxLayout();
    btnDownload = new QPushButton(QStringLiteral("⬇️ Descargar Catálogo Oficial Slang (~25 MB)"), downloadBanner);
    btnDownload->setProperty("primary", true);
    btnDownload->setCursor(Qt::PointingHandCursor);
    connect(btnDownload, &QPushButton::clicked, this, &WatermelonShaderDialog::onDownloadClicked);
    bannerBtnLayout->addWidget(btnDownload);
    bannerBtnLayout->addStretch();
    bannerLayout->addLayout(bannerBtnLayout);

    progressBar = new QProgressBar(downloadBanner);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setVisible(false);
    progressBar->setFixedHeight(14);
    progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { background: #141317; border: 1px solid #363442; border-radius: 4px; text-align: center; color: #f6f5f3; font-size: 10px; }"
        "QProgressBar::chunk { background: #6fbf4a; border-radius: 3px; }"
    ));
    bannerLayout->addWidget(progressBar);

    mainLayout->addWidget(downloadBanner);
    downloadBanner->setVisible(!checkShadersInstalled());

    // Search / Filter Box
    filterInput = new QLineEdit(this);
    filterInput->setPlaceholderText(QStringLiteral("🔍 Filtrar presets (ej: crt, lcd, xbrz, handheld, scanline)..."));
    filterInput->setClearButtonEnabled(true);
    connect(filterInput, &QLineEdit::textChanged, this, &WatermelonShaderDialog::onFilterChanged);
    mainLayout->addWidget(filterInput);

    // Tree / List of Shaders
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderLabels(QStringList() << QStringLiteral("Categoría / Preset") << QStringLiteral("Archivo"));
    treeWidget->setColumnWidth(0, 360);
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setStyleSheet(QStringLiteral(
        "QTreeWidget {"
        "  background: #1e1d24;"
        "  border: 1px solid #2a2833;"
        "  border-radius: 8px;"
        "  color: #f6f5f3;"
        "}"
        "QTreeWidget::item { padding: 4px; }"
        "QTreeWidget::item:selected { background: #2a2833; color: #6fbf4a; }"
    ));
    connect(treeWidget, &QTreeWidget::itemSelectionChanged, this, &WatermelonShaderDialog::onCategorySelectionChanged);
    connect(treeWidget, &QTreeWidget::itemDoubleClicked, this, &WatermelonShaderDialog::onPresetItemDoubleClicked);
    mainLayout->addWidget(treeWidget, 1);

    // Active Preset Info & Bottom Actions
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);

    activePresetLabel = new QLabel(this);
    activePresetLabel->setStyleSheet(QStringLiteral("color: #a09da8; font-size: 12px; font-weight: 500;"));
    bottomLayout->addWidget(activePresetLabel, 1);

    btnDisable = new QPushButton(QStringLiteral("🚫 Sin Shader"), this);
    btnDisable->setToolTip(QStringLiteral("Desactivar shader y volver a renderizado directo nativo"));
    connect(btnDisable, &QPushButton::clicked, this, &WatermelonShaderDialog::onDisableClicked);
    bottomLayout->addWidget(btnDisable);

    btnApply = new QPushButton(QStringLiteral("✨ Aplicar Shader"), this);
    btnApply->setProperty("primary", true);
    btnApply->setEnabled(false);
    connect(btnApply, &QPushButton::clicked, this, &WatermelonShaderDialog::onApplyClicked);
    bottomLayout->addWidget(btnApply);

    auto* btnClose = new QPushButton(QStringLiteral("Cerrar"), this);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(btnClose);

    mainLayout->addLayout(bottomLayout);
}

void WatermelonShaderDialog::updateActivePresetLabel()
{
    QString active = getActivePresetPath();
    if (active.isEmpty())
    {
        activePresetLabel->setText(QStringLiteral("Preset activo: <span style='color: #a09da8;'>Ninguno (Renderizado directo)</span>"));
    }
    else
    {
        QFileInfo fi(active);
        activePresetLabel->setText(QStringLiteral("Preset activo: <b style='color: #6fbf4a;'>%1</b>").arg(fi.fileName()));
    }
}

void WatermelonShaderDialog::loadShaderCategories()
{
    treeWidget->clear();
    QString filter = filterInput ? filterInput->text().trimmed().toLower() : QString();

    QDir rootDir(shadersPath);
    if (!rootDir.exists()) return;

    // Scan top-level folders
    QStringList subdirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& sub : subdirs)
    {
        QDir subDir(rootDir.filePath(sub));
        QStringList slangpFiles = subDir.entryList(QStringList() << "*.slangp", QDir::Files, QDir::Name);

        // Also check one level deeper
        QStringList nestedDirs = subDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& nd : nestedDirs)
        {
            QDir ndDir(subDir.filePath(nd));
            QStringList ndFiles = ndDir.entryList(QStringList() << "*.slangp", QDir::Files, QDir::Name);
            for (const QString& f : ndFiles)
            {
                slangpFiles.append(nd + "/" + f);
            }
        }

        if (slangpFiles.isEmpty()) continue;

        auto* categoryItem = new QTreeWidgetItem();
        categoryItem->setText(0, sub.toUpper());
        categoryItem->setIcon(0, QIcon());

        int matchCount = 0;
        for (const QString& relFile : slangpFiles)
        {
            QString fullPath = subDir.filePath(relFile);
            QFileInfo fi(fullPath);

            if (!filter.isEmpty() && !fi.fileName().toLower().contains(filter) && !sub.toLower().contains(filter))
            {
                continue;
            }

            auto* childItem = new QTreeWidgetItem(categoryItem);
            childItem->setText(0, fi.fileName());
            childItem->setText(1, relFile);
            childItem->setData(0, Qt::UserRole, fullPath);
            matchCount++;
        }

        if (matchCount > 0)
        {
            treeWidget->addTopLevelItem(categoryItem);
            categoryItem->setExpanded(!filter.isEmpty());
        }
        else
        {
            delete categoryItem;
        }
    }
}

void WatermelonShaderDialog::onFilterChanged(const QString&)
{
    loadShaderCategories();
}

void WatermelonShaderDialog::onCategorySelectionChanged()
{
    QList<QTreeWidgetItem*> selected = treeWidget->selectedItems();
    if (selected.isEmpty())
    {
        btnApply->setEnabled(false);
        currentSelectedPreset.clear();
        return;
    }

    QTreeWidgetItem* item = selected.first();
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty() && path.endsWith(QStringLiteral(".slangp")))
    {
        currentSelectedPreset = path;
        btnApply->setEnabled(true);
    }
    else
    {
        currentSelectedPreset.clear();
        btnApply->setEnabled(false);
    }
}

void WatermelonShaderDialog::onPresetItemDoubleClicked(QTreeWidgetItem* item, int)
{
    if (!item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty() && path.endsWith(QStringLiteral(".slangp")))
    {
        currentSelectedPreset = path;
        onApplyClicked();
    }
}

void WatermelonShaderDialog::onApplyClicked()
{
    if (currentSelectedPreset.isEmpty()) return;
    setActivePresetPath(currentSelectedPreset);
    updateActivePresetLabel();
    emit shaderPresetChanged(currentSelectedPreset);

    QMessageBox::information(this, QStringLiteral("Shader Aplicado"),
        QStringLiteral("El shader se ha configurado como preset activo:\n%1").arg(QFileInfo(currentSelectedPreset).fileName()));
}

void WatermelonShaderDialog::onDisableClicked()
{
    setActivePresetPath(QString());
    updateActivePresetLabel();
    emit shaderPresetChanged(QString());

    QMessageBox::information(this, QStringLiteral("Shaders Desactivados"),
        QStringLiteral("Se han desactivado los shaders adicionales. El emulador usará el renderizado estándar."));
}

void WatermelonShaderDialog::onDownloadClicked()
{
    btnDownload->setEnabled(false);
    statusLabel->setText(QStringLiteral("Descargando catálogo de shaders desde libretro.com..."));
    progressBar->setVisible(true);
    progressBar->setValue(0);

    QNetworkRequest request(QUrl(QString::fromUtf8(SHADER_DOWNLOAD_URL)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("WatermelonDS-Linux"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, &WatermelonShaderDialog::onDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, &WatermelonShaderDialog::onDownloadFinished);
}

void WatermelonShaderDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0)
    {
        int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        progressBar->setValue(percent);
        statusLabel->setText(QStringLiteral("Descargando: %1 MB / %2 MB (%3%)")
            .arg(bytesReceived / (1024.0 * 1024.0), 0, 'f', 1)
            .arg(bytesTotal / (1024.0 * 1024.0), 0, 'f', 1)
            .arg(percent));
    }
}

void WatermelonShaderDialog::onDownloadFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        statusLabel->setText(QStringLiteral("❌ Error al descargar: %1").arg(reply->errorString()));
        btnDownload->setEnabled(true);
        progressBar->setVisible(false);
        return;
    }

    QByteArray data = reply->readAll();
    QString zipPath = shadersPath + "/shaders_slang.zip";

    QFile file(zipPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        statusLabel->setText(QStringLiteral("❌ No se pudo guardar el archivo zip temporal."));
        btnDownload->setEnabled(true);
        progressBar->setVisible(false);
        return;
    }
    file.write(data);
    file.close();

    statusLabel->setText(QStringLiteral("Extrayendo shaders..."));
    progressBar->setValue(100);

    // Extract using unzip
    QProcess process;
    process.start(QStringLiteral("unzip"), QStringList() << QStringLiteral("-q") << QStringLiteral("-o") << zipPath << QStringLiteral("-d") << shadersPath);
    if (!process.waitForFinished(60000))
    {
        statusLabel->setText(QStringLiteral("❌ Tiempo de espera agotado al extraer los shaders."));
        btnDownload->setEnabled(true);
        return;
    }

    // Clean up zip
    QFile::remove(zipPath);

    downloadBanner->setVisible(false);
    loadShaderCategories();
    updateActivePresetLabel();

    QMessageBox::information(this, QStringLiteral("Instalación Completa"),
        QStringLiteral("¡El catálogo de shaders RetroArch se ha instalado correctamente!"));
}
