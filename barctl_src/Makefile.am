bin_PROGRAMS = barctl
barctl_SOURCES = main.c
barctl_CFLAGS = -I$(top_srcdir)/sharedlib_src -I$(builddir)/../sharedlib_src -I$(builddir) $(GIO_CFLAGS)
barctl_LDADD = -liniparser -lJudy -lpthread $(top_builddir)/sharedlib_src/libshared.la $(GIO_LIBS)
