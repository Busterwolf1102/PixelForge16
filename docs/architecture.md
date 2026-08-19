# Architecture

PixelForge 16 is intentionally compact:

```text
Win32 window
  640x480 framebuffer
  nearest-neighbor integer scaling

CPU renderer
  line rasterization
  triangle rasterization
  z-buffer
  wireframe / flat / flat+wire

Scene model
  stable object, vertex, and face IDs
  mesh objects
  object transforms

Editor
  hover picking
  direct vertex dragging
  box selection
  command snapshots for undo/redo

I/O
  .pf16 text format
  OBJ import/export
  BMP screenshot export
```

