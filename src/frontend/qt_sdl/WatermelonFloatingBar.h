#ifndef WATERMELONFLOATINGBAR_H
#define WATERMELONFLOATINGBAR_H

#include <QWidget>

class QLabel;
class QPushButton;
class QTimer;
class QGraphicsOpacityEffect;

class WatermelonFloatingBar : public QWidget
{
    Q_OBJECT

public:
    explicit WatermelonFloatingBar(QWidget* parent = nullptr);
    ~WatermelonFloatingBar() override = default;

    void setEmuPaused(bool paused);
    void showBar();
    void hideBar();
    void pingHover();

signals:
    void pauseRequested(bool pause);
    void frameStepRequested();
    void resetRequested();
    void stopRequested();
    void settingsRequested();
    void fullscreenRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void onPauseClicked();
    void onFrameStepClicked();
    void onResetClicked();
    void onStopClicked();

private:
    QLabel*      lblStatus        = nullptr;
    QPushButton* btnPause         = nullptr;
    QPushButton* btnFrameStep     = nullptr;
    QPushButton* btnReset         = nullptr;
    QPushButton* btnStop          = nullptr;
    QPushButton* btnSettings      = nullptr;
    QPushButton* btnFullscreen    = nullptr;

    QTimer* hideTimer = nullptr;
    bool isPaused = false;
};

#endif // WATERMELONFLOATINGBAR_H
