
EXE = rpl-simulator
OBJS = main.o event.o node.o system.o scenario.o gui/mainwin.o gui/simfield.o gui/legend.o gui/dialogs.o proto/measure.o proto/phy.o proto/mac.o proto/ip.o proto/icmp.o proto/rpl.o
CFLAGS = -Wall -g3 -pg -pthread -std=gnu99 `pkg-config --cflags gtk+-2.0 gthread-2.0`
LDFLAGS = -Wall -g3 -pg -rdynamic -pthread -lm `pkg-config --libs gtk+-2.0 gthread-2.0 gmodule-export-2.0`

CC = gcc
RM = rm -f

all: $(EXE)

clean:
	$(RM) $(OBJS) $(EXE)

$(EXE) : $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.o:
	$(CC) -c $< $(CFLAGS) -o $@

main.o: main.c main.h base.h node.h system.h event.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h gui/mainwin.h gui/dialogs.h

event.o: event.c event.h base.h node.h system.h gui/mainwin.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h

node.o: node.c node.h base.h system.h event.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h

system.o: system.c system.h base.h node.h event.h gui/simfield.h gui/mainwin.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h

scenario.o: scenario.c scenario.h base.h node.h event.h system.h gui/mainwin.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h

gui/mainwin.o: gui/mainwin.c gui/mainwin.h base.h node.h main.h system.h event.h gui/simfield.h gui/dialogs.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h

gui/simfield.o: gui/simfield.c gui/simfield.h base.h node.h main.h system.h event.h gui/mainwin.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h 

gui/legend.o: gui/legend.c gui/legend.h base.h node.h gui/mainwin.h gui/simfield.h system.h event.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h

gui/dialogs.o: gui/dialogs.c gui/dialogs.h base.h

proto/measure.o: proto/measure.c proto/measure.h base.h node.h event.h system.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h

proto/phy.o: proto/phy.c proto/phy.h base.h node.h system.h event.h proto/measure.h proto/mac.h proto/ip.h proto/icmp.h proto/rpl.h

proto/mac.o: proto/mac.c proto/mac.h base.h node.h system.h event.h proto/measure.h proto/phy.h proto/ip.h proto/icmp.h proto/rpl.h

proto/ip.o: proto/ip.c proto/ip.h base.h node.h system.h event.h proto/measure.h proto/phy.h proto/mac.h proto/icmp.h proto/rpl.h

proto/icmp.o: proto/icmp.c proto/icmp.h base.h node.h system.h event.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/rpl.h

proto/rpl.o: proto/rpl.c proto/rpl.h base.h node.h system.h event.h proto/measure.h proto/phy.h proto/mac.h proto/ip.h proto/icmp.h 
