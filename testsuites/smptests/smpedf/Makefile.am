
rtems_tests_PROGRAMS = smpedf
smpedf_SOURCES = init.c system.h ../../support/src/locked_print.c

dist_rtems_tests_DATA = smpedf.scn
dist_rtems_tests_DATA += smpedf.doc

include $(RTEMS_ROOT)/make/custom/@RTEMS_BSP@.cfg
include $(top_srcdir)/../automake/compile.am
include $(top_srcdir)/../automake/leaf.am

AM_CPPFLAGS += -I$(top_srcdir)/../support/include
AM_CPPFLAGS += -DSMPTEST 

LINK_OBJS = $(smpedf_OBJECTS)
LINK_LIBS = $(smpedf_LDLIBS)

smpedf$(EXEEXT): $(smpedf_OBJECTS) $(smpedf_DEPENDENCIES)
	@rm -f smpedf$(EXEEXT)
	$(make-exe)

include $(top_srcdir)/../automake/local.am
