
CFLAGS= 
COPT= -O2 -fomit-frame-pointer -ffast-math -std=gnu11
CWARN= -Wall -Wextra -Wredundant-decls
CWARNIGNORE= -Wno-unused-result -Wno-strict-aliasing
CINCLUDE= -Isrc/
CDEF=

#ifdef debug
CFLAGS+= -O0 -g -Wno-format -fno-omit-frame-pointer
CDEF+= -DEQP_DEBUG -DDEBUG
#else
#CFLAGS+= -DNDEBUG
#endif

##############################################################################
# eqp-server
##############################################################################
_EDP_OBJECTS=           \
 bg_thread              \
 bin                    \
 bit                    \
 crc                    \
 edp_array              \
 edp_atomic_posix       \
 edp_buffer             \
 edp_hash_tbl           \
 edp_semaphore_posix    \
 edp_string             \
 err_code               \
 hash                   \
 lex                    \
 main                   \
 parse                  \
 patch                  \
 pfs                    \
 ringbuf                \
 sqlite3

EDP_OBJECTS= $(patsubst %,build/%.o,$(_EDP_OBJECTS))

##############################################################################
# Core Linker flags
##############################################################################
LFLAGS= -rdynamic
LDYNAMIC= -pthread -lrt -lz -lcurl -ldl

##############################################################################
# Util
##############################################################################
Q= @
E= @echo
RM= rm -f 

##############################################################################
# Build rules
##############################################################################
.PHONY: default all clean

default all: edp

edp: bin/edp

bin/edp: $(EDP_OBJECTS)
	$(E) "Linking $@"
	$(Q)$(CC) -o $@ $^ $(LDYNAMIC) $(LFLAGS)
    
build/sqlite3.o: src/sqlite3.c $($(CC) -M src/sqlite3.c)
	$(E) "\033[0;32mCC     $@\033[0m"
	$(Q)$(CC) -c -o $@ $< $(CDEF) $(COPT) -fno-fast-math $(CWARN) $(CWARNIGNORE) $(CFLAGS) $(CINCLUDE)

build/%.o: src/%.c $($(CC) -M src/%.c)
	$(E) "\033[0;32mCC     $@\033[0m"
	$(Q)$(CC) -c -o $@ $< $(CDEF) $(COPT) $(CWARN) $(CWARNIGNORE) $(CFLAGS) $(CINCLUDE)

clean:
	$(Q)$(RM) build/*.o
	$(E) "Cleaned build directory"
