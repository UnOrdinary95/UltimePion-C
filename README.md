<p align="center">
  <img src="assets/images/icon.png" alt="Logo de l’Université Gustave Eiffel" width="72">
</p>

<h1 align="center">L’Ultime Pion</h1>

<p align="center">Un jeu de stratégie pour deux joueurs, réalisé en C dans le cadre d’un DM de Licence Informatique.</p>

## Groupe

- **Groupe :** TP2
- **Binôme :** Hernani CASTRO DE ALMEIDA et Rémi TRIBUT

## Compilation et lancement

Depuis la racine du projet, pour compiler et lancer la version terminal :

```sh
gcc src/ultime_pion.c -o src/ultime_pion -Wall -std=c17 && ./src/ultime_pion
```

Pour compiler et lancer la version graphique, GCC, Make, `pkg-config` et la bibliothèque MLV doivent être installés :

```sh
make run
```

Lancez la version graphique depuis la racine du projet pour que les images des pièces soient chargées correctement.

## Comment jouer

Les règles sont décrites dans le sujet. Dans la version terminal, suivez les indications affichées pour saisir les positions demandées.

Dans la version graphique, le joueur 1 joue les pièces noires et le joueur 2 les pièces blanches. À chaque tour, chacun sélectionne avec le clic gauche une de ses pièces, puis sa case de destination. Les deux déplacements sont résolus après le choix du second joueur.

La pièce sélectionnée est surlignée, mais les destinations possibles ne sont pas indiquées afin de conserver le système de pénalités. Pour désélectionner une pièce, cliquez à nouveau dessus ; le bouton **Annuler la sélection / le coup** permet aussi d’annuler le choix en cours, avant sa validation. Si un redéploiement est demandé, choisissez une case vide hors de votre dernière ligne.

Le panneau latéral affiche le tour, les pénalités et le résultat de la partie. Le bouton **Quitter** permet de fermer le jeu.

## Difficultés rencontrées

Le programme a été découpé en nombreuses fonctions, chacune chargée d’une tâche précise. La documentation de leurs paramètres et de leurs valeurs de retour aide à comprendre le rôle de chaque valeur sans devoir parcourir constamment le code. Comme il n’y avait pas d’ordre évident pour créer les fonctions, nous sommes partis des fonctions principales, puis avons construit progressivement les éléments du jeu avant de les assembler.

Pour la version MLV, nous avons également dû construire les boutons de l’interface à partir de rectangles et de zones cliquables. Cela a demandé un peu de travail supplémentaire, sans constituer une difficulté majeure.

L’icône utilisée est celle de l’Université Gustave Eiffel.
