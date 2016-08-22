gdbus/generated.c: gdbus/dk.slashwin.bard.xml
	gdbus-codegen  --c-namespace dbus --interface-prefix "dk.slashwin." --generate-c-code "bard-generated" $^

gdbus/dbus-wrapper.c: gdbus/generated.c

bin_PROGRAMS = bard
bard_SOURCES = main.c barconfig.c unitexecute.c runner.c logger.c vector.c linkedlist.c unit.c configparser.c workmanager.c sortedlist.c formatter.c advancedformat.c output.c xlib_color.c unitcontainer.c fs.c font.c regex.c formatarray.c parser.c myerror.c manager.c gdbus/dbus-wrapper.c gdbus/dbus.c
bard_CFLAGS = -I$(builddir) $(GIO_CFLAGS)
bard_LDADD = -liniparser -lX11 -lJudy -lpcre2-8 -lpthread $(GIO_LIBS)

clean-local:
	rm -f $(builddir)/gdbus/bard-generated.{h,c}
