CPP=cl.exe
LINK32=link.exe

LINK32_FLAGS= /PDB:slony1_funcs.pdb /DEF:slony1_funcs.def /DLL $(PG_LIB)\postgres.lib
OBJS = 	slony1_funcs.obj		\

CPP_FLAGS=/c /D MSVC /D WIN32 /D PGSHARE=\"$(PGSHARE)\" /I..\misc /I..\..\ /I$(PG_INC) /I$(PG_INC)\server  /I$(PG_INC)\server\port\win32_msvc /I$(PG_INC)\server\port\win32 /I$(GETTEXT_INC) /D HAVE_LL_CONSTANTS=1 /D HAVE_GETACTIVESNAPSHOT  /LD /Gd /Tc 

slony1_funcs.obj: slony1_funcs.c
	$(CPP) $(CPP_FLAGS)  slony1_funcs.c

slony1_funcs.$(SLONY_VERSION).dll: $(OBJS)
	$(LINK32) $(LINK32_FLAGS) /out:slony1_funcs.$(SLONY_VERSION).dll $(OBJS)  
