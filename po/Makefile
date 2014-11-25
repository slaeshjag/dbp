OUTPATH = ../build/share/locale
PO = $(wildcard *.po)
MO = $(patsubst %.po,$(OUTPATH)/%/LC_MESSAGES/dbp-run.mo,$(PO))

all: $(MO)
	@echo "Done."
	@echo ""

$(OUTPATH)/%/LC_MESSAGES/dbp-run.mo: %.po
	@echo " [ MO ] $@"
	@mkdir -p $(OUTPATH)/$*/LC_MESSAGES
	@msgfmt -o $@ $<
