MAN1_FILES  = \
	bard.txt

A2X_ARGS = \
	--doctype manpage \
	--format manpage

man_MANS = \
	$(MAN1_FILES:.txt=.1)

.txt.1:
if HAVE_A2X
	$(AM_V_GEN) $(A2X) $(A2X_ARGS) $<
else
	@echo "*** a2x (asciidoc) is required to regenerate $(@) ***"; exit 1;
endif

CLEANFILES = \
	$(MAN1_FILES:.txt=.1) \
	$(MAN1_FILES:.txt=.xml)
