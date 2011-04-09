all: audiotap.exe

audiotap-resources.o: audiotap.rc
	windres -o $@ $^

CFLAGS=-I../libtap -I../libaudiotap

ifdef DEBUG
CFLAGS += -g
endif

ifdef OPTIMIZE
CFLAGS += -O2
endif

audiotap.exe: audiotap_main.o audio2tap_core.o tap2audio_core.o audiotap-resources.o
	$(CC) $(LDFLAGS) -mwindows -o $@ $^ ../libaudiotap/audiotap.lib -lhtmlhelp

audio2tap.exe: audio2tap.o audiotap_callback.o audio2tap_core.o
	$(CC) $(LDFLAGS) -o $@ $^ ../libaudiotap/audiotap.lib

tap2audio.exe: tap2audio.o audiotap_callback.o tap2audio_core.o
	$(CC) $(LDFLAGS) -o $@ $^ ../libaudiotap/audiotap.lib

clean:
	rm -f *.o *~ audiotap.exe audio2tap.exe tap2audio.exe audio2tap tap2audio docs/audiotap.chm .#*

audio2tap: audio2tap.o audio2tap_core.o
tap2audio: tap2audio.o tap2audio_core.o
audio2tap tap2audio: audiotap_callback.o 
audio2tap tap2audio: LDFLAGS=-laudiotap -L../libaudiotap

docs/audiotap.chm: docs/audiotap-doc.hhp
	hhc $^
