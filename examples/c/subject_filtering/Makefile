# Copyright 2007-2018 United States Government as represented by the
# Administrator of The National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Rights Reserved.


GMSEC_HOME=../../..

include $(GMSEC_HOME)/config/$(GMSEC_PLATFORM)

SRC1    = publish_subject_filter.c
TARGET1 = publish_subject_filter

SRC2    = subscribe_subject_filter.c
TARGET2 = subscribe_subject_filter

default: checkDefs $(TARGET1) $(TARGET2)


clean: checkDefs
	$(RM) $(TARGET1) $(TARGET2)
	$(RM) *.gcno *.gcda

checkDefs:
	@ if [ -z $(TARGET1) ] || [ -z $(TARGET2) ]; then \
		echo "TARGET is not defined"; \
		exit 1; \
	fi


$(TARGET1): $(SRC1)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@


$(TARGET2): $(SRC2)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
