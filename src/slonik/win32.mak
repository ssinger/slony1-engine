CPP=cl.exe
LINK32=link.exe
LINK32_FLAGS=/libpath:c:\postgresql\9.0\lib libpq.lib libpgport.lib /NODEFAULTLIB:MSVCRT 
OBJS = slonik.obj \
	dbutil.obj \
	parser.obj \
	..\parsestatements\scanner.obj \
	scan.obj \


PGSHARE=\"c:\\postgresql\\9.0\\share\"
CPP_FLAGS=/c /D MSVC /D WIN32 /D PGSHARE=$(PGSHARE) /D YY_NO_UNISTD_H /I..\..\ /Ic:\Postgresql\9.0\include /Ic:\Postgresql\9.0\include/server /Ic:\Postgresql\9.0\include/server/port/win32 /MD

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