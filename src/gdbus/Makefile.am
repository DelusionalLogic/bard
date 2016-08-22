generated.c: dk.slashwin.bard.xml
	gdbus-codegen  --c-namespace dbus --interface-prefix "dk.slashwin." --generate-c-code "generated" $^

dbus-wrapper.c: generated.c

bard_SOURCES = dbus-wrapper.c

clean-local:
	rm -f $(builddir)/generated.{h,c}
