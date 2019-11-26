DESCRIPTION = Miscellaneous terminal utilites
URL = https://github.com/ikle/term

CFLAGS += -pthread -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600

include make-core.mk
