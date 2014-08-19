BINPATH = ../../build/bin
PO = $(wildcard *.po)
MO = $(patsubst %.po,$(BINPATH)/po/%/LC_MESSAGES/dbp-run.mo,$(PO))

all: $(MO)

$(BINPATH)/po/%/LC_MESSAGES/dbp-run.mo: %.po
	mkdir -p $(BINPATH)/po/$*/LC_MESSAGES
	msgfmt -o $@ $<