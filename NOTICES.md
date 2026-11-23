# Notices

Witchaven 360 (`witchaven.xex`) is built from this repository's source and the libraries below. These are the notices
their licences ask for. Witchaven's game data (the `.art` and `.map` files, `joesnd`, `songs` and the rest) is not
included; it's Capstone Software's.

## Witchaven 360 and jfwhaven

The game's code is Witchaven's, as released by Les Bird (https://lesbird.github.io/capstone/), in Jonathon Fowler's
port, jfwhaven (https://github.com/jonof/jfwhaven). jfwhaven states no licence of its own. The source of every
release is at https://github.com/owl8319-source/witchaven360, the release's tag.

## The Build engine (jfbuild)

```
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
```

The licence is `jfbuild/buildlic.txt`: free, non-commercial distribution only. Jonathon Fowler's port of it,
https://github.com/jonof/jfbuild, is under the same terms (`jfbuild/LICENSE`). Witchaven 360 changes
`jfbuild/include/compat.h` and `jfbuild/src/engine.c`; both carry the "modified from Ken Silverman's original release"
notice.

## jfaudiolib

https://github.com/jonof/jfaudiolib: Apogee Software's audio library (James R. Dose), with Jonathon Fowler's
changes, under the GNU General Public License, version 2 or later (`COPYING.md`). `xenon/driver_sdl3.c` is based on
its `driver_sdl.c`, under the same licence.

## libsmacker

`libsmacker/`, Copyright (C) 2012-2021 Greg Kennedy, under the GNU Lesser General Public License, version 2.1 or
later (`libsmacker/COPYING`).

## Nuked OPL3

The music's OPL3 chip emulator (`opl/opl3.c`, `opl/opl3.h`, `opl/wf_rom.h`), Copyright (C) 2013-2020 Nuke.YKT, with
changes by Tony Gies (Nuked-OPL3-fast), as Heretic 360 has it, under the GNU Lesser General Public License, version 2.1
or later.

## The controller picture

`art/360_controller.svg`, drawn into the game as `xenon/controls_art.h`: "360 controller.svg" by Alphathon,
https://commons.wikimedia.org/wiki/File:360_controller.svg, under Creative Commons Attribution-ShareAlike 3.0
(https://creativecommons.org/licenses/by-sa/3.0/). Changed: labels, pointer lines and the Xbox logo removed, body
filled grey. Xbox and Xbox 360 are trademarks of Microsoft, which has nothing to do with this port.

## SDL 3 (SDL-xenon)

https://github.com/xenosworks/SDL-xenon, under the zlib licence:

```
Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>
  
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
  
1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required. 
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

```

## xsdk

https://github.com/xenosworks/xsdk, under the MIT licence:

```
MIT License

Copyright (c) 2026 Mena

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### In every title built with xsdk

`libxsdk.a` includes code from libxenon and from Xenia, both under the 3-clause BSD licence, which asks that a program
distributed as a binary carry the notice in its documentation. A game or homebrew title built with xsdk meets that by
shipping this section with it: in its documentation, or in a notices file beside the `.xex`.

#### libxenon

In `src/gpu_draw.c`, `src/gpu_resolve.c`, `src/gpu_setup.c`, `src/shader.c` and `src/texture.c`, from libxenon's
`drivers/xenos/xe.c`.

```
Copyright (c) 2009, free60.org
USB Stack Copyright (c) 2000,2001,2002,2003,2005, Broadcom Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY free60.org ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL free60.org BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
```

#### Xenia

In `src/texture_layout.c` and `src/tiled.c`, from Xenia's `src/xenia/gpu/texture_util.cc`, `texture_util.h` and
`texture_address.h`.
xost's shader encoder (`src/xsc_*.c`) also follows the instruction layouts in Xenia's microcode definitions.

```
Copyright (c) 2015, Ben Vanik.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the project nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL BEN VANIK BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```


newlib's C library, linked into every title, is under the permissive licences listed in its `COPYING.NEWLIB`; GCC's
runtime library exception covers the libgcc code in it.
