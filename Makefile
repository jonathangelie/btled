SUBDIRS := src 

.PHONY: clean install

all: 
	@echo Building all object files
	@for i in $(SUBDIRS); do if [ -d $i ]; then cd $$i; make; cd ..; fi; done

package: 
	@echo Building package
	make -C ./src package

clean:
	@echo Clean all object files
	@for i in $(SUBDIRS); do if [ -d $i ]; then cd $$i; make clean; cd ..; fi; done

distclean:
	@echo Distclean all object files
	@for i in $(SUBDIRS); do if [ -d $i ]; then cd $$i; make distclean; cd ..; fi; done; rm -rf bin/

scratch: distclean all

# DO NOT DELETE
