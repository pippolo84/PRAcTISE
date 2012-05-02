# Author:	Fabio Falzoi
# Date:		06/04/2012

CC		= gcc

# executable name
PRJCTNAME	= practise

SRCDIR		= src
INCDIR		= include
OBJDIR		= obj
DEPDIR		= dep

CFLAGS		= -O3 -g #-Wall -Wextra
INC_FLAG	= -I ./$(INCDIR)
ALL_CFLAGS	= $(INC_FLAG) $(CFLAGS)
LDFLAGS		= -lm -lrt -lpthread

SOURCES = $(wildcard ./$(SRCDIR)/*.c)
OBJS = $(SOURCES:./$(SRCDIR)/%.c=./$(OBJDIR)/%.o)
DEPS = $(SOURCES:./$(SRCDIR)/%.c=./$(DEPDIR)/%.d)

$(PRJCTNAME): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o  $@

./$(OBJDIR)/%.o : $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) -c $(ALL_CFLAGS) $<	-o	$@

./$(DEPDIR)/%.d: ./$(SRCDIR)/%.c | $(DEPDIR)
	@set -e; rm -f $@; \
	$(CC) $(INC_FLAG) -MM $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(SOURCES:./$(SRCDIR)/%.c=./$(DEPDIR)/%.d)

$(OBJDIR):
	mkdir $(OBJDIR)

$(DEPDIR):
	mkdir $(DEPDIR)

.PHONY: clean
clean:
	-rm -f $(PRJCTNAME) 
	-rm -rf $(OBJDIR) 
	-rm -rf $(DEPDIR)

