# invoke SourceDir generated makefile for empty.pem3
empty.pem3: .libraries,empty.pem3
.libraries,empty.pem3: package/cfg/empty_pem3.xdl
	$(MAKE) -f /home/janne/Downloads/JTKJ_labra/src/makefile.libs

clean::
	$(MAKE) -f /home/janne/Downloads/JTKJ_labra/src/makefile.libs clean

