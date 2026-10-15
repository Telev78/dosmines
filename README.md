# Démineur DOS Edition

Clone rétro du jeu **Démineur** (*Minesweeper*) développé en C++ pour **MS-DOS**, ciblant le compilateur **Borland C++ 3.1**. 

Le moteur graphique est **compatible avec les architectures EGA et VGA** grâce à une détection matérielle automatique au démarrage.

L'exécutable généré est **100% autonome (*standalone*)** : le pilote graphique BGI, les polices vectorielles ainsi que la sprite sheet par défaut sont directement incorporés dans le binaire sans nécessiter de fichiers annexes lors de l'exécution.

---

## Fonctionnalités

* **Double compatibilité matérielle native avec correction d'Aspect Ratio (EGA / VGA) :**
  * **Mode VGAHI :** Résolution 640x480 en 16 couleurs. Les pixels sont naturellement carrés (ratio 1:1). Injection de la palette du fichier BMP indexé directement dans les registres du DAC VGA.
  * **Mode EGAHI :** Résolution 640x350 en 16 couleurs. Les pixels d'origine de cette résolution sont rectangulaires (étirés verticalement). **Le moteur intègre un algorithme d'écrasement vertical mathématique (Nearest-Neighbor) appliqué à la volée en RAM lors du chargement des sprites** (~73% sur l'axe Y). Une fois affichés sur le moniteur CRT 4:3 d'époque, les sprites (cases à 16x12, émojis à 24x18, LED à 13x17) reprennent des proportions visuelles carrées sans aucune perte de performance en jeu. *Ce mode nécessite une carte EGA équipée d'au moins 128 Ko de mémoire vidéo*.
  * L'ensemble de l'interface (grille, menus, boîtes de dialogue, crédits) ainsi que la taille des polices vectorielles et l'échelle des boutons s'adaptent et se recentrent dynamiquement selon la hauteur de l'écran détectée (350px ou 480px) pour garantir une esthétique et un aspect ratio identiques sur les deux architectures.
* **3 modes de difficulté classiques :**
  * **Débutant :** Grille 9x9 avec 10 mines.
  * **Intermédiaire :** Grille 16x16 avec 40 mines.
  * **Avancé :** Grille 30x16 avec 99 mines.
* **Mécaniques complètes et fidélité chirurgicale (Mouse Focus) :**
  * **Premier clic sécurisé :** Génération et placement des mines garantissant que le tout premier clic n'est jamais une mine.
  * **Cascade (Flood-fill) :** Révélation automatique (itérative sans récursion) de toutes les zones vides adjacentes.
  * **Gestion rigoureuse du Mouse-Up :** Toutes les actions (clic gauche pour creuser, clic droit pour marquer) sont validées *uniquement* au relâchement du bouton. Un glissement hors de la case cible (*Drag-Out*) annule proprement l'action.
  * **Survol dynamique (Focus) :** Maintenir un bouton enfoncé et glisser hors d'une case restaure son relief. Revenir sur la case initiale réactive visuellement l'enfoncement, offrant un comportement identique à la version Windows 3.1 originale.
  * **Chording visuel dynamique :** Maintenir le clic gauche + droit simultanés enfonce visuellement la case ciblée et ses 8 voisines éligibles. La détection physique des coordonnées de lignes s'adapte au pixel près selon la hauteur des cellules (16px en VGA, 12px en EGA) pour éviter tout décalage du pointeur.
  * **Option des points d'interrogation (`?`) :** Option commutable dans le menu principal permettant un cycle complet à trois états.
* **Interface graphique et Palette :**
  * Bandeau supérieur avec afficheurs LED rouges (mines restantes et chronomètre en temps réel jusqu'à 999 secondes) logés dans des rectangles noirs dynamiquement ajustés à la hauteur des chiffres.
  * Bouton émoji interactif au centre réagissant aux actions et au survol (visage normal, enfoncé/cliqué au survol, surpris lors des clics sur la grille, lunettes de soleil en cas de victoire, croix en cas de défaite).
  * Algorithme de conversion de palette optimisé préservant les biseaux et reliefs 3D de l'interface en séparant proprement les intensités lumineuses sur EGA et VGA.
  * Compatibilité avec les thèmes alternatifs : si un fichier `asset.bmp` externe est présent, il est chargé en priorité ; sinon, le jeu bascule sur ses données intégrées.

---

## Comment compiler le jeu

### Prérequis
1. Copier le dossier d'installation de **Borland C++ 3.1** (dossier `BORLANDC`) directement à la racine du projet, de façon à avoir l'arborescence :
   ```text
   DosMines/
   ├── BORLANDC/
   │   ├── BIN/
   │   ├── INCLUDE/
   │   ├── LIB/
   │   └── BGI/
   ├── PROJECT/
   ├── src/
   └── ...
   ```
2. L'environnement d'émulation (DOSBox, DOSBox Staging ou DOSBox-X / 86Box) doit être configuré avec un profil de rendu graphique réglé au **minimum sur `machine=ega`** ou idéalement `machine=svga_s3` (VGA par défaut) pour exécuter et tester le jeu correctement.

### Configuration de l'environnement VS Code
Le projet utilise le système de configuration de Visual Studio Code pour localiser l'émulateur DOSBox.

Avant de compiler, ajustez les chemins dans votre fichier `.vscode/settings.json` local selon votre installation :

```json
{
    "dosbox.windows.installPath": "C:\\Chemin\\Vers\\Votre\\Dossier\\DOSBox",
    "dosbox.windows.executableName": "dosbox.exe",
    "dosbox.windows.configPath": "${config:dosbox.windows.installPath}\\vscode.conf",
    "dosbox.linux.binaryName": "dosbox",
    "dosbox.linux.configPath": "${userHome}/.config/dosbox/vscode.conf"
}
```

### Compilation via VS Code
Le projet est directement compilable depuis **Visual Studio Code** via les tâches configurées :
* Vous pouvez lancer la tâche de build par défaut directement avec le raccourci `Ctrl+Shift+B` (ou via le menu *Terminal > Run Build Task*).
* Toutes les tâches personnalisées du projet (comme l'exécution de l'application ou le nettoyage du dossier de build) sont également accessibles à tout moment en ouvrant la **palette de commandes** (`Ctrl+Shift+P`), en saisissant `Tâches : exécuter la tâche` (ou `Tasks: Run Task`), puis en selecting la commande souhaitée dans la liste.

La tâche sélectionnée lancera automatiquement l'instance DOSBox configurée pour exécuter le traitement voulu.

### Compilation via l'IDE Borland C++
Un fichier de projet est également disponible dans le dossier `PROJECT/` :
1. Lancez l'environnement Borland C++ (`BC.EXE`) dans DOSBox ou sur votre machine DOS.
2. Ouvrez le projet situé dans le répertoire `PROJECT/`.
3. **⚠️ Note importante avant compilation :** L'IDE Borland requiert la présence des fichiers objets graphiques et polices vectorielles directement incorporés. Si vous n'avez pas exécuté la compilation VS Code au moins une fois, vous devez générer manuellement ces trois fichiers objets à l'aide de l'outil `BGIOBJ.EXE` inclus dans le répertoire `BORLANDC\BGI`. Exécutez les commandes DOS suivantes depuis le dossier racine de votre projet :
   ```cmd
   mkdir build
   .\BORLANDC\BGI\bgiobj .\BORLANDC\BGI\EGAVGA.BGI .\build\egavga.obj _EGAVGA_driver
   .\BORLANDC\BGI\bgiobj .\BORLANDC\BGI\SANS.CHR .\build\sans.obj _sansserif_font
   .\BORLANDC\BGI\bgiobj .\BORLANDC\BGI\LITT.CHR .\build\litt.obj _small_font
   ```
   *Sans cette étape, l'édition de liens (link) au sein de l'IDE Borland échouera.*
4. Lancez la compilation directe dans l'IDE (*Compile > Build all*).

---

## Outils d'accompagnement (`tools/`)

Le répertoire `tools/` contient trois scripts Python indispensables pour la gestion des ressources graphiques et la personnalisation des thèmes.

* **`tools/bin2c.py` :** Convertit un fichier d'asset binaire (`asset.bmp`) en tableau d'octets C++ (`src/assetdef.cpp` et `src/assetdef.h`) qualifié en mémoire `far` pour intégrer directement la sprite sheet dans l'exécutable lors de la compilation.
* **`tools/palette.py` :** Inspecte un fichier BMP 4-bits ou 8-bits indexé et extrait sa table de palette sous forme d'image PNG claire, affichant les pavés de couleurs, leurs index et leurs codes hexadécimaux disposés en quinconce.
* **`tools/remap_palette.py` :** Normalise et réordonne la palette d'un fichier BMP indexé sans altérer aucun pixel de l'image. Il résout la subtilité matérielle des pilotes de souris DOS en forçant la couleur la plus sombre à l'index `0` et le **blanc pur à l'index `15`**, garantissant que le pointeur de souris matériel conserve sa couleur et sa stabilité, peu importe le thème graphique appliqué au jeu.

---

## Remerciements & Crédits

* **Développement :** Telev avec l'aide d'un agent IA.
* **Ressources graphiques :** Un grand merci à **Black Squirrel** pour la création de la sprite sheet originale du Démineur.

---

## Captures d'écran

### Interface et Menus
<p align="center">
  <img src="docs/screenshots/menu_principal.png" width="45%" alt="Menu Principal - Sélection des difficultés" />
  <img src="docs/screenshots/meilleurs_temps.png" width="45%" alt="Tableau des Meilleurs Temps" />
</p>

### Comparaison des rendus matériels (VGA vs EGA avec correction d'Aspect Ratio)

Le moteur graphique s'adapte de manière native aux deux architectures pour offrir la même géométrie visuelle. Les captures d'écran ci-dessous restituent fidèlement l'expérience visuelle sur un moniteur CRT d'époque au ratio d'aspect 4:3.

Grâce au redimensionnement dynamique des sprites au démarrage et à l'adaptation automatique des mises en page, les éléments tentent de conserver une proportion carrée, que le jeu s'exécute en VGA ou en EGA :

#### 🥇 Écrans de Victoire (VGA vs EGA)
> Les cases conservent un aspect ratio carré sur les deux architectures. L'en-tête (compteurs de mines et chronomètre) s'ajuste à la hauteur réduite des sprites (16px en VGA contre 12px en EGA).

<p align="center">
  <img src="docs/screenshots/partie_gagne.png" width="48%" alt="Partie gagnée - Résolution VGA (640x480)" />
  <img src="docs/screenshots/partie_gagne_ega.png" width="48%" alt="Partie gagnée - Résolution EGA (640x350 corrigé)" />
</p>

#### ✍️ Fenêtres de Record (VGA vs EGA)

> La boîte globale de dialogue s'adaptent dynamiquement en hauteur pour effacer toute déformation verticale liée au mode EGA.

<p align="center">
  <img src="docs/screenshots/nouveau_record.png" width="48%" alt="Saisie du nom - Résolution VGA" />
  <img src="docs/screenshots/nouveau_record_ega.png" width="48%" alt="Saisie du nom - Résolution EGA" />
</p>

#### 💀 Écran de Défaite (VGA)
> Gestion complète des états de la grille avec révélation des mines et mise à jour de l'émoji central.

<p align="center">
  <img src="docs/screenshots/partie_perdu.png" width="60%" alt="Partie perdue en résolution VGA" />
</p>

