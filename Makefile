all: audiotap

WINDRES=windres

audiotap-resources.o: audiotap.rc
	$(WINDRES) -o $@ $^

ifdef AUDIOTAP_HDR
  CFLAGS+=-I $(AUDIOTAP_HDR)
endif

ifdef DEBUG
CFLAGS += -g
endif

ifdef OPTIMIZE
CFLAGS += -O3
endif

ifdef HTMLHELP
CFLAGS += -DHAVE_HTMLHELP -I"$(HTMLHELP)\include"
LDLIBS += -lhtmlhelp -L"$(HTMLHELP)\lib"
endif

clean:
	rm -f *.o *~ audiotap.exe audio2tap.exe tap2audio.exe audio2tap tap2audio docs/audiotap.chm .#*

audio2tap: audio2tap.o audio2tap_core.o
tap2audio: tap2audio.o tap2audio_core.o
audiotap: audiotap.o audio2tap_core.o tap2audio_core.o audiotap-resources.o
audio2tap tap2audio: audiotap_callback.o
audio2tap tap2audio audiotap: audiotap_loop.o
audio2tap tap2audio audiotap: LDLIBS+=-laudiotap
audiotap: LDFLAGS+=-mwindows
audiotap: LDLIBS+=-lcomdlg32

ifdef AUDIOTAP_LIB
  LDLIBS+=-L$(AUDIOTAP_LIB)
endif

ifdef USE_RPATH
  ifneq ($(AUDIOTAP_LIB),)
    LDLIBS+=-Wl,-rpath=$(AUDIOTAP_LIB)
  endif
endif

HHC=hhc

docs\audiotap.chm: docs\audiotap-doc.hhp docs\2tap.htm docs\2tap_adv.htm docs\2tap_tips.htm docs\2wav.htm docs\2wav_adv.htm docs\2wav_tips.htm docs\concat.htm docs\contacts.htm docs\credits.htm docs\intro.htm docs\lesser_license.htm docs\license.htm docs\main.htm docs\portaudio_license.htm docs\2tap.png docs\2wav.png docs\optimal_rec.png docs\play_volume.png docs\rec_volume.png docs\sinusoid.png docs\too_loud.png
	$(HHC) $<
