# Art

## 360_controller.svg

The controller on the Settings > Controls page, as Heretic 360's.

- **Original:** [360 controller.svg](https://commons.wikimedia.org/wiki/File:360_controller.svg) by
  [Alphathon](https://commons.wikimedia.org/wiki/User:Alphathon), on Wikimedia Commons.
- **Licence:** [Creative Commons Attribution-ShareAlike 3.0 Unported](https://creativecommons.org/licenses/by-sa/3.0/)
  (CC BY-SA 3.0). This changed version, and the picture made from it (`xenon/controls_art.h`), are under the same
  licence.
- **Changes:** the English labels and their pointer lines removed; the Xbox logo on the Guide button removed; the
  body filled light grey (was white).
- Neither Alphathon nor Microsoft has anything to do with this port or endorses it. Xbox and Xbox 360 are trademarks
  of Microsoft.

To make `xenon/controls_art.h` again (Python 3, Pillow and `rsvg-convert`, from `librsvg2-bin`):

    python3 tools/make_controls_art.py path/to/palette.dat art/360_controller.svg

`palette.dat` gives only Witchaven's palette; nothing of it goes into the header.
