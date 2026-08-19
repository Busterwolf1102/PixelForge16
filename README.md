# PIXELFORGE 16

PixelForge 16 is a small native Windows low-poly modeler built around a fixed
640x480 logical framebuffer, a 16 color VGA-style palette, direct mouse editing,
and a CPU software renderer. The interface uses a retro blue/black DOS CAD style:
black viewports, blue pixel panels, cyan hover feedback, and yellow selection.

## Build

Run:

```bat
build.bat
```

The script uses CMake when available. If CMake is not on `PATH`, it falls back to
direct MSVC compilation through Visual Studio 2022.

The packaged executable is written to:

```text
dist\PixelForge16\PixelForge16.exe
```

A Korean beginner tutorial is available at:

```text
docs\tutorial_ko.md
```

## Mouse Controls

```text
LMB                 Select / drag vertex
Shift+LMB           Add to selection
Ctrl+LMB            Toggle selection
Empty LMB drag      Box select vertices
MMB                 Orbit perspective view
Shift+MMB           Pan view
Wheel               Zoom
Palette click       Paint hovered/selected face
Right click         Pixel context menu
```

## Viewport

```text
SPACE               Maximize hovered/active viewport, or return to 4 views
Home                Frame all
Ctrl+Home           Reset views
```

## Modeling

```text
Shift+A / Tab       Add primitive menu
E                   Extrude hovered/selected face
I                   Inset hovered/selected face
F                   Create face from 3+ selected vertices, or focus selection
R                   Rotate selection/object by 15 degrees
S                   Scale selection/object
M                   Merge selected vertices, or cycle render mode
X / Delete          Delete selection
D                   Duplicate object/selection
Arrow/WASD          Nudge selected vertices
PageUp/PageDown     Nudge selected vertices in view depth
Shift               10x nudge
Alt                 0.1x nudge
Ctrl while dragging Temporarily invert snap
```

## Save And Export

```text
Ctrl+N              New scene
Ctrl+O              Open .pf16
Ctrl+S              Save .pf16
Ctrl+Shift+S        Save as .pf16
File > Import OBJ   Import v/f OBJ geometry
File > Export OBJ   Export OBJ + 16-color MTL
F12                 Save perspective screenshot BMP
```

## Notes

This is an MVP: geometry, 16 colors, flat/wire rendering, direct manipulation,
undo/redo, save/load, OBJ import/export, and screenshots are implemented. Texture
painting, UV editing, animation, materials, physics, node graphs, and PBR are
intentionally out of scope.
