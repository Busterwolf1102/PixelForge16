# PF16 File Format

The `.pf16` format is a simple text format for fast iteration.

```text
PF16 1
NEXT <next-stable-id>
SETTINGS <render-mode> <current-color> <grid-on> <snap-on> <grid-size>
OBJECTS <count>
OBJECT <id> <name>
TRANS <px> <py> <pz> <rx> <ry> <rz> <sx> <sy> <sz>
VERTICES <count>
v <id> <x> <y> <z>
FACES <count>
f <id> <color-index> <vertex-count> <vertex-id> ...
END_OBJECT
```

Color indices are always 0 through 15.

