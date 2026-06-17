lib_LTLIBRARIES =

# bayes_opt codels library
lib_LTLIBRARIES += libbayes_opt_codels.la

libbayes_opt_codels_la_SOURCES  =	bayes_opt_c_types.h
libbayes_opt_codels_la_SOURCES +=	bayes_opt_codels.c
libbayes_opt_codels_la_SOURCES +=	bayes_opt_optimize_codels.c

libbayes_opt_codels_la_CPPFLAGS =	$(requires_CFLAGS)
libbayes_opt_codels_la_LIBADD   =	$(requires_LIBS)
libbayes_opt_codels_la_CPPFLAGS+=	$(codels_requires_CFLAGS)
libbayes_opt_codels_la_LIBADD  +=	$(codels_requires_LIBS)
libbayes_opt_codels_la_LDFLAGS  =	-release $(PACKAGE_VERSION)


# idl mappings
BUILT_SOURCES=	bayes_opt_c_types.h
CLEANFILES=	${BUILT_SOURCES}

bayes_opt_c_types.h: ${top_srcdir}/bayes_opt.gen
	${GENOM3}  mappings \
	  -MD -MF .deps/$@.d -MT $@ --signature -l c $< >$@

-include .deps/bayes_opt_c_types.h.d
