PtBi Source Release
===================

If you are looking at this you are interested in
PtBi's source code. General information about the 
program for end-users is available at:
http://ptbi.metaclassofnil.com/

I sometimes post about its development at:
http://blog.metaclassofnil.com/

General Notes
=============

This is not a particularly clean project, it was
never really intended for public release and has
been developed over more than 6 years now. Some of
the code is even older and hijacked from other
projects, so I'd appreciate not drawing too many
conclusions about my development skills/style from
this.

I primarily released the code since two people
expressed interest, and since I believe that there
is some interesting stuff in there that might be of 
use, including:
- the example of Blackmagic Decklink API usage,
  including video and sound, and GPU decoding
- using libraries for Dolby Digital and DTS 
  decoding from an unformatted bitstream
- the OpenGL integration of FXAA/SMAA

Compilation
===========

The Visual Studio project file supplied here has 
plenty of hard references to my local paths,
and obviously needs to be adjusted.

The external dependencies required by PtBi include:
- GLEW   (OpenGL extension handling)
- BASS   (audio output)
- DevIL  (screenshots)
- a52dec (DD decoding)
- libdca (DTS decoding)
- The Blackmagic DeckLink API

The stb_truetype header for text rendering is 
included in the repository, as are FXAA and SMAA
codes and files. Oh, and there is a dependency on
Ruby for version number incrementing (I know this
makes no sense).

PtBi should be run in the "run" folder, which contains
the files required during execution.

License
=======
All the source code is released under the conditions of 
the GPLv3, except for the dependencies listed above,
which have their own licensing terms

Contact
=======

If you want to report bugs or contribute you can
do so directly on GitHub (e.g. open an issue or
pull request).

You can always reach me at
peter at metaclassofnil dot com
if you have any questions or comments.

