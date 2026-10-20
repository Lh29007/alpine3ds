# alpine3ds

Mini terminal tactile pour Nintendo 3DS, en homebrew (`.3dsx`).

- **Écran du haut** : sortie du terminal.
- **Écran du bas** : clavier QWERTY tactile (chiffres, lettres, `.`, espace, effacer `<-`, valider `OK`).
- **Commandes** : `help`, `clear`, `fetch` (infos système façon screenfetch), `exit`.
- **START** : quitter.

## Compiler

Avec devkitARM + libctru + citro2d + citro3d installés (image Docker `devkitpro/devkitarm`) :

```sh
make
```

Produit `alpine3ds.3dsx`.

Sans toolchain local, le workflow GitHub Actions compile à chaque push sur `main`.

## Où voir le résultat de la compilation

- Le log de chaque build est écrit dans **`logs/build-latest.log`**, commité automatiquement dans le dépôt (commit `[skip ci]`).
- Le `.3dsx` est disponible comme artefact dans l'onglet **Actions** du dépôt.

## Utilisation sur la console

1. Copier `alpine3ds.3dsx` dans `sd:/3ds/alpine3ds/` (carte SD).
2. Lancer depuis le Homebrew Launcher.
3. Taper les commandes sur le clavier du bas, valider avec `OK`.

## Limites actuelles

- Le payload est un **homebrew `.3dsx`**, pas un payload Luma (`.bin`). Le packaging Luma est une étape ultérieure.
- **Alpine Linux n'est pas installée** : `fetch` l'indique. Une rootfs Alpine (`alpine-minirootfs`) pourrait être embarquée plus tard, mais elle ne pourra pas tourner dans l'environnement 3DS sans un noyau Linux (projet `linux-3ds`), et la RAM disponible est très limitée (128 Mo, 256 Mo sur New 3DS).
- Pas de majuscules ni de caractères spéciaux (Shift non implémenté).
- Le terminal ne lance pas de vrais programmes : les commandes sont internes.
