## Credits to Shifaa and Wasim
## Wasimshebalny@gmail.com
## Shifaakhatib28@gmail.com

# List of subdirectories
SUBDIRS = q1 q2 q3 q3.5 q4
#UBDIRS = Q1 Q2 Q3 Q3.5 Q4 Q6

# Default target
all: $(SUBDIRS)

# Rule to build each subdirectory
$(SUBDIRS):
	$(MAKE) -C $@

# Recursive call to build all subdirectories
exe1:
	$(MAKE) -C q1

exe2:
	$(MAKE) -C q2

exe3:
	$(MAKE) -C q3

exe3.5:
	$(MAKE) -C q3.5

exe4:
	$(MAKE) -C q4

# exe6:
# 	$(MAKE) -C Q6

# Clean target for each subdirectory
.PHONY: clean $(SUBDIRS)
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
