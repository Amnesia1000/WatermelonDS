#include "WatermelonFloatingBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QFrame>
#include <QIcon>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

WatermelonFloatingBar::WatermelonFloatingBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("watermelonFloatingBar"));
    setAttribute(Qt::WA_NativeWindow, true);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::NoFocus);

    setStyleSheet(QStringLiteral(
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
        "  padding: 5px 12px;"
        "  font-size: 12px;"
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
        "}"
    ));

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(10, 6, 10, 6);
    lay->setSpacing(8);

    lblStatus = new QLabel(tr("🟢 <b style='color:#6fbf4a;'>JUGANDO</b>"), this);
    lblStatus->setStyleSheet(QStringLiteral("font-size: 11px; padding: 0 4px; background: transparent;"));
    lay->addWidget(lblStatus);

    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet(QStringLiteral("color: #333242;"));
    lay->addWidget(sep1);

    btnPause = new QPushButton(this);
    btnPause->setIcon(QIcon(QStringLiteral(":/pause-icon")));
    btnPause->setIconSize(QSize(13, 13));
    btnPause->setText(tr("  Pausar"));
    btnPause->setToolTip(tr("Pausar o reanudar emulación"));
    btnPause->setCursor(Qt::PointingHandCursor);
    connect(btnPause, &QPushButton::clicked, this, &WatermelonFloatingBar::onPauseClicked);
    lay->addWidget(btnPause);

    btnFrameStep = new QPushButton(this);
    btnFrameStep->setIcon(QIcon(QStringLiteral(":/step-icon")));
    btnFrameStep->setIconSize(QSize(13, 13));
    btnFrameStep->setText(tr("  Paso"));
    btnFrameStep->setToolTip(tr("Avanzar un frame (Frame Step)"));
    btnFrameStep->setCursor(Qt::PointingHandCursor);
    connect(btnFrameStep, &QPushButton::clicked, this, &WatermelonFloatingBar::onFrameStepClicked);
    lay->addWidget(btnFrameStep);

    btnReset = new QPushButton(this);
    btnReset->setIcon(QIcon(QStringLiteral(":/refresh-icon")));
    btnReset->setIconSize(QSize(13, 13));
    btnReset->setText(tr("  Reiniciar"));
    btnReset->setToolTip(tr("Reiniciar juego"));
    btnReset->setCursor(Qt::PointingHandCursor);
    connect(btnReset, &QPushButton::clicked, this, &WatermelonFloatingBar::onResetClicked);
    lay->addWidget(btnReset);

    btnStop = new QPushButton(this);
    btnStop->setObjectName(QStringLiteral("btnStop"));
    btnStop->setIcon(QIcon(QStringLiteral(":/stop-icon")));
    btnStop->setIconSize(QSize(12, 12));
    btnStop->setText(tr("  Detener"));
    btnStop->setToolTip(tr("Detener emulación y volver al menú"));
    btnStop->setCursor(Qt::PointingHandCursor);
    connect(btnStop, &QPushButton::clicked, this, &WatermelonFloatingBar::onStopClicked);
    lay->addWidget(btnStop);

    auto* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setStyleSheet(QStringLiteral("color: #333242;"));
    lay->addWidget(sep2);

    btnSettings = new QPushButton(tr("⚙ Ajustes"), this);
    btnSettings->setToolTip(tr("Configuración"));
    btnSettings->setCursor(Qt::PointingHandCursor);
    connect(btnSettings, &QPushButton::clicked, this, &WatermelonFloatingBar::settingsRequested);
    lay->addWidget(btnSettings);

    btnFullscreen = new QPushButton(this);
    btnFullscreen->setIcon(QIcon(QStringLiteral(":/fullscreen-icon")));
    btnFullscreen->setIconSize(QSize(13, 13));
    btnFullscreen->setText(tr("  Pantalla"));
    btnFullscreen->setToolTip(tr("Pantalla completa"));
    btnFullscreen->setCursor(Qt::PointingHandCursor);
    connect(btnFullscreen, &QPushButton::clicked, this, &WatermelonFloatingBar::fullscreenRequested);
    lay->addWidget(btnFullscreen);

    adjustSize();
    hide();

    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &WatermelonFloatingBar::hideBar);
}

void WatermelonFloatingBar::setEmuPaused(bool paused)
{
    isPaused = paused;
    if (isPaused) {
        lblStatus->setText(tr("⏸ <b style='color:#f1c40f;'>PAUSADO</b>"));
        btnPause->setIcon(QIcon(QStringLiteral(":/play-icon")));
        btnPause->setText(tr("  Continuar"));
    } else {
        lblStatus->setText(tr("🟢 <b style='color:#6fbf4a;'>JUGANDO</b>"));
        btnPause->setIcon(QIcon(QStringLiteral(":/pause-icon")));
        btnPause->setText(tr("  Pausar"));
    }
}

void WatermelonFloatingBar::showBar()
{
    show();
    raise();
    hideTimer->start(2500);
}

void WatermelonFloatingBar::hideBar()
{
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

void WatermelonFloatingBar::onStopClicked()
{
    emit stopRequested();
}
