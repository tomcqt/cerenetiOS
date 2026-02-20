# ===============================
#           cerenetiOS
# ===============================

# ----------------------------
# makefile options
# ----------------------------

NAME        = cereneti
ICON        = icon.png
DESCRIPTION = "Full operating system replacement for the TI-84 Plus CE graphing calculator, written in C++."
COMPRESSED  = YES
ARCHIVED    = YES

COMPRESSED_MODE = zx7 # use zx0 for release

CXXFLAGS =  -Wall -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Wwrite-strings 
CXXFLAGS += -Waggregate-return -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code -Oz -Wno-unused-command-line-argument -Wcast-qual
CXXFLAGS += -Wno-sign-conversion -Wno-implicit-int-conversion -I$(INCLUDE_DIR) -I$(GENERATED_DIR) -std=c++1z

CFLAGS   =  -Wall -Wextra -Oz -Wno-unused-command-line-argument

# ----------------------------
# directories
# ----------------------------

SRCDIR        = src generated
ASSETS_DIR    = assets
GENERATED_DIR = generated
OBJDIR        = build
INCLUDE_DIR   = include

# CEDev Makefile include
include $(shell cedev-config --makefile)

# ----------------------------
# graphics generation
# ----------------------------

CONVIMG_YAMLS = $(ASSETS_DIR)/convimg.yaml

$(GENERATED_DIR):
	mkdir -p $(GENERATED_DIR)

generate:
	@echo "[compiling] Generating graphics..."
	convimg -i assets/convimg.yaml
	cp ./*.c generated/images
	cp ./*.h generated/images
	rm ./*.c ./*.h

# ----------------------------
# font generation
# ----------------------------

FONTS := $(wildcard $(ASSETS_DIR)/fonts/*.fnt)
FONT_INCS := $(patsubst $(ASSETS_DIR)/fonts/%.fnt,$(GENERATED_DIR)/fonts/%.inc,$(FONTS))

$(GENERATED_DIR)/fonts/%.inc: $(ASSETS_DIR)/fonts/%.fnt | $(GENERATED_DIR)/fonts
	@echo "Generating font $< → $@"
	convfont -o carray -f $< $@

fonts: $(FONT_INCS)

# ensure generated files are built before compilation
$(OBJDIR)/$(NAME).8xp: generate fonts

.PHONY: generate fonts