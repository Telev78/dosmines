# Chemins Borland C++ 3.1
BC_PATH  = C:\BORLANDC
INCLUDES = -I$(BC_PATH)\INCLUDE
LIBS     = -L$(BC_PATH)\LIB
BGI      = $(BC_PATH)\BGI

# Par défaut, si DEBUG n'est pas défini, on est en Release
!ifdef DEBUG
CFLAGS = -ml -v -y -O-    # Model Large, Debug info, Pas d'optimisation
!else
CFLAGS = -ml -v- -O2 -vi -2 -y- # Model Large, Pas de debug info, Optimisation vitesse
!endif

BUILD_DIR = ..\build

TARGET   = $(BUILD_DIR)\dosmines.exe

# Fichiers objets attendus directement dans le dossier build pour le linker
BUILD_OBJS = $(BUILD_DIR)\main.obj $(BUILD_DIR)\minesapp.obj $(BUILD_DIR)\board.obj $(BUILD_DIR)\video.obj $(BUILD_DIR)\mouse.obj $(BUILD_DIR)\assetdef.obj $(BUILD_DIR)\egavga.obj

# 1. RÈGLE DE LIAISON FINALE
$(TARGET): main.obj minesapp.obj board.obj video.obj mouse.obj assetdef.obj egavga.obj
	bcc $(CFLAGS) $(LIBS) -e$(TARGET) @&&|
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