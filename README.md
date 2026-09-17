# Démineur DOS Edition

Clone rétro du jeu **Démineur** (*Minesweeper*) développé en C++ pour **MS-DOS**, ciblant le compilateur **Borland C++ 3.1**. 

Le moteur graphique est **100% hybride et compatible de manière native avec les architectures EGA et VGA** grâce à une détection matérielle automatique au démarrage.

L'exécutable généré est **100% autonome (*standalone*)** : le pilote graphique BGI, les polices vectorielles ainsi que la sprite sheet par défaut sont directement incorporés dans le binaire sans nécessiter de fichiers annexes lors de l'exécution.

---

## Fonctionnalités

* **Double compatibilité matérielle native (EGA / VGA) :**
  * **Mode VGAHI :** Résolution 640x480 en 16 couleurs. Injection de la palette du fichier BMP indexé (lue depuis ses structures d'en-tête RGB) directement dans les registres du DAC VGA.
  * **Mode EGAHI :** Résolution 640x350 en 16 couleurs. Le moteur convertit mathématiquement et à la volée la table de couleurs indexée du BMP vers la palette physique 6-bits (64 couleurs) de l'EGA via ses registres d'attributs matériels.
  * L'ensemble de l'interface (grille, menus, crédits) s'adapte et se recentre dynamiquement selon la hauteur de l'écran détectée (350px ou 480px).
* **3 modes de difficulté classiques :**
  * **Débutant :** Grille 9x9 avec 10 mines.
  * **Intermédiaire :** Grille 16x16 avec 40 mines.
  * **Avancé :** Grille 30x16 avec 99 mines (compactée intelligemment pour s'intégrer sans débordement dans les 350 pixels de haut du mode EGA).
* **Mécaniques complètes et fidélité chirurgicale (Mouse Focus) :**
  * **Premier clic sécurisé :** Génération et placement des mines garantissant que le tout premier clic n'est jamais une mine.
  * **Cascade (Flood-fill) :** Révélation automatique récursive optimisée (sans récursion de pile) de toutes les zones vides adjacentes.
  * **Gestion rigoureuse du Mouse-Up :** Toutes les actions (clic gauche pour creuser, clic droit pour marquer) sont validées *uniquement* au relâchement du bouton. Un glissement hors de la case cible (*Drag-Out*) annule proprement l'action.
  * **Survol dynamique (Focus) :** Maintenir un bouton enfoncé et glisser hors d'une case restaure son relief. Revenir sur la case initiale réactive visuellement l'enfoncement (*Push-Down* de la case ou du point d'interrogation), offrant un comportement identique à la version Windows 3.1 originale.
  * **Chording visuel dynamique :** Maintenir le clic gauche + droit simultanés enfonce visuellement la case ciblée et ses 8 voisines éligibles. Le survol suit le curseur et le coup n'est validé au relâchement que si la souris n'a pas quitté la cellule d'ancrage initiale.
  * **Option des points d'interrogation (`?`) :** Option commutable dans le menu principal permettant un cycle complet à trois états.
* **Interface graphique et Palette :**
  * Bandeau supérieur avec afficheurs LED rouges (mines restantes et chronomètre en temps réel jusqu'à 999 secondes).
  * Bouton émoji interactif au centre réagissant aux actions et au survol (visage normal, enfoncé/cliqué au survol, surpris lors des clics sur la grille, lunettes de soleil en cas de victoire, croix en cas de défaite).
  * Algorithme de conversion de palette optimisé préservant les biseaux et reliefs 3D de l'interface en séparant proprement les intensités lumineuses sur EGA.
  * Compatibilité avec les thèmes alternatifs : si un fichier `asset.bmp` externe est présent, il est chargé en priorité ; sinon, le jeu bascule sur ses données intégrées en mémoire `far`.

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
3. *(Recommandé sous 86Box)* Pour tester le mode EGA de manière authentique, assurez-vous de configurer votre machine virtuelle 86Box avec une carte graphique **EGA dotée de 256 Ko de mémoire vidéo** (pour éviter les conflits d'allocation de plans de mémoire avec le pilote de souris DOS matérielle en résolution 640x350).

### Compilation via VS Code
Le projet est directement compilable depuis **Visual Studio Code** via les tâches configurées (`Ctrl+Shift+B` ou menu *Terminal > Run Build Task*). La tâche lance DOSBox qui exécute Borland `MAKE.EXE` sur le `makefile` du projet, générant l'exécutable final autonome dans le dossier `build/dosmines.exe`.

### Compilation via l'IDE Borland C++
Un fichier de projet est également disponible dans le dossier `PROJECT/` :
1. Lancez l'environnement Borland C++ (`BC.EXE`) dans DOSBox ou sur votre machine DOS.
2. Ouvrez le projet situé dans le répertoire `PROJECT/`.
3. Lancez la compilation directe (*Compile > Build all*).

---

## Outils d'accompagnement (`tools/`)

Le répertoire `tools/` contient trois scripts Python indispensables pour la gestion des ressources graphiques et la personnalisation des thèmes :

* **`tools/bin2c.py` :** Convertit un fichier d'asset binaire (`asset.bmp`) en tableau d'octets C++ (`src/assetdef.cpp` et `src/assetdef.h`) qualifié en mémoire `far` pour intégrer directement la sprite sheet dans l'exécutable lors de la compilation.
* **`tools/palette.py` :** Inspecte un fichier BMP 4-bits ou 8-bits indexé et extrait sa table de palette sous forme d'image PNG claire, affichant les pavés de couleurs, leurs index et leurs codes hexadécimaux disposés en quinconce.
* **`tools/remap_palette.py` :** Normalise et réordonne la palette d'un fichier BMP indexé sans altérer aucun pixel de l'image. Il résout la subtilité matérielle des pilotes de souris DOS en forçant la couleur la plus sombre à l'index `0` et le **blanc pur à l'index `15`**, garantissant que le pointeur de souris matériel conserve sa couleur et sa stabilité, peu importe le thème graphique appliqué au jeu.

---

## Remerciements & Crédits

* **Développement :** Telev avec l'aide d'un agent IA.
* **Ressources graphiques :** Un grand merci à **Black Squirrel** pour la création de la sprite sheet originale du Démineur.
