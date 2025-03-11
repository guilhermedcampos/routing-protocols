TARGETS = dv-simulator dvrpp-simulator pv-simulator ls-simulator

CC = g++
CFLAGS = -Wall -O0 -g
LD = g++
LDFLAGS =

default: $(TARGETS)

dv-simulator: dv.o routing-simulator.o
dvrpp-simulator: dvrpp.o routing-simulator.o
pv-simulator: pv.o routing-simulator.o
ls-simulator: ls.o routing-simulator.o

$(TARGETS):
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) -MT $@ -MMD -MP -MF $@.d $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) -MT $@ -MMD -MP -MF $@.d $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGETS) *.o *.d

-include $(wildcard *.d)

submit:
	@read -p "Enter commit message: " MESSAGE; \
	rm -f build-report.md; \
	git add .; \
	git commit -m "$$MESSAGE"
	@git push
	@git tag -f project2-submission
	@git push -f origin project2-submission