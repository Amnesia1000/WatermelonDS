#include "WatermelonTheme.h"

#include <QApplication>
#include <QPalette>
#include <QColor>

namespace WatermelonTheme
{

QString getStyleSheet()
{
    return QStringLiteral(R"(
        /* === WatermelonDS Base Styling === */
        QWidget {
            background-color: #141317;
            color: #f6f5f3;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            font-size: 13px;
            selection-background-color: #6fbf4a;
            selection-color: #141317;
        }

        QMainWindow, QDialog {
            background-color: #141317;
        }

        /* === Menu Bar & Menus === */
        QMenuBar {
            background-color: #141317;
            color: #d0ced8;
            border: none;
            padding: 4px 12px;
            font-size: 13px;
            font-weight: 500;
        }

        QMenuBar::item {
            background-color: transparent;
            padding: 5px 12px;
            border-radius: 6px;
            margin-right: 4px;
        }

        QMenuBar::item:selected {
            background-color: #22212b;
            color: #ffffff;
        }

        QMenuBar::item:pressed {
            background-color: #2b1921;
            color: #ff5477;
        }

        QMenu {
            background-color: #1a1922;
            color: #f6f5f3;
            border: 1px solid #2d2c39;
            border-radius: 8px;
            padding: 6px;
        }

        QMenu::item {
            padding: 7px 24px 7px 12px;
            border-radius: 6px;
            font-size: 13px;
            font-weight: 500;
        }

        QMenu::item:selected {
            background-color: #272533;
            color: #ff5477;
        }

        QMenu::separator {
            height: 1px;
            background-color: #292835;
            margin: 4px 6px;
        }

        /* === Buttons === */
        QPushButton {
            background-color: #1e1d24;
            color: #f6f5f3;
            border: 1px solid #2e2d38;
            border-radius: 8px;
            padding: 7px 16px;
            font-weight: 600;
        }

        QPushButton:hover {
            background-color: #272630;
            border-color: #6fbf4a;
            color: #6fbf4a;
        }

        QPushButton:pressed {
            background-color: #141317;
            border-color: #559c36;
        }

        QPushButton:disabled {
            background-color: #16151a;
            color: #5b5962;
            border-color: #222129;
        }

        QPushButton[primary="true"] {
            background-color: #6fbf4a;
            color: #141317;
            border: none;
            font-weight: 700;
        }

        QPushButton[primary="true"]:hover {
            background-color: #7dd254;
        }

        QPushButton[primary="true"]:pressed {
            background-color: #5da53d;
        }

        QPushButton[danger="true"] {
            background-color: #f44336;
            color: #ffffff;
            border: none;
        }

        QPushButton[danger="true"]:hover {
            background-color: #ff574a;
        }

        /* === Inputs & Edit Controls === */
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #1a1921;
            color: #f6f5f3;
            border: 1px solid #2e2d38;
            border-radius: 6px;
            padding: 2px 8px;
            min-height: 24px;
        }

        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus, QComboBox:focus {
            border: 1px solid #6fbf4a;
            background-color: #1e1d26;
        }

        /* === ComboBox Dropdown === */
        QComboBox {
            padding-right: 22px;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 22px;
            border: none;
            background: transparent;
        }

        QComboBox::down-arrow {
            image: url(:/arrow-down);
            width: 10px;
            height: 10px;
        }

        QComboBox QAbstractItemView {
            background-color: #1a1921;
            border: 1px solid #2e2d38;
            border-radius: 8px;
            selection-background-color: #25242c;
            selection-color: #6fbf4a;
        }

        /* === SpinBox Arrows === */
        QSpinBox, QDoubleSpinBox {
            padding-right: 20px;
        }

        QSpinBox::up-button, QDoubleSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 18px;
            border-left: 1px solid #2e2d38;
            border-bottom: 1px solid #2e2d38;
            border-top-right-radius: 5px;
            background: #1c1b22;
        }

        QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover {
            background: #282732;
        }

        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
            image: url(:/arrow-up);
            width: 8px;
            height: 8px;
        }

        QSpinBox::down-button, QDoubleSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 18px;
            border-left: 1px solid #2d2c38;
            border-bottom-right-radius: 5px;
            background: #1c1b22;
        }

        QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
            background: #282732;
        }

        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
            image: url(:/arrow-down-small);
            width: 8px;
            height: 8px;
        }

        /* === Checkboxes & Radio Buttons === */
        QCheckBox, QRadioButton {
            color: #f6f5f3;
            spacing: 8px;
        }

        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 5px;
            border: 1px solid #2e2d38;
            background-color: #1a1921;
        }

        QCheckBox::indicator:hover {
            border-color: #6fbf4a;
        }

        QCheckBox::indicator:checked {
            background-color: #6fbf4a;
            border-color: #6fbf4a;
            image: url(:/check-icon);
        }

        /* === Tab Widget === */
        QTabWidget::pane {
            border: 1px solid #2e2d38;
            border-radius: 8px;
            background-color: #1a1921;
            padding: 8px;
        }

        QTabBar::tab {
            background-color: #141317;
            color: #b3b1bd;
            padding: 8px 16px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            margin-right: 4px;
        }

        QTabBar::tab:selected {
            background-color: #1a1921;
            color: #6fbf4a;
            border-bottom: 2px solid #6fbf4a;
            font-weight: 600;
        }

        QTabBar::tab:hover:!selected {
            color: #f6f5f3;
            background-color: #1e1d24;
        }

        /* === Group Box === */
        QGroupBox {
            border: 1px solid #2e2d38;
            border-radius: 10px;
            margin-top: 14px;
            padding-top: 12px;
            font-weight: 600;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 14px;
            padding: 0 6px;
            color: #6fbf4a;
        }

        /* === Scrollbars === */
        QScrollBar:vertical {
            background: #141317;
            width: 10px;
            margin: 0;
            border-radius: 5px;
        }

        QScrollBar::handle:vertical {
            background: #2e2d38;
            min-height: 25px;
            border-radius: 5px;
        }

        QScrollBar::handle:vertical:hover {
            background: #6fbf4a;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
            height: 0;
        }

        QScrollBar:horizontal {
            background: #141317;
            height: 10px;
            margin: 0;
            border-radius: 5px;
        }

        QScrollBar::handle:horizontal {
            background: #2e2d38;
            min-width: 25px;
            border-radius: 5px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #6fbf4a;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
            width: 0;
        }

        /* === Sliders === */
        QSlider::groove:horizontal {
            height: 6px;
            background: #25242c;
            border-radius: 3px;
        }

        QSlider::sub-page:horizontal {
            background: #6fbf4a;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background: #f6f5f3;
            border: 2px solid #6fbf4a;
            width: 16px;
            margin-top: -5px;
            margin-bottom: -5px;
            border-radius: 8px;
        }

        QSlider::handle:horizontal:hover {
            background: #6fbf4a;
            border-color: #ffffff;
        }

        /* === Status Bar === */
        QStatusBar {
            background-color: #141317;
            color: #b3b1bd;
            border-top: 1px solid #25242c;
        }

        /* === Tooltips === */
        QToolTip {
            background-color: #1e1d24;
            color: #f6f5f3;
            border: 1px solid #3e3d4a;
            padding: 4px 8px;
            font-size: 11px;
        }
    )");
}

void apply(QApplication* app)
{
    if (!app) return;

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x14, 0x13, 0x17));
    palette.setColor(QPalette::WindowText, QColor(0xf6, 0xf5, 0xf3));
    palette.setColor(QPalette::Base, QColor(0x1a, 0x19, 0x21));
    palette.setColor(QPalette::AlternateBase, QColor(0x22, 0x21, 0x2b));
    palette.setColor(QPalette::ToolTipBase, QColor(0x1e, 0x1d, 0x24));
    palette.setColor(QPalette::ToolTipText, QColor(0xf6, 0xf5, 0xf3));
    palette.setColor(QPalette::Text, QColor(0xf6, 0xf5, 0xf3));
    palette.setColor(QPalette::Button, QColor(0x1e, 0x1d, 0x24));
    palette.setColor(QPalette::ButtonText, QColor(0xf6, 0xf5, 0xf3));
    palette.setColor(QPalette::BrightText, QColor(0xf4, 0x43, 0x36));
    palette.setColor(QPalette::Highlight, QColor(0x6f, 0xbf, 0x4a));
    palette.setColor(QPalette::HighlightedText, QColor(0x14, 0x13, 0x17));
    palette.setColor(QPalette::PlaceholderText, QColor(0x75, 0x73, 0x80));

    app->setPalette(palette);
    app->setStyleSheet(getStyleSheet());
}

}
