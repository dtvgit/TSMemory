ASM = ml
AFLAG = /c /W3 /coff
# AFLAG = /c /coff /Zd /Zi
# CC = cl
# CFLAG = /c /Gr /GF /Oa /Ob2 /Ot /Og /Oi /Op /MT /W4 /Zp1 /D "NDEBUG"
CC = icl
CFLAG = /c /MT /O1 /Oy /Oi /Ob1 /Qsfalign16 /Zp1 /Gr /W3 /GS /GF /Qvc10 /Qip /fp:double /arch:IA32
LINK = xilink
LFLAG = /DLL /nologo /EXPORT:vfGetPluginInfo /EXPORT:vfGetPluginFunc /EXPORT:vfGetPluginFuncEditExt /EXPORT:GetInputPluginTable kernel32.lib user32.lib advapi32.lib 
RC = rc
BINDIR = "..\release\"

RES = m2v.res

ALL: m2v.vfp

!INCLUDE depend.mak

m2v.vfp: $(OBJ) $(RES)
	$(LINK) $(LFLAG) /OUT:m2v.vfp *.obj m2v.res

m2v.res: m2v.rc resource.h
	$(RC) m2v.rc

clean:
	DEL *.obj
	DEL *.res
	DEL *.exp
	DEL *.lib
	DEL m2v.vfp

install: all
	copy m2v.vfp $(BINDIR)m2v.vfp

depend: 
	ruby depend.rb > depend.mak

