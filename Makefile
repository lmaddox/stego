# these rules do not create files (all other rules do)
.PHONY: all install uninstall clean dist_clean release benchmark libraries todo default carefully debug



# defaults (can be overridden on the command line)

# -v makes these tools verbose
STRIP:=strip -v
LN:=ln -sfv
TAR:=tar -v
RM:=$(RM) -v

CC:=$(CXX)

#$(shell mysql_config --include)
CPPFLAGS+=-MMD -MP -D_GNU_SOURCE
CFLAGS:=-march=native -mtune=native -pipe
ARFLAGS:=vcrs

WARNINGS:=-Wall -Wextra -pedantic -pedantic-errors -Wfatal-errors -Werror -Wno-long-long
APPLICATION_OPTIMIZATIONS:=-g0 -fomit-frame-pointer -Ofast
LIBRARY_OPTIMIZATIONS:=-g0 -fomit-frame-pointer -Os
ARCHIVE_OPTIMIZATIONS:=-g0 -fomit-frame-pointer -O2
DEBUG:=-g -O0 -DDEBUG

PREFIX:=/usr/bin
EXE:=lmaddox_cs4953_hw4
RELEASE:=$(EXE)_$(shell whoami)_$(shell date +%s)
MAJOR:=0
MINOR:=0

LIB:=
SRC:=main.c $(LIB)
LIBRARIES:=-lm



# yeah, whatever
all:
  make -j2 -l3 default

default: $(EXE)_dbg $(EXE)_vanilla $(EXE) lib$(EXE).so lib$(EXE).a

# force one-by-one compilation
carefully:
	$(MAKE) $(EXE)_dbg
	$(MAKE) $(EXE)_vanilla
	$(MAKE) lib$(EXE).a
	$(MAKE) lib$(EXE).so
	$(MAKE) $(EXE)

# generate a todo-list
#TODO figure out why the anchor doesn't work
todo:
	grep -n TODO *.c *.h Makefile | grep -v --mmap '\	grep -n TODO '
	@echo $@: Success\!

# list required libraries
libraries:
	@echo $(LIBRARIES)



# typical make targets

install: $(EXE)
	$(LN) $(shell pwd)/$< $(PREFIX)/$<
	@echo $@: Success\!

uninstall:
	$(RM) $(PREFIX)/$(EXE)
	@echo $@: Success\!

# create an archival copy
release: dist_clean
	$(TAR) --bzip2 --create --file ../$(RELEASE).tbz --transform 's/^./$(RELEASE)/' .
	scp ../$(RELEASE).tbz elk04.cs.utsa.edu:
	@echo $@: Success\!

# delete all machine-generated files
dist_clean: clean
	-$(RM) $(EXE) $(EXE)_dbg $(EXE)_tmp $(EXE)_vanilla lib$(EXE).so* lib$(EXE).a *.gcda unity.c labels.dat
	#cd valgrind_suppressions ; make $@
	@echo $@: Success\!

# delete all intermediate files... but keep data for profiling
clean:
	-$(RM) *.d *.o
	@echo $@: Success\!



$(EXE)_vanilla: $(SRC) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(APPLICATION_OPTIMIZATIONS) -fwhole-program -o $@ $(SRC) $(LIBRARIES)
	$(STRIP) --strip-all $@
	-upx --ultra-brute --all-methods --all-filters $@ && upx -t $@
	@wc -c $@
	@echo $@: Success\!

$(EXE): benchmark
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(APPLICATION_OPTIMIZATIONS) -DBENCHMARK -fwhole-program -fprofile-use -o $@ $(SRC) $(LIBRARIES)
	$(STRIP) --strip-all $@
	-upx --ultra-brute --all-methods --all-filters $@ && upx -t $@
	@wc -c $@
	@echo $@: Success\!

# TODO use real benchmark
benchmark: $(EXE)_tmp
	for k in $(shell seq 1 1000) ; \
	do \
		PATH=. $< > /dev/null; \
	done
	@echo $@: Success\!

$(EXE)_tmp: $(SRC) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(APPLICATION_OPTIMIZATIONS) -DBENCHMARK -fwhole-program -fprofile-generate -fprofile-arcs -o $@ $(SRC) $(LIBRARIES)
	$(STRIP) --strip-all $@
	-upx --ultra-brute --all-methods --all-filters $@ && upx -t $@
	@wc -c $@
	@echo $@: Success\!

# not using -flto here, because it's supposed to break the code
$(EXE)_dbg: $(SRC) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(DEBUG) -fwhole-program -o $@ $(SRC) $(LIBRARIES)
	@wc -c $@
	@echo $@: Success\!

lib$(EXE).so: so.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(LIBRARY_OPTIMIZATIONS) -shared -Wl,-O1 -Wl,--sort-common -Wl,--relax -Wl,--strip-debug -Wl,-z,now -Wl,-soname,$@.$(MAJOR) -o $@.$(MAJOR).$(MINOR) $< $(LIBRARIES)
	$(STRIP) --strip-all $@.$(MAJOR).$(MINOR)
	$(LN) $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	$(LN) $@.$(MAJOR) $@
	@wc -c $@
	@echo $@: Success\!

so.o: unity.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(LIBRARY_OPTIMIZATIONS) -fPIC -c -o $@ $< $(LIBRARIES)
	@echo $@: Success\!

lib$(EXE).a: unity.c
	$(AR) vcrs $@ $<
	$(STRIP) --strip-unneeded $@
	@wc -c $@
	@echo $@: Success\!

a.o: unity.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(ARCHIVE_OPTIMIZATIONS) -c -o $@ $< $(LIBRARIES)
	@echo $@: Success\!

unity.c: $(LIB) Makefile
	echo -n > $@
	for cfile in $(LIB) ; \
	do \
		echo \#include \"$${cfile}\" >> $@ ; \
	done
	@echo $@: Success\!



#-include $(patsubst %.c,%.d,$(SRC) unity.c)
#-include a.d  sceadan.d  sceadan_dbg.d  sceadan_tmp.d  sceadan_vanilla.d  so.d
#-include *.d
-include $(SRC:%.c=%.d)

# references:
# http://stackoverflow.com/questions/13674894/when-to-use-certain-optimizations-such-as-fwhole-program-and-fprofile-generate
# http://www.technovelty.org/linux/stripping-shared-libraries.html
# http://www.yolinux.com/TUTORIALS/LibraryArchives-StaticAndDynamic.html
# http://stackoverflow.com/questions/6489627/how-to-use-multiple-source-files-to-create-a-single-object-file-with-gcc
