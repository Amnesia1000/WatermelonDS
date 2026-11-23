#!/usr/bin/env bash
# Regenerates the .ts files from the source strings and recompiles the .qm
# files embedded in the resource file.
#
# Usage: ./scripts/update-translations.sh
# Set QT_BIN to override the Qt bin directory (it must contain lupdate/lrelease).
set -euo pipefail
cd "$(dirname "$0")/.."

find_tool() {
    local name="$1"
    if [ -n "${QT_BIN:-}" ] && [ -x "$QT_BIN/$name" ]; then
        echo "$QT_BIN/$name"
    elif command -v "$name" >/dev/null 2>&1; then
        command -v "$name"
    else
        local candidate
        for candidate in /usr/lib/qt6/bin/"$name" /usr/lib/qt5/bin/"$name"; do
            if [ -x "$candidate" ]; then
                echo "$candidate"
                return 0
            fi
        done
        echo "error: $name not found (set QT_BIN)" >&2
        return 1
    fi
}

LUPDATE="$(find_tool lupdate)"
LRELEASE="$(find_tool lrelease)"

SOURCES=(
    src/frontend/qt_sdl/WatermelonLauncher.cpp
    src/frontend/qt_sdl/WatermelonSettingsDialog.cpp
    src/frontend/qt_sdl/WatermelonGameDialog.cpp
    src/frontend/qt_sdl/WatermelonRADialog.cpp
    src/frontend/qt_sdl/WatermelonShaderDialog.cpp
    src/frontend/qt_sdl/WatermelonFloatingBar.cpp
)

mkdir -p res/i18n

"$LUPDATE" "${SOURCES[@]}" -ts translations/watermelon_es.ts translations/watermelon_en.ts

for lang in es en; do
    "$LRELEASE" "translations/watermelon_${lang}.ts" -qm "res/i18n/watermelon_${lang}.qm"
done
