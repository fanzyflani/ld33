Frametime - my LD33 jam category entry

Just a game about dogfighting as I didn't have time to implement the theme.

Controls:
* Arrows to turn your plane
* L1/R1 to strafe
* X to rocket boost
* Square to shoot
* Start to restart the level

If you got this from git, good luck compiling the damn thing!

If you got this from an archive, congratulations! If there isn't an emulator already provided in the archive, you'll need one. PCSX-Reloaded has incomplete timer code and doesn't show the "PIRACY harms consumers" screen properly, so there's a workaround especially for you. Personally I recommend playing this game with Mednafen instead, assuming your computer can handle it.

For clarification, you usually run from the .cue file, not the .raw file.

As this is a PAL game, the BIOS you'll need for emulators that need a BIOS is scph5502.bin with an MD5 of 32736f17079d0b2b7024407c39bd3050. This is purely to make the emulator work - this game does not use *any* BIOS features once booted.

If you get this working properly on a real PlayStation without any modifications, congratulations! Contact me with proof so I can stain my shirt brown with shock. For now, though, the emulator route is probably your best bet.

Apologies if your emulator doesn't like the fact that I have a broken iso-to-raw converter and thus spams incorrect error correction + detection codes like there's no tomorrow.

You will probably not be able to get up to a level that has blatant performance issues as the difficulty ramps up severely.

----

Code and scripts are MIT-licensed (see LICENCE.md) EXCEPT tools/iso2raw.c which is a modified mashup of two pieces of code and is in an unknown state. It is provided because it is VERY USEFUL.

rawcga.bin is the IBM CGA ROM font.

rawhead2.bin is a PAL CD-ROM header. It is required for PS1 development and ultimately cannot be replaced.
