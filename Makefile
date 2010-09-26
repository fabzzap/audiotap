wav2prg: wav2prg_api.o \
         main.o \
         loaders.o \
         dependency_tree.o \
         turbotape.o \
         kernal.o \
         novaload.o \
         audiogenic.o \
         pavlodapenetrator.o \
         pavlodaold.o \
         pavloda.o \
         connection.o \
         rackit.o
	$(CC) -o $@ $^

t.o: wavprg.rc
	windres $(CPPFLAGS) -o $@ $^

t: CPPFLAGS = -D_WIN32_IE=0x0400
t: LDLIBS = -lcomctl32

t: gui_main.o \
   t.o \
   loaders.o \
   dependency_tree.o \
   turbotape.o \
   kernal.o \
   novaload.o \
   audiogenic.o \
   pavlodapenetrator.o \
   pavlodaold.o \
   pavloda.o \
   connection.o \
   rackit.o
