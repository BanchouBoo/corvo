# corvo

Simple X utility to hide the mouse cursor in certain windows. You pass it a list of conditions and if a matching window becomes focused the mouse cursor is hidden.

## usage

Help output listed below. Flags can be repeated, so you can pass in e.g. multiple classes.

```
corvo: [CONDITIONS]
  -h             print this help and exit

  CONDITIONS:
    -t TITLE     hide the cursor on windows with TITLE as their window title (WM_NAME)
    -c CLASS     hide the cursor on windows with CLASS as their class value (first value in WM_CLASS)
    -i INSTANCE  hide the cursor on windows with INSTANCE as their class value (second value in WM_CLASS)
    -w WINDOW    hide the cursor on the window with the ID WINDOW
```

## build

Dependencies
- `xcb`
- `xcb-xfixes`
Then just run `make` to build the binary in the project folder and `make install` to install it to the `bin` folder in the specified prefix (default `/usr/local`)

## caveats

The focused window is tracked using the `_NET_ACTIVE_WINDOW` atom on the root window, if your window manager does not have this (it probably does) then it will not work.

Additionally, if you move the cursor from a hiding window to a window that wouldn't take focus/be put into `_NET_ACTIVE_WINDOW` (such as the root window, and possibly bars and such) then the cursor will stay hidden.
