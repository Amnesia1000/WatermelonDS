#pragma once

#include <QString>

class QApplication;

namespace WatermelonTheme
{
    QString getStyleSheet();
    void apply(QApplication* app);
}
