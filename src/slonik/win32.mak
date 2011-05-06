CPP=cl.exe
LINK32=link.exe
PGHOME=c:\\postgresql\\9.0
LINK32_FLAGS=/libpath:$(PGHOME)\lib libpq.lib libpgport.lib kernel32.lib
OBJS = slonik.obj \
	dbutil.obj \
	parser.obj \
	..\parsestatements\scanner.obj \
	scan.obj \



CPP_FLAGS=/c /D MSVC /D WIN32 /D PGSHARE=\"$(PGHOME)/share\" /D YY_NO_UNISTD_H /I..\..\ /I$(PGHOME)\include  /MD

slonik.obj: slonik.c
	$(CPP)$(CPP_FLAGS)  slonik.c

dbutil.obj: dbutil.c
	$(CPP) $(CPP_FLAGS) dbutil.c

parser.obj: parser.c
	$(CPP) $(CPP_FLAGS) parser.c

../parsestatements/scanner.obj: ..\parsestatements\scanner.c
	$(CPP) $(CPP_FLAGS) /Fo..\parsestatements\scanner.obj ..\parsestatements/scanner.c 

scan.obj: scan.c
	$(CPP) $(CPP_FLAGS) scan.c

slonik.exe: $(OBJS)
	$(LINK32) $(LINK32_FLAGS) $(OBJS) 