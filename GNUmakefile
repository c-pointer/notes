#
# note: ps2pdf produces better results
# note: if libncursesw.so.* is missing create symbolic link of libncurses.so.* and run ldconfig

DESTDIR ?=
prefix  ?= /usr/local
bindir  ?= $(prefix)/bin
mandir  ?= $(prefix)/share/man
man1dir ?= $(mandir)/man1
man5dir ?= $(mandir)/man5

APPNAME := notes
ADDMODS := str.o nc-readstr.o nc-core.o nc-keyb.o nc-view.o nc-list.o notes.o list.o errio.o

CFLAGS  := -O -Wall -Wformat=0 -D_GNU_SOURCE
LDLIBS  := -lncursesw -lncurses
M2RFLAGS := -z

all: $(APPNAME)

clean:
	rm -f *.o $(APPNAME) $(APPNAME).man $(APPNAME).1.gz $(APPNAME)rc.man $(APPNAME)rc.5.gz nc-colors nc-getch > /dev/null

$(APPMODS): %.o: %.c

$(APPNAME): $(ADDMODS)

$(APPNAME).man: $(APPNAME).md
	md2roff $(M2RFLAGS) $(APPNAME).md > $(APPNAME).man

$(APPNAME).1.gz: $(APPNAME).md
	md2roff $(M2RFLAGS) $(APPNAME).md > $(APPNAME).1
	gzip -f $(APPNAME).1

$(APPNAME)rc.man: $(APPNAME)rc.md
	md2roff $(M2RFLAGS) $(APPNAME)rc.md > $(APPNAME)rc.man

$(APPNAME)rc.5.gz: $(APPNAME)rc.md
	md2roff $(M2RFLAGS) $(APPNAME)rc.md > $(APPNAME)rc.5
	gzip -f $(APPNAME)rc.5

html: $(APPNAME).man
	md2roff -q $(APPNAME).md > $(APPNAME).man
	groff $(APPNAME).man -Thtml -man > $(APPNAME).html

pdf: $(APPNAME).md
	md2roff -q $(APPNAME).md > $(APPNAME).man
	groff $(APPNAME).man -Tpdf -man -dPDF.EXPORT=1 -dLABEL.REFS=1 -P -e > $(APPNAME).pdf

install: $(APPNAME) $(APPNAME).1.gz $(APPNAME)rc.5.gz
	install -m 755 -o root -g root -d $(DESTDIR)$(bindir)
	install -m 755 -o root -g root -d $(DESTDIR)$(man1dir)
	install -m 755 -o root -g root -d $(DESTDIR)$(man5dir)
	install -m 755 -o root -g root -s $(APPNAME) $(DESTDIR)$(bindir)
	install -m 644 -o root -g root $(APPNAME).1.gz $(DESTDIR)$(man1dir)
	install -m 644 -o root -g root $(APPNAME)rc.5.gz $(DESTDIR)$(man5dir)

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(APPNAME) $(DESTDIR)$(man1dir)/$(APPNAME).1.gz $(DESTDIR)$(man5dir)/$(APPNAME)rc.5.gz

# utilities
nc-colors: nc-colors.c

nc-getch: nc-getch.c

