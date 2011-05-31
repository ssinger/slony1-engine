CPP=cl.exe
LINK32=link.exe

LINK32_FLAGS= /PDB:slony1_funcs.pdb /DEF:slony1_funcs.def /DLL $(PG_LIB)\postgres.lib
OBJS = 	slony1_funcs.obj		\

CPP_FLAGS=/c /D MSVC /D WIN32 /D PGSHARE=\"$(PGSHARE)\" /I..\misc /I..\..\ /I$(PG_INC) /I$(PG_INC)\server  /I$(PG_INC)\server\port\win32_msvc /I$(PG_INC)\server\port\win32 /D HAVE_LONG_INT_64 /D HAVE_GETACTIVESNAPSHOT /LD /Gd /Tc 

slony1_funcs.obj: slony1_funcs.c
	$(CPP) $(CPP_FLAGS)  slony1_funcs.c

slony1_funcs.dll: $(OBJS)
	$(LINK32) $(LINK32_FLAGS) $(OBJS)  
