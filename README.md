Audiotap is a program that creates TAP files from Commodore tapes, and turns TAP or DMP files into Commodore tapes. It is also able to concatenate many TAP files into one.

It uses the Audiotap library to access Commodore tapes (the Audiotap library can read TAP, DMP and some kinds of audio files including WAV, and can write TAP and WAV files).
Furthermore, it needs the tapencoder library to create TAP files from audio, and the tapdecoder library to create audio from TAP and DMP files.
The command-line tool tap2audio will only need tapdecoder, the command-line tool audio2tap will only need tapencoder.

In order to build it, GNU make and a compiler are needed. Unix systems typically have such an environment ready, while, on Windows, [MinGW]
will do. To build, type `make` followed by the target name (not needed for the default target) and the optional modifiers.

The possible targets are:
* `audiotap.exe` (the default target), the Windows GUI executable
* `audio2tap`, the command-line tape reader
* `tap2audio`, the command-line tape writer

The dependency on the Audiotap library means that:
1. at compile time, the compiler must be able to find `audiotap.h`
2. at link time, the linker must be able to find `audiotap.lib` (Windows) `libaudiotap.so` (Unix)
3. at run time, the executable must be able to find `audiotap.dll` (Windows) `libaudiotap.so` (Unix), plus `tapencoder.dll` (Windows) `libtapencoder.so` (Unix) to be able to create audio, and `tapdecoder.dll` (Windows) `libtapdecoder.so` (Unix) to be able to read audio

In order to achieve that:
1. One solution is to deploy `audiotap.h` in the compiler's environment (e.g. `/usr/include`). Alternatively, the modifier `AUDIOTAP_HDR` can be used, with the path where `audiotap.h` is (not including `audiotap.h`). Example:

        make cmdline/wav2prg AUDIOTAP_HDR=path/to/audiotap.h/
2. One solution is to deploy `audiotap.lib` or `libaudiotap.so` in the library path (e.g. `/usr/lib`). Alternatively, the modifier `AUDIOTAP_LIB` can be used, with the path where `audiotap.lib` or `libaudiotap.so` is. Example:

        make cmdline/wav2prg AUDIOTAP_HDR=path/to/audiotap.h/ AUDIOTAP_LIB=/path/to/libaudiotap/
3. On Windows, just make sure `audiotap.dll`, `tapencoder.dll` and `tapdecoder.dll` are in the same folder as the executable. On Unix, one solution is to deploy `libaudiotap.so`, `libtapencoder.so` and `libtapdecoder.so` in the library path (e.g. `/usr/lib`). Alternatively, the modifier `USE_RPATH` can be used along with `AUDIOTAP_LIB`. If `USE_RPATH` is set to 1, the path to the library will be hard-coded in the executable's library search path. In this case it is recommended to specify an absolute path in `AUDIOTAP_LIB`. Example:

        make cmdline/wav2prg AUDIOTAP_HDR=path/to/audiotap.h/ AUDIOTAP_LIB=/path/to/libaudiotap/ USE_RPATH=1

    On Mac, one can either ensure libaudiotap.so, libtapencoder.so and libtapdecoder.so are in the same folder as the executable (similar to Windows), or deploy them in the library path (as in other flavours of Unix), but USE_RPATH won't work.

Other modifiers are:
* DEBUG: set to 1 to produce an executable that can be debugged with gdb. Example:

        make audiotap.exe DEBUG=1
* OPTIMIZE: set to 1 to produce an optimised executable. Example:

        make audiotap.exe OPTIMIZE=1
* CC: changes the compiler. Examples:

        make CC=gcc
    needed if your installation does not provide cc.

        make CC=clang
    if you want to use LLVM instead of gcc (and LLVM is installed).

       make CC=i586-mingw32msvc-gcc
    if you are running Linux and you want to produce Windows DLLs (and you have the MinGW cross-compiler installed)
* WINDRES: changes the Windows resource compiler. Only needed when building the Windows GUI version and the Windows resource compiler has a non-standard name. Example:

        make CC=i586-mingw32msvc-gcc WINDRES=i586-mingw32msvc-windres
* HTMLHELP: adds support for invoking help by pressing F1 in the Windows GUI version. Requires [Microsoft HTML Help Workshop] to be installed. Argument to HTMLHELP is HTML Help Workshop's installation path. Example:

        make AUDIOTAP_HDR=..\libaudiotap AUDIOTAP_LIB=..\libaudiotap DEBUG=1 HTMLHELP="c:\Programmi\HTML Help Workshop"

[MinGW]: http://www.mingw.org/
[Microsoft HTML Help Workshop]: https://www.microsoft.com/en-us/download/details.aspx?id=21138

