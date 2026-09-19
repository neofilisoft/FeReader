# FeReader 4.0.0
High-performance desktop PDF, EPUB, and Comic Book Viewer and Converter built with modern C++17 and Qt 5.

## Overview
FeReader is a lightweight, responsive document reader engineered with a modular C++ architecture. It features direct integration with the MuPDF C engine for PDF rendering and editing, a custom libzip and gumbo-parser engine for EPUB parsing, native archive extraction via libarchive for Comic Books (.cbz, .cbr, .zip, .rar, .cb7, .cbt), and an ultra-fast two-tier render cache powered by xxHash (XXH3).

## Key Features
- **High-Fidelity PDF Engine**: Powered by MuPDF C API with support for password-protected documents.
- **Native Comic Book Reader**: Read comic and manga archives (.cbz, .cbr, .zip, .rar, .cb7, .cbt) powered by libarchive with natural alphanumeric page sorting.
- **Custom EPUB Engine**: Standalone XHTML/HTML parsing and asset extraction using libzip and gumbo-parser without external runtime dependencies.
- **PDF Editing Suite**:
  - Rotate current page clockwise (Ctrl+R) or counter-clockwise (Ctrl+Shift+R)
  - Delete individual pages (Del)
  - Merge multiple PDF files
  - Extract and split custom page ranges
  - Add text note annotations directly onto PDF pages
  - Save as PDF copy (Ctrl+Shift+S)
- **Print Support**: Native printer and Print-to-PDF support via Qt5::PrintSupport (Ctrl+P) for PDF, EPUB, and Comic books.
- **Two-Tier Render Cache**: L1 Memory Cache (30 pages) combined with L2 Disk Cache using xxHash (XXH3_64bits) for instant page flipping (0 ms).
- **Flexible Reading Modes**:
  - Single page viewing (Vertical)
  - Two-page spread reading (Horizontal)
  - Fullscreen reading mode (F11)
- **Zoom Controls**: Smooth zoom in/out with custom percentage dialog and initial auto-fit sizing.
- **Appearance & Themes**:
  - Light, Dark, and Sepia color themes
  - Configurable font family and base font size
  - Automatic persistent configuration via `settings.ini`
- **Book Converter**:
  - Text to PDF (with optional AES-128 password encryption)
  - Text to EPUB 2.0
  - Images to PDF
- **Internationalization (i18n)**: Full UI localization for English and Thai.
- **Modern Windows Integration**:
  - Per-Monitor V2 HiDPI awareness
  - Common Controls v6 styling
  - UTF-8 active code page and LongPathAware
  - Embedded application icon and Win32 VERSIONINFO resource

## Keyboard Shortcuts
| Shortcut | Action |
|---|---|
| `Ctrl+O` | Open document (PDF / EPUB / CBZ / CBR / ZIP / RAR) |
| `Ctrl+P` | Print document |
| `Ctrl+Shift+S` | Save As PDF copy |
| `Ctrl+R` | Rotate page clockwise 90 deg |
| `Ctrl+Shift+R` | Rotate page counter-clockwise 90 deg |
| `Delete` | Delete current PDF page |
| `Left` / `Page Up` | Previous page |
| `Right` / `Page Down` | Next page |
| `Ctrl++` / `Ctrl+=` | Zoom In |
| `Ctrl+-` | Zoom Out |
| `F1` | Settings dialog |
| `F2` | Converter dialog |
| `F11` | Toggle Fullscreen |

## Technical Stack & Libraries
- **Language**: C++17
- **UI Framework**: Qt 5.15 (Widgets, Core, Gui, Xml, PrintSupport)
- **PDF Engine**: MuPDF 1.27 C API
- **Comic Book Reader**: libarchive
- **EPUB Parser**: libzip + gumbo-parser
- **Hashing**: xxHash (XXH3)
- **Configuration**: QSettings (INI format)
- **Build System**: CMake 3.16+ with Ninja / GCC

## Building from Source

### Prerequisites (MSYS2 UCRT64)
Install required development packages:
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-qt5-base \
          mingw-w64-ucrt-x86_64-libmupdf \
          mingw-w64-ucrt-x86_64-libzip \
          mingw-w64-ucrt-x86_64-gumbo-parser \
          mingw-w64-ucrt-x86_64-libarchive
```

### Build Commands
```powershell
# Configure build
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Build executable
cmake --build build
```

The compiled binary and required runtime dependencies will be generated inside the `build/` directory:
```powershell
.\build\FeReader.exe
```

## License
Copyright 2026 Neofilisoft.
