gdbus/generated.c: gdbus/dk.slashwin.bard.xml
	gdbus-codegen  --c-namespace dbus --interface-prefix "dk.slashwin." --generate-c-code "bard-generated" $^

gdbus/dbus-wrapper.c: gdbus/generated.c

noinst_LTLIBRARIES = libshared.la
libshared_la_SOURCES = vector.c myerror.c logger.c gdbus/dbus-wrapper.c gdbus/dbus.c
libshared_la_CFLAGS = -I$(builddir) $(GIO_CFLAGS)
libshared_la_LIBADD = -lJudy -lpcre2-8 -lpthread $(GIO_LIBS)

clean-local:
	rm -f $(builddir)/gdbus/bard-generated.{h,c}
