# Démineur DOS Edition

Clone rétro du jeu **Démineur** (*Minesweeper*) développé en C++ pour **MS-DOS**, ciblant le compilateur **Borland C++ 3.1** en mode graphique VGA (640x480 en 16 couleurs, Mode 12h).

L'exécutable généré est **100% autonome (*standalone*)** : le pilote graphique BGI, les polices vectorielles ainsi que la sprite sheet par défaut sont directement incorporés dans le binaire sans nécessiter de fichiers annexes lors de l'exécution.

---

## Fonctionnalités

* **3 modes de difficulté classiques :**
  * **Débutant :** Grille 9x9 avec 10 mines.
  * **Intermédiaire :** Grille 16x16 avec 40 mines.
  * **Avancé :** Grille 30x16 avec 99 mines (occupant 480x256 pixels, parfaitement centrée à l'écran).
* **Mécaniques complètes de jeu :**
  * **Premier clic sécurisé :** Génération et placement des mines garantissant que le tout premier clic n'est jamais une mine.
  * **Cascade (Flood-fill) :** Révélation automatique récursive de toutes les zones vides adjacentes.
  * **Pose de drapeaux (clic droit) :** Décrémentation du compteur et protection des cases marquées.
  * **Option des points d'interrogation (`?`) :** Option commutable dans le menu principal (`[X] MARQUES (?)`, touche `M` ou clic souris) permettant un cycle à trois états (*Non révélée* $\rightarrow$ *Drapeau* $\rightarrow$ *Point d'interrogation* $\rightarrow$ *Non révélée*).
  * **Chording (clic gauche + droit simultanés) :** Dévoilement instantané des cases voisines lorsque les drapeaux nécessaires sont placés.
  * **Comportement souris ergonomique authentique :** Déclenchement au relâchement (*Mouse-Up*) avec annulation possible par glissement hors de la case ou du bouton (*Drag-Out*), et retour visuel enfoncé (*Push-Down* de la case ou du point d'interrogation).
* **Interface graphique et palette dynamique :**
  * Bandeau supérieur avec afficheurs LED rouges (mines restantes et chronomètre en temps réel jusqu'à 999 secondes).
  * Bouton émoji interactif au centre réagissant aux actions (visage normal, surpris lors du maintien du clic, lunettes de soleil en cas de victoire, croix en cas de défaite).
  * Rendu automatique du relief 3D (biseaux et panneaux pleins) adapté à la palette du thème utilisé.
  * Compatibilité avec les thèmes alternatifs : si un fichier `asset.bmp` externe est placé à côté du jeu, il est chargé en priorité ; sinon, le jeu bascule sur ses données intégrées.


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
2. Configurer la variable d'environnement système ou utilisateur `DOSBOX_PATH` pointant vers le dossier contenant l'exécutable de **DOSBox** (ou DOSBox-X / DOSBox Staging).
3. *(Facultatif)* Configurer le fichier `vscode.conf` situé dans le dossier de `DOSBOX_PATH` selon vos préférences de montage et de cycles CPU.

### Compilation via VS Code
Une fois les prérequis en place, le projet est directement compilable depuis **Visual Studio Code** via les tâches configurées (`Ctrl+Shift+B` ou menu *Terminal > Run Build Task*). La tâche lance DOSBox qui exécute Borland `MAKE.EXE` sur le `makefile` du projet, générant l'exécutable final dans le dossier `build/dosmines.exe`.

### Compilation via l'IDE Borland C++
Un fichier de projet est également disponible dans le dossier `PROJECT/` :
1. Lancez l'environnement Borland C++ (`BC.EXE`) dans DOSBox ou sur votre machine DOS.
2. Ouvrez le projet situé dans le répertoire `PROJECT/`.
3. Lancez la compilation directe (*Compile > Build all*).

---

## Outils d'accompagnement (`tools/`)

Le répertoire `tools/` contient trois scripts Python utilitaires pour la gestion des ressources graphiques :

* **`tools/bin2c.py` :** Convertit un fichier d'asset binaire (`asset.bmp`) en tableau d'octets C++ (`src/assetdef.cpp` et `src/assetdef.h`) qualifié en mémoire `far` pour intégrer directement la sprite sheet dans l'exécutable lors de la compilation.
* **`tools/palette.py` :** Inspecte un fichier BMP 4-bits ou 8-bits et extrait sa table de palette sous forme d'image PNG claire, affichant les pavés de couleurs, leurs index et leurs codes hexadécimaux disposés en quinconce.
* **`tools/remap_palette.py` :** Normalise et réordonne la palette d'un fichier BMP 4-bits sans altérer aucun pixel de l'image. Il veille notamment à positionner la couleur la plus sombre à l'index `0` (contour du curseur souris DOS) et la couleur la plus claire au dernier index, en préservant strictement la taille du fichier.

---

## Remerciements & Crédits

* **Développement :** Telev avec l'aide d'agent IA
* **Ressources graphiques :** Un grand merci à **Black Squirrel** pour la création de la sprite sheet originale du Démineur.
