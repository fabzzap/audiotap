audiotap-resources.o: audiotap.rc
	windres -o $@ $^

CFLAGS=-I../libtap -I../libaudiotap

audiotap.exe: audiotap_main.o audio2tap_core.o tap2audio_core.o audiotap-resources.o
	$(CC) $(LDFLAGS) -mwindows -o $@ $^ ../libaudiotap/audiotap.lib

audio2tap.exe: audio2tap.o audiotap_callback.o audio2tap_core.o
	$(CC) $(LDFLAGS) -o $@ $^ ../libaudiotap/audiotap.lib

tap2audio.exe: tap2audio.o audiotap_callback.o tap2audio_core.o
	$(CC) $(LDFLAGS) -o $@ $^ ../libaudiotap/audiotap.lib

clean:
	rm -f *.o *~ audiotap.exe audio2tap.exe tap2audio.exe audio2tap tap2audio

audio2tap: audio2tap.o audiotap_callback.o audio2tap_core.o
	$(CC) $(LDFLAGS) -o $@ $^ -laudiotap -L../libaudiotap -ldl -Wl,--rpath-link=../libtap

tap2audio: tap2audio.o audiotap_callback.o tap2audio_core.o
	$(CC) $(LDFLAGS) -o $@ $^ -laudiotap -L../libaudiotap -ldl -Wl,--rpath-link=../libtap
