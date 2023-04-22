# hyprMonitorPreview
A small ncurses preview of the set monitor layout (hyprland only)

![Preview](Preview.png)

## Build && Installation

```bash
cmake -B build . -DCMAKE_BUILD_TYPE=Release
cmake -B build . -DCMAKE_BUILD_TYPE=Debug

make -C build
```

## Possible Future Additions

- Live Update of Monitor Windows (size, position, rotation, etc.)
- Change current config of Monitor Windows by click and dragging (size, position, transform by menu?)
- Append changes to config or output on stdout

