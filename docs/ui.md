# UI

The UI is drawn into the same 640x480 framebuffer as the model views. It uses
the same 16 color palette as model rendering, but UI color roles are separated
through `UITheme` so changing interface colors does not change saved model data.

```text
Top bar      BLUE, WHITE text
Viewports    BLACK background, BLUE/CYAN borders
Bottom bar   BLUE status and palette band
Menus        BLUE panels, LIGHT BLUE hover rows, CYAN shortcuts
Selection    YELLOW outlines and vertex markers
Hover        CYAN outlines and focus cues
Warnings     LIGHT RED text
Enabled      LIGHT GREEN text
```

The visual style avoids rounded corners, gradients, shadows, transparency blur,
native Windows controls, anti-aliased fonts, and modern dashboard styling.
Buttons, menus, popups, view headers, and palette swatches use hard 1px pixel
borders and bitmap glyph text.
