CPP=cl.exe
LINK32=link.exe

!if "$(PGVER)" == "8.3" 
PORTLIBS=libpgport.lib
!elseif  "$(PGVER)" == "8.4"
PORTLIBS=libpgport.lib
!elseif  "$(PGVER)" == "9.0"
PORTLIBS=libpgport.lib
!elseif  "$(PGVER)" == "9.1"
PORTLIBS=libpgport.lib
!elseif  "$(PGVER)" == "9.2"
PORTLIBS=libpgport.lib
!else
PORTLIBS=libpgport.lib libpgcommon.lib
!endif

LINK32_FLAGS=/libpath:$(PG_LIB) libpq.lib $(PORTLIBS) ws2_32.lib kernel32.lib user32.lib advapi32.lib /libpath:$(GETTEXT_LIB) intl.lib
OBJS = slonik.obj \
	dbutil.obj \
	parser.obj \
	..\parsestatements\scanner.obj \
	scan.obj \



CPP_FLAGS=/c /D MSVC /D WIN32 /D PGSHARE=\"$(PGSHARE)\" /D YY_NO_UNISTD_H /I..\..\ /D HAVE_PGPORT /I$(PG_INC) /I$(PG_INC)\server /I$(PG_INC)\server\port\win32 /MD

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
