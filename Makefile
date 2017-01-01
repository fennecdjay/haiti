# comment those line to disable
USE_GITHUB ?= 1
# compile for github or not
DEBUG ?==1
PREFIX?=/usr
# pedantic, -g and other flags
#define USE_GITHUB
#endef
PROG=haiti
SRC=${PROG}.c print.c
LDFLAGS+=-lpthread -std=c99
LDFLAGS+=-pedantic -Wall

#include config.mk

ifeq ($(USE_GITHUB), 1)
SRC+=issue.c ini.c cJSON.c
LDFLAGS+=-lcurl -lm
endif

#ifeq ($(DEBUG), 1)
#CFLAGS+=-pedantic -Wall -O3
#CFLAGS+=-pedantic -Wall -O3
#else
CFLAGS+=-march=native -O3
#endif

OBJ=$(SRC:.c=.o)

${PROG}: ${OBJ}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJ}
	strip ${PROG}
	rm *.o

install:
	install ${PROG} ${PREFIX}/bin

clean:
	rm -f *.o vgcore.* haiti

ini.o: ini.c
	${CC} -DINI_ALLOW_MULTILINE=0 -c $< -o $(<:.c=.o)

issue.o: issue.c
	${CC} -DUSE_GITHUB -c $< -o $(<:.c=.o)

.c.o:
	${CC} ${CFLAGS} -c $< -o $(<:.c=.o)
