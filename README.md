# corvo

Simple X utility to hide the mouse cursor in certain windows. You pass it a list of window titles (one title per argument) and if a matching window becomes focused the mouse cursor is hidden.

## usage

```
corvo "title1" "title2"
```

## build

Dependencies
- `xcb`
- `xcb-xfixes`
Then just run `make` to build the binary in the project folder and `make install` to install it to the `bin` folder in the specified prefix (default `/usr/local`)

## caveats

The focused window is tracked using the `_NET_ACTIVE_WINDOW` atom on the root window, if your window manager does not have this (it probably does) then it will not work.

Additionally, if you move the cursor from a hiding window to a window that wouldn't take focus/be put into `_NET_ACTIVE_WINDOW` (such as the root window, and possibly bars and such) then the cursor will stay hidden.

## notes

For now I only really plan to support titles, as I made this primarily for games where the cursor for whatever reason isn't hidden when it should, but I may expand it a bit to support lists of not just titles but also class names, instance names, and arbitrary window IDs, but any further work on this program is low priority for me.
