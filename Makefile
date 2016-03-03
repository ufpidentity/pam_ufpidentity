CC?=gcc
ODIR=.
LIBS=-lpam -lufpidentity
LIBDIR?=$(PREFIX)/lib

_OBJ = pam_ufpidentity.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -fPIC -c -o $@ $< $(CFLAGS)

pam_ufpidentity.so: $(OBJ)
	gcc -shared -Wl,-soname,$@ -o $@ $^ $(LIBS) -Wl,-z,defs

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~

install: pam_ufpidentity.so
	install -m 644 pam_ufpidentity.so $(DESTDIR)$(LIBDIR)/security 
