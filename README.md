# 🍉 WatermelonDS (Desktop Edition)

<div align="center">
  <img src="res/icon/melon_256x256.png" width="120" height="120" alt="WatermelonDS Logo">
  <h3>Un emulador de Nintendo DS & DSi moderno, elegante y potente para Linux y Windows.</h3>

  <p>
    <a href="https://github.com/Amnesia1000/WatermelonDS/releases"><img src="https://img.shields.io/github/v/release/Amnesia1000/WatermelonDS?style=for-the-badge&color=6fbf4a&label=Release" alt="Latest Release"></a>
    <a href="https://github.com/Amnesia1000/WatermelonDS/actions/workflows/build-windows.yml"><img src="https://img.shields.io/github/actions/workflow/status/Amnesia1000/WatermelonDS/build-windows.yml?style=for-the-badge&label=Windows%20CI" alt="Windows Build"></a>
    <a href="https://github.com/Amnesia1000/WatermelonDS/actions/workflows/build-ubuntu.yml"><img src="https://img.shields.io/github/actions/workflow/status/Amnesia1000/WatermelonDS/build-ubuntu.yml?style=for-the-badge&label=Linux%20CI" alt="Linux Build"></a>
    <img src="https://img.shields.io/badge/Platforms-Linux%20%7C%20Windows-blue?style=for-the-badge" alt="Platforms">
    <img src="https://img.shields.io/badge/Framework-Qt%206%20%7C%20C%2B%2B17-blueviolet?style=for-the-badge" alt="Framework">
    <img src="https://img.shields.io/badge/License-GPLv3-yellow.svg?style=for-the-badge" alt="License">
  </p>
</div>

---

## 📖 Acerca del Proyecto / About the Project

**WatermelonDS Desktop** es una reimaginación moderna del frontend del emulador de Nintendo DS y Nintendo DSi para ordenadores de escritorio.

Este proyecto nace combinando lo mejor de dos mundos:
1. **El motor de emulación de [melonDS](https://github.com/melonDS-emu/melonDS)**: Desarrollado por *StapleButter* y el equipo de melonDS, reconocido por su gran precisión, soporte de DSi, emulación Wi-Fi/LAN, JIT y motores de renderizado 3D (OpenGL, Software y cómputo).
2. **La filosofía de diseño y experiencia de usuario de [WatermelonDS](https://github.com/SapphireRhodonite/melonDS-android-lib)**: Basado e inspirado en la versión para Android de *SapphireRhodonite* y *rafaelvcaetano* ([melonDS Android](https://github.com/rafaelvcaetano/melonDS-android)), trasladando la experiencia visual fluida, la biblioteca con descarga de carátulas en alta resolución y una navegación limpia e intuitiva al escritorio mediante **C++17 y Qt 6**.

---

## ✨ Características Principales

### 🍉 Biblioteca Visual y Lanzador de Juegos (Game Hub)
- **Cuadrícula con carátulas automáticas**: Descarga y muestra automáticamente portadas de alta definición de tu colección de juegos.
- **Estantería «Continuar Jugando»**: Acceso inmediato con un clic a los títulos jugados recientemente.
- **Buscador en tiempo real**: Encuentra tus juegos al instante buscando por título, nombre de archivo o código de juego (Game Code).
- **Menú contextual rápido**: Clic derecho en cualquier carátula para Iniciar, Configurar opciones específicas, Gestionar trucos o Consultar logros.

### 🕹️ Barra Flotante In-Game (HUD)
- **Pantalla limpia al 100%**: Se elimina la barra de menús tradicional (`Menu Bar`) para que los juegos ocupen todo el viewport sin barras estorbando.
- **Controles automáticos al pasar el ratón**: Al acercar el cursor al borde superior de la ventana (`y <= 60px`), aparece suavemente una elegante barra flotante en forma de píldora translúcida con:
  - ⏸ **Pausar / ▶ Continuar**: Pausa o reanuda la emulación inmediatamente con icono dinámico.
  - ⏭ **Paso a paso (Frame Step)**: Avanza la emulación fotograma a fotograma.
  - 🔄 **Reiniciar**: Resetea el juego en ejecución.
  - ⏹ **Detener**: Finaliza la partida de forma segura y vuelve fluidamente a la biblioteca.
  - ⚙ **Ajustes**: Abre la configuración unificada directamente.
  - ⛶ **Pantalla completa**: Alterna entre pantalla completa y ventana.
- **Ocultación inteligente**: Se desvanece de forma automática tras unos segundos de inactividad del ratón.

### ⚙️ Diálogo de Ajustes Unificado (Modern Settings)
- **Diseño lateral moderno**: Inspirado en emuladores contemporáneos (Ryujinx, Yuzu, Lichii), agrupando todas las opciones de forma limpia en pestañas laterales:
  - **General**: Idioma del firmware, nombre de usuario, mensaje personal y selector de color favorito con muestras de color visuales.
  - **Sistema**: Tipo de consola (DS / DSi), selector gráfico de BIOS y Firmware personalizados, y tamaño de bloque JIT.
  - **Gráficos**: Motor 3D (OpenGL, Vulkan, Software), escalado interno, filtro de texturas y sincronización vertical.
  - **Audio & Micrófono**: Dispositivos de salida, volumen, interpolación y fuentes de micrófono (ruido blanco, patrón o micrófono real).
  - **Mando & Controles**: Interfaz visual de mapeo interactivo replicando la silueta original de la Nintendo DS con soporte para teclado y mandos vía SDL2.
  - **Logros (RetroAchievements)**: Configuración completa de Hardcore Mode, Rich Presence, sonidos y logros no oficiales.

### 👾 Pestaña de Trucos (Action Replay Cheats)
- **Configuración individual por juego**: Cada ROM cuenta con su propia gestión de trucos en los detalles del juego.
- **Interruptor maestro**: Activa o desactiva la aplicación de trucos para el juego actual.
- **Árbol jerárquico**: Lista ordenada de categorías y códigos Action Replay con casillas de verificación para activar solo los trucos deseados.
- **➕ Añadir truco manual**: Formulario integrado con validación hexadecimal para agregar códigos personalizados fácilmente.
- **📥 Importar base de datos (`usrcheat.dat`)**: Carga archivos de base de datos R4 / Action Replay; WatermelonDS detecta de forma automática los trucos correspondientes al código de la ROM.

### 🏆 RetroAchievements
- Acceso desde la ventana principal con un diálogo de resumen de perfil de usuario (avatar, puntos Hardcore/Softcore, rango y estadísticas).
- Lista detallada de logros por juego, con estado de desbloqueo, insignias y progreso.

### 🎨 Shaders de Post-Procesado
- Compatibilidad con presets de shaders de RetroArch (`.slangp` y `.glslp`).
- Selector visual de shaders con previsualización en vivo y configuración global o individual por juego.

---

## ⚖️ ¿Qué ofrecemos a diferencia de melonDS clásico y WatermelonDS Android?

### 🆚 Frente a melonDS clásico (Desktop):
| Característica | melonDS Clásico | 🍉 WatermelonDS Desktop |
| :--- | :---: | :---: |
| **Lanzador visual de juegos (Game Hub)** | ❌ (Solo diálogo de archivos) | ✅ Cuadrícula con carátulas automáticas en alta resolución y estantería "Continuar jugando" |
| **Escalado dinámico de carátulas** | ❌ | ✅ Slider de zoom interactivo en vivo (100px – 260px) |
| **Experiencia visual inmersiva** | ❌ (Barra de menús gris superior fija) | ✅ 100% libre de barras de menú clásicas; inmersión total en emulación y pantalla completa |
| **Barra de control in-game (HUD)** | ❌ | ✅ Píldora flotante que aparece al acercar el cursor con modo compacto responsive para cualquier tamaño de ventana |
| **Ajustes centralizados** | ❌ (Menús dispersos y ventanas modales) | ✅ Diálogo unificado y moderno por categorías (General, Sistema, Gráficos, Audio, Mandos, Rutas, Shaders, Logros) |
| **Mapeo visual de mandos** | ❌ (Listas de texto desplegables) | ✅ Silueta interactiva de Nintendo DS para asignar botones de mando y teclado visualmente |
| **RetroAchievements** | ❌ (Sin integración nativa) | ✅ Totalmente integrado: hashing MD5 oficial (rcheevos), insignias de trofeo en carátulas y panel de perfil |
| **Gestión de trucos por juego** | ❌ (Global/rudimentaria) | ✅ Pestaña dedicada por juego con códigos manuales e importación de bases de datos R4 (`usrcheat.dat`) |
| **Soporte multilingüe** | ❌ (Solo inglés) | ✅ Español e Inglés con selector dinámico en ajustes |

### 🆚 Frente a WatermelonDS original (Android):
- 🖥️ **Rendimiento Nativo en Ordenadores de Escritorio**: Construido desde cero en **C++17 y Qt 6** nativo para Linux y Windows, sin depender de máquinas virtuales Java/Kotlin ni capas de emulación móvil.
- 🎮 **Soporte Extensivo para Periféricos de PC**: Soporte para mandos de Xbox, PlayStation, Nintendo Switch, mandos genéricos y teclado completo con remapeo visual interactivo.
- 🖥️ **Experiencia Multiventana y Multi-monitor**: Ventana escalable libremente, modo pantalla completa sin bordes y compatibilidad con pipelines de shaders RetroArch (`.slangp` y `.glslp`).
- 📦 **Empaquetado Nativo para Escritorio**: Disponible en formatos **AppImage**, **Flatpak**, **Tarball** para Linux y **ZIP portable** para Windows.

---

## 🚀 Registro de Cambios — v1.0.0 (Versión Inicial Funcional)

- **Lanzador Visual Completo**: Cuadrícula de juegos con descarga automática de portadas, estantería de títulos recientes, badges de trofeo dorado RetroAchievements y etiqueta DS/DSi.
- **Slider de Tamaño de Carátulas**: Deslizador interactivo en la cabecera de la biblioteca para escalar el tamaño de las portadas dinámicamente según la preferencia del usuario.
- **Eliminación Total de la Barra de Menú Clásica**: La interfaz ahora aprovecha el 100% del área de juego con todos los atajos de teclado preservados (`Ctrl+O`, `F1-F8`, `F11`, etc.).
- **Barra Flotante In-Game Responsiva**: HUD flotante superior con modo compacto para ventanas pequeñas (oculta texto y optimiza iconos para ventanas de 256px o 1x nativo sin desbordarse).
- **Diálogo de Ajustes Unificado**: Portabilidad del 100% de las opciones de melonDS a una interfaz categorizada moderna con silueta gráfica para configuración de controles.
- **Integración con RetroAchievements**: Autenticación, visualización de estado de logros por juego, cálculo de hash MD5 según estándar rcheevos y diálogo de resumen de perfil.
- **Trucos Action Replay**: Pestaña dedicada por juego, validación hexadecimal e importador de `usrcheat.dat`.
- **Compatibilidad de Compilación Multiplataforma**: Soporte limpio en Linux (Ubuntu 22.04 LTS / Qt 6.2+) y Windows (MSYS2 UCRT64 / MinGW).

---

## 📥 Descargas / Downloads

Puedes descargar la última versión compilada para tu sistema operativo desde la sección de **[Releases](https://github.com/Amnesia1000/WatermelonDS/releases)**:

| Plataforma | Formato | Notas |
| :--- | :--- | :--- |
| 🐧 **Linux** (x86_64) | `.AppImage` | Paquete autónomo ejecutable ("doble clic y jugar"). |
| 🐧 **Linux** (x86_64) | `.flatpak` | Bundle Flatpak aislado para cualquier distribución. |
| 🐧 **Linux** (x86_64) | `.tar.gz` | Binario portable para Linux. |
| 🪟 **Windows** (x86_64) | `.zip` (Portable) | Descomprimir y ejecutar `WatermelonDS.exe`. |

---

## 🛠️ Compilación desde el código fuente / Building from Source

### Requisitos previos
- Compilador con soporte para **C++17** (GCC 9+, Clang 10+, MSVC 2019+).
- **CMake** 3.16 o superior y **Ninja** (o Make).
- **Qt 6** (o Qt 5) con módulos: `Core`, `Gui`, `Widgets`, `Network`, `Multimedia`, `OpenGL`, `OpenGLWidgets`, `Svg`.
- **SDL2**, **libarchive**, **libzstd**, **faad2**, **enet**.

### En Linux (Ubuntu / Debian / Linux Mint):
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git \
  qt6-base-dev qt6-base-private-dev qt6-multimedia-dev libqt6svg6-dev \
  libsdl2-dev libarchive-dev libzstd-dev libfaad-dev libenet-dev libpcap-dev

git clone https://github.com/Amnesia1000/WatermelonDS.git
cd WatermelonDS
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Ejecutar
./build/melonDS
```

### En Linux (Arch Linux / Manjaro):
```bash
sudo pacman -S base-devel cmake ninja git qt6-base qt6-multimedia qt6-svg sdl2 libarchive zstd faad2 enet libpcap

git clone https://github.com/Amnesia1000/WatermelonDS.git
cd WatermelonDS
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### En Windows (MSYS2 UCRT64 / MinGW):
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-multimedia mingw-w64-ucrt-x86_64-qt6-svg \
  mingw-w64-ucrt-x86_64-sdl2 mingw-w64-ucrt-x86_64-libarchive mingw-w64-ucrt-x86_64-zstd

git clone https://github.com/Amnesia1000/WatermelonDS.git
cd WatermelonDS
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## 🤝 Créditos y Agradecimientos

- **[melonDS](https://github.com/melonDS-emu/melonDS)**: Creado por **StapleButter** (Arisotura) y todos los increíbles colaboradores de melonDS.
- **[WatermelonDS / melonDS-android](https://github.com/rafaelvcaetano/melonDS-android)**: Desarrollado por **rafaelvcaetano** y **[SapphireRhodonite](https://github.com/SapphireRhodonite/melonDS-android-lib)**, cuya interfaz y experiencia inspiraron esta versión de escritorio.
- **[RetroAchievements](https://retroachievements.org/)**: Por su API y plataforma abierta para la comunidad retro.
- Todos los autores de emuladores modernos cuyas interfaces inspiraron los componentes visuales de este proyecto.

---

## 📄 Licencia

Este proyecto está licenciado bajo los términos de la **GNU General Public License v3.0 (GPLv3)**. Consulta el archivo [LICENSE](LICENSE) para más detalles.
