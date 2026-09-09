# Chemins Borland C++ 3.1
BC_PATH  = C:\BORLANDC
INCLUDES = -I$(BC_PATH)\INCLUDE
LIBS     = -L$(BC_PATH)\LIB
BGI      = $(BC_PATH)\BGI

# Options du compilateur : -ml (Modèle Large)
CFLAGS   = -ml $(INCLUDES)

BUILD_DIR = ..\build

TARGET   = $(BUILD_DIR)\dosmines.exe

# Fichiers objets attendus directement dans le dossier build pour le linker
BUILD_OBJS = $(BUILD_DIR)\main.obj $(BUILD_DIR)\minesapp.obj $(BUILD_DIR)\board.obj $(BUILD_DIR)\video.obj $(BUILD_DIR)\mouse.obj $(BUILD_DIR)\assetdef.obj $(BUILD_DIR)\egavga.obj

# 1. RÈGLE DE LIAISON FINALE
$(TARGET): main.obj minesapp.obj board.obj video.obj mouse.obj assetdef.obj egavga.obj
	bcc -ml $(LIBS) -e$(TARGET) @&&|
$(BUILD_OBJS) graphics.lib
|

egavga.obj: $(BGI)\EGAVGA.BGI
	$(BGI)\bgiobj $(BGI)\EGAVGA.BGI $(BUILD_DIR)\egavga.obj _EGAVGA_driver

# 2. RÈGLE GÉNÉRIQUE DE COMPILATION
.cpp.obj:
	bcc $(CFLAGS) -n$(BUILD_DIR) -c $<

# Nettoyage complet
clean:
	del $(BUILD_DIR)\*.obj
	del $(TARGET)
	del $(BUILD_DIR)\log.txt