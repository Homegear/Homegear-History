AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear

bin_PROGRAMS = homegear-history
homegear-history_SOURCES = main.cpp GD/GD.cpp
homegear-history_LDADD = -lpthread -lgcrypt -lgnutls -lhomegear-base -lgpg-error -lsqlite3

if BSDSYSTEM
else
homegear-history_LDADD += -ldl
endif
