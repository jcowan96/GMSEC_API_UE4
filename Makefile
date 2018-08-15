 
# Copyright 2007-2018 United States Government as represented by the
# Administrator of The National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Rights Reserved.




 
ifndef GMSEC_PLATFORM

%:
	@ echo "Error: GMSEC_PLATFORM is not defined."
	@ echo
	@ echo "Please refer to the 'INSTALL.txt' file for reference"
	@ echo "on how to setup your build environment."
	@ echo
	@ false

else

RELEASE    = ../release
GMSEC_HOME = .

include $(GMSEC_HOME)/config/$(GMSEC_PLATFORM)

.PHONY: check_support build library perl_api perl_api_3x perl_api_4x python_api python_api_4x env_val api_docs mw_wrapper clean install tools


default: check_support build library mw_wrappers java_api perl_api python_api env_val tools


check_support:
	@ if [ -d ../SUPPORT ]; then \
		echo "Found SUPPORT directory..." ; \
	fi


build:
	CXX=$(CXX) perl buildscript.pl	


library:
	$(MAKE) -C framework


java_api:
	$(MAKE) -C java


perl_api: perl_api_3x
# Note: Perl binding for API 4.x is currently only supported on Linux and Mac platforms.
ifeq ($(findstring linux,$(GMSEC_PLATFORM)), linux)
	$(MAKE) perl_api_4x
endif
ifeq ($(findstring macosx,$(GMSEC_PLATFORM)), macosx)
	$(MAKE) perl_api_4x
endif


perl_api_3x:
ifneq ($(findstring linux.x86_64-32bit,$(GMSEC_PLATFORM)), linux.x86_64-32bit)
	@echo
	@echo
	@echo "###########################################################"
	@echo "#"
	@echo "#  Building Perl 3.x binding of the GMSEC API"
	@echo "#"
	@echo "###########################################################"
	cd ./perl/gmsec; \
		PERL_CC=$(PERL_CC) perl -Iextra Makefile.PL PREFIX=../../bin ; \
		$(MAKE) ; $(MAKE) install;
endif


perl_api_4x:
	@if [ -z $(SWIG_HOME) ]; then \
		echo; \
		echo; \
		echo "###########################################################"; \
		echo "#"; \
		echo "# SWIG_HOME is not defined"; \
		echo "#"; \
		echo "# Skipping build of Perl binding for API 4.x"; \
		echo "#"; \
		echo "###########################################################"; \
	else \
		$(MAKE) -C perl/gmsec4; \
	fi


python_api:
# Note: Python binding for API 4.x is currently only supported on Linux and Mac platforms.
ifeq ($(findstring linux,$(GMSEC_PLATFORM)), linux)
ifneq ($(findstring linux.x86_64-32bit,$(GMSEC_PLATFORM)), linux.x86_64-32bit)
	$(MAKE) python_api_4x
endif
endif
ifeq ($(findstring macosx,$(GMSEC_PLATFORM)), macosx)
	$(MAKE) python_api_4x
endif


python_api_4x:
	@if [ -z $(SWIG_HOME) ]; then \
		echo; \
		echo; \
		echo "###########################################################"; \
		echo "#"; \
		echo "# SWIG_HOME is not defined"; \
		echo "#"; \
		echo "# Skipping build of Python binding for API 4.x"; \
		echo "#"; \
		echo "###########################################################"; \
	else \
		$(MAKE) -C python/gmsec4; \
	fi


env_val:
	@echo
	@echo
	@echo "###########################################################"
	@echo "#"
	@echo "#  Setting up GMSEC API Validator"
	@echo "#"
	@echo "###########################################################"
	mkdir -p ./bin/validator
	cp validator/env_validator.sh ./bin/validator
	cp validator/perl_ver.pl ./bin/validator
	cp validator/*.env ./bin/validator


tools:
	$(MAKE) -C tools


api_docs:
	$(MAKE) -C doxygen


mw_wrappers:
	$(MAKE) -C wrapper


all_examples:
	$(MAKE) -C examples INTERNAL=1


clean: clean-gcov
	$(RM) -r bin/amqp
	$(RM) -r bin/lib
	$(RM) -r bin/validator
	$(RM) bin/*
	@ for dir in framework examples wrapper java doxygen tools; do \
		$(MAKE) -C $$dir $@ ; \
	done
	- @ [ -f perl/gmsec/Makefile ] && $(MAKE) -C perl/gmsec veryclean
	- @ $(MAKE) -C perl/gmsec4 clean
	- @ $(MAKE) -C python/gmsec4 clean
	$(RM) c.ver os.ver java.ver

clean-gcov:
	@ for dir in framework examples wrapper; do \
		echo $(RM) -r $$dir/*.gcno $$dir/*.gcda ; \
	done


install:
	@ echo
	@ echo
	@ echo "###########################################################"
	@ echo "#"
	@ echo "# Installing the GMSEC API"
	@ echo "#"
	@ echo "###########################################################"
	if [ -d $(RELEASE) ] ; then rm -rf $(RELEASE) ; fi
	mkdir -p $(RELEASE)/GMSEC_API
	tar cf - *.txt bin config templates examples -C framework include \
		--exclude='.svn' --exclude=vendor \
		--exclude=secure --exclude=internal \
		| tar xf - -C $(RELEASE)/GMSEC_API
	(cd $(RELEASE); tar zcf GMSEC_API.tgz GMSEC_API)

install_docs:
	$(MAKE) install -C doxygen

endif

