OBJS	+= jbf2html.o
OBJS	+= base64.o
OBJS	+= jbf.o

CFLAGS	+= -g3
CFLAGS	+= -O3
CFLAGS	+= -Wall

jbf2html: $(OBJS)
	$(CC) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) jbf2html.exe jbf2html
