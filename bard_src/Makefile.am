bin_PROGRAMS = bard
bard_SOURCES = main.c barconfig.c unitexecute.c runner.c linkedlist.c unit.c configparser.c workmanager.c sortedlist.c formatter.c advancedformat.c output.c xlib_color.c unitcontainer.c fs.c font.c regex.c formatarray.c parser.c manager.c
bard_CFLAGS = -I$(top_srcdir)/sharedlib_src -I$(builddir) $(GIO_CFLAGS)
bard_LDADD = -liniparser -lX11 -lJudy -lpcre2-8 -lpthread $(GIO_LIBS) $(top_builddir)/sharedlib_src/libshared.la
