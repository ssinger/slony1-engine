CPP=cl.exe
LINK32=link.exe
PGHOME=c:\postgresql\9.0
PGSHARE=$(PGHOME)\share
PTHREADS_INC=C:\pthreads-win32\include
PTHREADS_LIB=c:\pthreads-win32\lib
LINK32_FLAGS= /PDB:slony1_funcs.pdb /DEBUG /DEF:slony1_funcs.def /DLL $(PGHOME)\lib\postgres.lib
OBJS = 	slony1_funcs.obj		\

CPP_FLAGS=/c /D MSVC /D WIN32 /D PGSHARE=$(PGSHARE) /I..\misc /I..\..\ /I$(PGHOME)\include /I$(PGHOME)\include\server  /I$(PGHOME)\include\server\port\win32_msvc /I$(PGHOME)\include\server\port\win32 /D HAVE_LONG_INT_64 /D HAVE_GETACTIVESNAPSHOT /LD /Gd /Tc 

slony1_funcs.obj: slony1_funcs.c
	$(CPP) $(CPP_FLAGS)  slony1_funcs.c

slony1_funcs.dll: $(OBJS)
	$(LINK32) $(LINK32_FLAGS) $(OBJS)  
