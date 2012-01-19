AROPT = cr
export_dynamic = -Wl,-Bexport
shlib_symbolic = -Wl,-Bsymbolic

DLSUFFIX = .so
ifeq ($(GCC), yes)
CFLAGS_SL = -fpic
else
CFLAGS_SL = -K PIC
endif

%.so: $(SO_OBJS)
	$(LD) -G -Bdynamic -o $@ $<
sqlmansect = 7
