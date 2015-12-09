CC=gcc
ODIR=.
LIBS=-lpam -lufpidentity

_OBJ = pam_ufpidentity.o arguments.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -fPIC -c -o $@ $< $(CFLAGS)

pam_ufpidentity.so: $(OBJ)
	gcc -shared -o $@ $^ $(LIBS) -Wl,-z,defs

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
