#ifndef WATERMELONFLOATINGBAR_H
#define WATERMELONFLOATINGBAR_H

#include <QWidget>
#include <functional>

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
    void updateForWindowWidth(int windowWidth);
    void setCompactMode(bool compact);
    bool isCompactMode() const { return isCompact; }

    // Returns a short description (e.g. date/time) for the given state slot,
    // or an empty string if the slot is empty.
    void setStateSlotInfoProvider(std::function<QString(int)> provider);

signals:
    void pauseRequested(bool pause);
    void frameStepRequested();
    void resetRequested();
    void stopRequested();
    void settingsRequested();
    void fullscreenRequested();
    void saveStateRequested(int slot);
    void loadStateRequested(int slot);
    void undoStateLoadRequested();
    void undoStateSaveRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void onPauseClicked();
    void onFrameStepClicked();
    void onResetClicked();
    void onStopClicked();
    void onSaveStateClicked();
    void onLoadStateClicked();

private:
    void refreshButtonTexts();

    QLabel*      lblStatus        = nullptr;
    QPushButton* btnPause         = nullptr;
    QPushButton* btnFrameStep     = nullptr;
    QPushButton* btnReset         = nullptr;
    QPushButton* btnSaveState     = nullptr;
    QPushButton* btnLoadState     = nullptr;
    QPushButton* btnStop          = nullptr;
    QPushButton* btnSettings      = nullptr;
    QPushButton* btnFullscreen    = nullptr;

    QTimer* hideTimer = nullptr;
    bool isPaused = false;
    bool isCompact = false;
    bool initialized = false;
    bool stateMenuOpen = false;
    std::function<QString(int)> stateSlotInfoProvider;
};

#endif // WATERMELONFLOATINGBAR_H
