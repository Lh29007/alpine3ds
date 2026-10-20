alpine3ds : Linux (linux-3ds) avec rootfs Alpine armhf, sans apk
===================================================================

Contenu du dossier :
  luma/payloads/down_firm_linux_loader.firm   loader officiel firm_linux_loader
  linux/zImage                                noyau linux-3ds officiel
  linux/nintendo3ds_ctr.dtb                   device tree (3DS / New 3DS)
  linux/nintendo3ds_ktr.dtb                   device tree (2DS / autres)
  linux/initramfs.cpio.gz                     rootfs Alpine + init + fetch

Installation
------------
1. Copier le contenu de ce dossier a la racine de la carte SD (fusionner avec
   les dossiers existants : luma/ et linux/).
2. Prerequis : une 3DS avec Luma3DS (custom firmware).
3. Eteindre la console, puis la rallumer en MAINTENANT LA TOUCHE BAS.
   Luma lance alors firm_linux_loader (prefixe down_).
4. Le noyau demarre, puis l'ecran affiche le resume "fetch" et un shell.

Utilisation
-----------
Le shell est ash (busybox) sur Alpine. Pas de gestionnaire de paquets (apk
retire) : seules les commandes presentes dans le rootfs sont disponibles.
Commande de resume systeme : fetch

Limites connues
---------------
- Rootfs limite a 8 Mo (zone initrd du loader).
- Pas d'apk, donc pas d'installation de paquets dans le systeme.
- Le clavier tactile du bas n'est pas utilisable depuis Linux : saisie
  avec un clavier USB ou la console serie si disponible.
- Non teste sur console au moment de la construction : les logs de build
  sont dans logs/linux-build.log du depot.
