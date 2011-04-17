.SUFFIXES :
OBJDIR := obj
EXTOBJDIR := ../../obj
BINDIR := bin
SRCDIR := src
TSTDIR := test
INCDIR := include

LDFLAGS := -lpthread -lGL -lGLU -lSDL -lSDL_image -lSDL_gfx
CFLAGS := -pedantic -Wall -ggdb -I$(INCDIR)
CX := g++ -c $(CFLAGS)
CC := gcc -c $(CFLAGS) -std=c99
LD := g++ $(LDFLAGS)

BINS := vxl shp_dump vxl_dump hva_dump map_dump shp_conv tmp_dump tmp_conv
vxlOBJS := VXLFile Palette Display VoxelRenderer vxl Input HVAFile
vxl_dumpOBJS := VXLFile vxl_dump Palette
hva_dumpOBJS := HVAFile hva_dump
map_dumpOBJS := Base64 INIFile LZODecompress minilzo map_dump Display MapReader Palette
shp_dumpOBJS := SHPFile shp_dump
shp_convOBJS := SHPFile Palette shp_conv
tmp_dumpOBJS := TMPFile tmp_dump
tmp_convOBJS := TMPFile Palette tmp_conv

.PHONY: all
all : $(BINS)

.PHONY : clean
clean :
	@echo "  RMDIR      $(OBJDIR)"
	@rm -rf $(OBJDIR)
	@echo "  RMDIR      $(BINDIR)"
	@rm -rf $(BINDIR)

.PHONY : help
help :
	@echo "Binaries:"
	@echo "  $(BINS)"
	@echo
	@echo "Other targets:"
	@echo "  help - Prints this help message"
	@echo "  clean - Removes all generated files"
	@echo "  $(OBJDIR)/file.o - Build a single object"
	@echo "  info - Prints a list of variables"

.PHONY : info
info :
	@echo "CC	- '$(CC)'"
	@echo "LD	- '$(LD)'"
	@echo "CFLAGS	- '$(CFLAGS)'"
	@echo "LDFLAGS	- '$(LDFLAGS)'"

$(OBJDIR) $(BINDIR) :
	@echo "  MKDIR      $@"
	@mkdir -p $@

$(OBJDIR)/%.o : $(SRCDIR)/%.cpp | $(OBJDIR)
	@echo "  CC         $@"
	@$(CX) -MMD -MP -o $@ $<

$(OBJDIR)/%.o : $(SRCDIR)/%.c | $(OBJDIR)
	@echo "  CC         $@"
	@$(CX) -MMD -MP -o $@ $<

$(OBJDIR)/%.o : $(TSTDIR)/%.cpp | $(OBJDIR)
	@echo "  CC         $@"
	@$(CX) -MMD -MP -o $@ $<

-include $(addprefix $(OBJDIR)/, $(addsuffix .d, $(foreach BIN, $(BINS), $($(BIN)OBJS))))

.SECONDEXPANSION :
$(BINS) : $$(addprefix $(BINDIR)/, $$@)

$(addprefix $(BINDIR)/, $(BINS)) : $$(addprefix $(OBJDIR)/, $$(addsuffix .o, $$($$(notdir $$@)OBJS))) | $(BINDIR)
	@echo "  LD         $@"
	@$(LD) $(LDFLAGS) -o $@ $(filter-out $(BINDIR), $^)
