SUBDIRS = audio core video

.rc.o:
	$(WINDRES) -o $@ $<

bin_PROGRAMS = punes

punes_SOURCES = \
	core/main.c \
	core/palette.c \
	$(ico_stuff)

punes_LDADD = \
	core/libcore.a \
	video/libvideo.a \
	c++/libcplus.a \
	audio/libaudio.a \
	$(LIB7ZIP_LIBS)
