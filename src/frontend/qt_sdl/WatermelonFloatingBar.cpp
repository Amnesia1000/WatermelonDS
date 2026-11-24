#include "WatermelonFloatingBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QFrame>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

static const char* kFloatingBarBaseStyle =
    "QWidget#watermelonFloatingBar {"
    "  background-color: #14131b;"
    "  border: 1px solid #6fbf4a;"
    "  border-radius: 14px;"
    "}"
    "QPushButton {"
    "  background-color: #232230;"
    "  color: #f6f5f3;"
    "  border: 1px solid #363548;"
    "  border-radius: 8px;"
    "  font-weight: 600;"
    "}"
    "QPushButton:hover {"
    "  background-color: #2f2d40;"
    "  border-color: #6fbf4a;"
    "  color: #6fbf4a;"
    "}"
    "QPushButton:pressed {"
    "  background-color: #1a1924;"
    "}"
    "QPushButton#btnStop:hover {"
    "  background-color: #3d1b26;"
    "  border-color: #ff5477;"
    "  color: #ff5477;"
    "}";

WatermelonFloatingBar::WatermelonFloatingBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("watermelonFloatingBar"));
    setAttribute(Qt::WA_NativeWindow, true);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::NoFocus);

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(10, 6, 10, 6);
    lay->setSpacing(8);

    lblStatus = new QLabel(this);
    lblStatus->setStyleSheet(QStringLiteral("font-size: 11px; padding: 0 4px; background: transparent;"));
    lay->addWidget(lblStatus);

    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet(QStringLiteral("color: #333242;"));
    lay->addWidget(sep1);

    btnPause = new QPushButton(this);
    btnPause->setIcon(QIcon(QStringLiteral(":/pause-icon")));
    btnPause->setIconSize(QSize(13, 13));
    btnPause->setCursor(Qt::PointingHandCursor);
    connect(btnPause, &QPushButton::clicked, this, &WatermelonFloatingBar::onPauseClicked);
    lay->addWidget(btnPause);

    btnFrameStep = new QPushButton(this);
    btnFrameStep->setIcon(QIcon(QStringLiteral(":/step-icon")));
    btnFrameStep->setIconSize(QSize(13, 13));
    btnFrameStep->setCursor(Qt::PointingHandCursor);
    connect(btnFrameStep, &QPushButton::clicked, this, &WatermelonFloatingBar::onFrameStepClicked);
    lay->addWidget(btnFrameStep);

    btnReset = new QPushButton(this);
    btnReset->setIcon(QIcon(QStringLiteral(":/refresh-icon")));
    btnReset->setIconSize(QSize(13, 13));
    btnReset->setCursor(Qt::PointingHandCursor);
    connect(btnReset, &QPushButton::clicked, this, &WatermelonFloatingBar::onResetClicked);
    lay->addWidget(btnReset);

    btnSaveState = new QPushButton(this);
    btnSaveState->setCursor(Qt::PointingHandCursor);
    connect(btnSaveState, &QPushButton::clicked, this, &WatermelonFloatingBar::onSaveStateClicked);
    lay->addWidget(btnSaveState);

    btnLoadState = new QPushButton(this);
    btnLoadState->setCursor(Qt::PointingHandCursor);
    connect(btnLoadState, &QPushButton::clicked, this, &WatermelonFloatingBar::onLoadStateClicked);
    lay->addWidget(btnLoadState);

    btnStop = new QPushButton(this);
    btnStop->setObjectName(QStringLiteral("btnStop"));
    btnStop->setIcon(QIcon(QStringLiteral(":/stop-icon")));
    btnStop->setIconSize(QSize(12, 12));
    btnStop->setCursor(Qt::PointingHandCursor);
    connect(btnStop, &QPushButton::clicked, this, &WatermelonFloatingBar::onStopClicked);
    lay->addWidget(btnStop);

    auto* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setStyleSheet(QStringLiteral("color: #333242;"));
    lay->addWidget(sep2);

    btnSettings = new QPushButton(this);
    btnSettings->setCursor(Qt::PointingHandCursor);
    connect(btnSettings, &QPushButton::clicked, this, &WatermelonFloatingBar::settingsRequested);
    lay->addWidget(btnSettings);

    btnFullscreen = new QPushButton(this);
    btnFullscreen->setIcon(QIcon(QStringLiteral(":/fullscreen-icon")));
    btnFullscreen->setIconSize(QSize(13, 13));
    btnFullscreen->setCursor(Qt::PointingHandCursor);
    connect(btnFullscreen, &QPushButton::clicked, this, &WatermelonFloatingBar::fullscreenRequested);
    lay->addWidget(btnFullscreen);

    setCompactMode(false);
    hide();

    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &WatermelonFloatingBar::hideBar);
}

void WatermelonFloatingBar::setCompactMode(bool compact)
{
    if (initialized && isCompact == compact)
        return;

    isCompact = compact;
    initialized = true;

    if (isCompact)
    {
        setStyleSheet(QString::fromLatin1(kFloatingBarBaseStyle) +
                      QStringLiteral("QPushButton { padding: 4px 6px; font-size: 11px; }"));
    }
    else
    {
        setStyleSheet(QString::fromLatin1(kFloatingBarBaseStyle) +
                      QStringLiteral("QPushButton { padding: 5px 12px; font-size: 12px; }"));
    }

    if (auto* lay = qobject_cast<QHBoxLayout*>(layout()))
    {
        if (isCompact)
        {
            lay->setContentsMargins(6, 4, 6, 4);
            lay->setSpacing(3);
        }
        else
        {
            lay->setContentsMargins(10, 6, 10, 6);
            lay->setSpacing(8);
        }
    }

    refreshButtonTexts();
    adjustSize();
}

void WatermelonFloatingBar::updateForWindowWidth(int windowWidth)
{
    // Threshold: if window is less than 860px wide, use compact icons-only mode
    // so the full-text bar never clips on narrower windows.
    setCompactMode(windowWidth < 860);
}

void WatermelonFloatingBar::refreshButtonTexts()
{
    if (isCompact)
    {
        if (isPaused)
        {
            lblStatus->setText(QStringLiteral("⏸"));
            lblStatus->setToolTip(tr("Pausado"));
            btnPause->setIcon(QIcon(QStringLiteral(":/play-icon")));
            btnPause->setText(QString());
            btnPause->setToolTip(tr("Continuar"));
        }
        else
        {
            lblStatus->setText(QStringLiteral("🟢"));
            lblStatus->setToolTip(tr("Jugando"));
            btnPause->setIcon(QIcon(QStringLiteral(":/pause-icon")));
            btnPause->setText(QString());
            btnPause->setToolTip(tr("Pausar"));
        }

        btnFrameStep->setText(QString());
        btnFrameStep->setToolTip(tr("Avanzar un frame (Frame Step)"));

        btnReset->setText(QString());
        btnReset->setToolTip(tr("Reiniciar juego"));

        btnSaveState->setText(QStringLiteral("💾"));
        btnSaveState->setToolTip(tr("Guardar estado (save state)"));

        btnLoadState->setText(QStringLiteral("📂"));
        btnLoadState->setToolTip(tr("Cargar estado (load state)"));

        btnStop->setText(QString());
        btnStop->setToolTip(tr("Detener emulación y volver al menú"));

        btnSettings->setText(QStringLiteral("⚙"));
        btnSettings->setToolTip(tr("Configuración"));

        btnFullscreen->setText(QString());
        btnFullscreen->setToolTip(tr("Pantalla completa"));
    }
    else
    {
        if (isPaused)
        {
            lblStatus->setText(tr("⏸ <b style='color:#f1c40f;'>PAUSADO</b>"));
            lblStatus->setToolTip(QString());
            btnPause->setIcon(QIcon(QStringLiteral(":/play-icon")));
            btnPause->setText(tr("  Continuar"));
            btnPause->setToolTip(tr("Pausar o reanudar emulación"));
        }
        else
        {
            lblStatus->setText(tr("🟢 <b style='color:#6fbf4a;'>JUGANDO</b>"));
            lblStatus->setToolTip(QString());
            btnPause->setIcon(QIcon(QStringLiteral(":/pause-icon")));
            btnPause->setText(tr("  Pausar"));
            btnPause->setToolTip(tr("Pausar o reanudar emulación"));
        }

        btnFrameStep->setText(tr("  Paso"));
        btnFrameStep->setToolTip(tr("Avanzar un frame (Frame Step)"));

        btnReset->setText(tr("  Reiniciar"));
        btnReset->setToolTip(tr("Reiniciar juego"));

        btnSaveState->setText(tr("💾 Guardar"));
        btnSaveState->setToolTip(tr("Guardar estado (save state)"));

        btnLoadState->setText(tr("📂 Cargar"));
        btnLoadState->setToolTip(tr("Cargar estado (load state)"));

        btnStop->setText(tr("  Detener"));
        btnStop->setToolTip(tr("Detener emulación y volver al menú"));

        btnSettings->setText(tr("⚙ Ajustes"));
        btnSettings->setToolTip(tr("Configuración"));

        btnFullscreen->setText(tr("  Pantalla"));
        btnFullscreen->setToolTip(tr("Pantalla completa"));
    }
}

void WatermelonFloatingBar::setStateSlotInfoProvider(std::function<QString(int)> provider)
{
    stateSlotInfoProvider = std::move(provider);
}

void WatermelonFloatingBar::setEmuPaused(bool paused)
{
    isPaused = paused;
    refreshButtonTexts();
    adjustSize();
}

void WatermelonFloatingBar::showBar()
{
    if (parentWidget())
        updateForWindowWidth(parentWidget()->width());
    show();
    raise();
    hideTimer->start(2500);
}

void WatermelonFloatingBar::hideBar()
{
    if (stateMenuOpen)
        return;
    hide();
}

void WatermelonFloatingBar::pingHover()
{
    if (!isVisible()) {
        showBar();
    } else {
        hideTimer->start(2200);
    }
}

void WatermelonFloatingBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(QStringLiteral("#14131b")));
    p.setBrush(QColor(QStringLiteral("#14131b")));
    p.setPen(QPen(QColor(QStringLiteral("#6fbf4a")), 1.5));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 14, 14);
}

void WatermelonFloatingBar::enterEvent(QEnterEvent* event)
{
    QWidget::enterEvent(event);
    hideTimer->stop();
}

void WatermelonFloatingBar::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    if (!stateMenuOpen)
        hideTimer->start(1800);
}

void WatermelonFloatingBar::onPauseClicked()
{
    emit pauseRequested(!isPaused);
    pingHover();
}

void WatermelonFloatingBar::onFrameStepClicked()
{
    emit frameStepRequested();
    pingHover();
}

void WatermelonFloatingBar::onResetClicked()
{
    emit resetRequested();
    pingHover();
}

void WatermelonFloatingBar::onSaveStateClicked()
{
    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(
        "QMenu { background-color: #1b1a24; color: #f6f5f3; border: 1px solid #363548; padding: 4px; }"
        "QMenu::item { padding: 5px 18px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: #2f2d40; color: #6fbf4a; }"
        "QMenu::separator { height: 1px; background: #363548; margin: 4px 8px; }"
    ));

    for (int i = 1; i <= 8; i++)
    {
        QString info = stateSlotInfoProvider ? stateSlotInfoProvider(i) : QString();
        QString label = info.isEmpty() ? tr("Slot %1 (vacío)").arg(i)
                                       : tr("Slot %1 — %2").arg(i).arg(info);
        QAction* act = menu.addAction(label);
        connect(act, &QAction::triggered, this, [this, i]() { emit saveStateRequested(i); });
    }

    menu.addSeparator();
    QAction* actFile = menu.addAction(tr("Guardar en archivo..."));
    connect(actFile, &QAction::triggered, this, [this]() { emit saveStateRequested(0); });

    QAction* actUndoSave = menu.addAction(tr("Deshacer último guardado"));
    connect(actUndoSave, &QAction::triggered, this, &WatermelonFloatingBar::undoStateSaveRequested);

    stateMenuOpen = true;
    hideTimer->stop();
    menu.exec(btnSaveState->mapToGlobal(QPoint(0, btnSaveState->height())));
    stateMenuOpen = false;
    pingHover();
}

void WatermelonFloatingBar::onLoadStateClicked()
{
    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(
        "QMenu { background-color: #1b1a24; color: #f6f5f3; border: 1px solid #363548; padding: 4px; }"
        "QMenu::item { padding: 5px 18px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: #2f2d40; color: #6fbf4a; }"
        "QMenu::separator { height: 1px; background: #363548; margin: 4px 8px; }"
    ));

    for (int i = 1; i <= 8; i++)
    {
        QString info = stateSlotInfoProvider ? stateSlotInfoProvider(i) : QString();
        QString label = info.isEmpty() ? tr("Slot %1 (vacío)").arg(i)
                                       : tr("Slot %1 — %2").arg(i).arg(info);
        QAction* act = menu.addAction(label);
        connect(act, &QAction::triggered, this, [this, i]() { emit loadStateRequested(i); });
    }

    menu.addSeparator();
    QAction* actFile = menu.addAction(tr("Cargar desde archivo..."));
    connect(actFile, &QAction::triggered, this, [this]() { emit loadStateRequested(0); });

    QAction* actUndo = menu.addAction(tr("Deshacer última carga"));
    connect(actUndo, &QAction::triggered, this, &WatermelonFloatingBar::undoStateLoadRequested);

    stateMenuOpen = true;
    hideTimer->stop();
    menu.exec(btnLoadState->mapToGlobal(QPoint(0, btnLoadState->height())));
    stateMenuOpen = false;
    pingHover();
}

void WatermelonFloatingBar::onStopClicked()
{
    emit stopRequested();
}
