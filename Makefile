# currency
# See LICENSE file for copyright and license details.

include config.mk

SRC = currency.c
OBJ = ${SRC:.c=.o}

all: options currency

options:
	@echo currency build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

currency: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f currency ${OBJ} currency-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p currency-${VERSION}
	@cp -R LICENSE Makefile README config.def.h config.mk ${SRC} currency.1 \
		currency-${VERSION}
	@tar -cf currency-${VERSION}.tar currency-${VERSION}
	@gzip currency-${VERSION}.tar
	@rm -rf currency-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f currency ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/currency
	@chmod u+s ${DESTDIR}${PREFIX}/bin/currency
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" <currency.1 >${DESTDIR}${MANPREFIX}/man1/currency.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/currency.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/currency
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/currency.1

.PHONY: all options clean dist install uninstall
