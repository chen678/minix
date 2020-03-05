# Makefile for Scheduler (SCHED)
PROG=	sched
SRCS=	main.c schedule.c utility.c

DPADD+=	${LIBSYS} ${LIBTIMERS}
LDADD+=	-lsys -ltimers -Wl,-Ttext=0x800000 -g

MAN=

BINDIR?= /usr/sbin

CPPFLAGS.main.c+=	-I${NETBSDSRCDIR}
CPPFLAGS.schedule.c+=	-I${NETBSDSRCDIR}
CPPFLAGS.utility.c+=	-I${NETBSDSRCDIR}

.include <minix.service.mk>
