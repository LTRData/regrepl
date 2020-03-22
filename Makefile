!IF "$(CPU)" == ""
CPU=$(_BUILDARCH)
!ENDIF

!IF "$(CPU)" == ""
CPU=i386
!ENDIF

$(CPU)\regrepl.exe: $(CPU)\regrepl.obj regrepl.res regrepl.rc.h ..\lib\minwcrt.lib
	link /nologo /opt:ref,icf=10 /fixed:no /out:$(CPU)\regrepl.exe $(CPU)\regrepl.obj regrepl.res

$(CPU)\regrepl.obj: regrepl.cpp regrepl.rc.h ..\include\winstrct.h
	cl /nologo /c /Yc /WX /W4 /Ox /GFS- /GR- /D_CRT_SECURE_NO_WARNINGS /wd4996 /MD /Fp$(CPU)\regrepl /Fo$(CPU)\regrepl regrepl.cpp

regrepl.res: regrepl.rc regrepl.rc.h regrepl.ico
	rc regrepl.rc

install: i386\regrepl.exe p:\utils\regrepl.exe

p:\utils\regrepl.exe: regrepl.exe
	xcopy /d/y regrepl.exe p:\utils\

clean:
	del /s *~ *.obj *.res *.pch
