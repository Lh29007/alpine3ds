#!/usr/bin/env bash
# Construit le bundle SD : loader firm_linux_loader + noyau linux-3ds + rootfs Alpine (sans apk).
# Doit tourner en root (ownership des fichiers du rootfs). Utilise en CI : sudo -E bash scripts/build-bundle.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$ROOT/work"
OUT="$ROOT/dist"

ALPINE_MIRROR="https://dl-cdn.alpinelinux.org/alpine"
ALPINE_BRANCH="v3.20"
KERNEL_BASE="https://github.com/linux-3ds/linux/releases/download/latest"
LOADER_API="https://api.github.com/repos/linux-3ds/firm_linux_loader/releases/latest"
MAX_INITRD=$((8 * 1024 * 1024))   # zone initrd du loader : 0x27800000 -> 0x28000000

rm -rf "$WORK" "$OUT"
mkdir -p "$WORK" "$OUT"

echo "== 1. Alpine minirootfs armhf (${ALPINE_BRANCH}) =="
REL_DIR="$ALPINE_MIRROR/$ALPINE_BRANCH/releases/armhf"
MINI=$(curl -fsSL "$REL_DIR/latest-releases.yaml" | grep -o 'alpine-minirootfs-[0-9.]*-armhf\.tar\.gz' | head -n 1)
[ -n "$MINI" ] || { echo "ERREUR: minirootfs introuvable"; exit 1; }
echo "fichier : $MINI"
curl -fsSL -o "$WORK/$MINI" "$REL_DIR/$MINI"

echo "== 2. Preparation du rootfs =="
mkdir -p "$WORK/rootfs"
tar -xzf "$WORK/$MINI" -C "$WORK/rootfs" --numeric-owner
# pas d'apk : on retire le gestionnaire de paquets et ses caches
rm -rf "$WORK/rootfs/etc/apk" "$WORK/rootfs/lib/apk" "$WORK/rootfs/usr/share/apk" \
       "$WORK/rootfs/var/cache/apk" "$WORK/rootfs/sbin/apk" "$WORK/rootfs/usr/bin/apk"
# overlay du projet (init + fetch)
cp -a "$ROOT/rootfs/overlay/." "$WORK/rootfs/"
chmod 755 "$WORK/rootfs/init" "$WORK/rootfs/usr/bin/fetch"
echo "alpine3ds" > "$WORK/rootfs/etc/hostname"
du -sh "$WORK/rootfs"

echo "== 3. initramfs.cpio.gz =="
(cd "$WORK/rootfs" && find . -print0 | cpio --null -o -H newc --owner=0:0 2>/dev/null | gzip -9 -n) > "$WORK/initramfs.cpio.gz"
SIZE=$(stat -c%s "$WORK/initramfs.cpio.gz")
echo "taille : $SIZE octets (max $MAX_INITRD)"
if [ "$SIZE" -gt "$MAX_INITRD" ]; then
	echo "ERREUR: initramfs trop gros pour la zone du loader"
	exit 1
fi

echo "== 4. Noyau linux-3ds (release latest) =="
for f in zImage nintendo3ds_ctr.dtb nintendo3ds_ktr.dtb; do
	curl -fsSL -o "$WORK/$f" "$KERNEL_BASE/$f"
	echo "ok : $f ($(stat -c%s "$WORK/$f") octets)"
done

echo "== 5. Loader firm_linux_loader (release latest) =="
LOADER_URL=$(curl -fsSL "$LOADER_API" | jq -r '.assets[] | select(.name | endswith(".firm")) | .browser_download_url' | head -n 1)
if [ -z "$LOADER_URL" ]; then
	echo "ERREUR: aucun asset .firm dans la release firm_linux_loader"
	curl -fsSL "$LOADER_API" | jq -r '.assets[].name' || true
	exit 1
fi
echo "url : $LOADER_URL"
curl -fsSL -o "$WORK/down_firm_linux_loader.firm" "$LOADER_URL"

echo "== 6. Arborescence SD =="
SD="$WORK/sd"
mkdir -p "$SD/luma/payloads" "$SD/linux"
# prefixe down_ : Luma lance le loader en maintenant Bas au demarrage
cp "$WORK/down_firm_linux_loader.firm" "$SD/luma/payloads/"
cp "$WORK/zImage" "$WORK/nintendo3ds_ctr.dtb" "$WORK/nintendo3ds_ktr.dtb" "$SD/linux/"
cp "$WORK/initramfs.cpio.gz" "$SD/linux/"
cp "$ROOT/rootfs/SD-README.txt" "$SD/"
find "$SD" -maxdepth 3 | sort

echo "== 7. Sorties =="
(cd "$SD" && zip -qr "$OUT/alpine3ds-sd.zip" .)
cp "$WORK/initramfs.cpio.gz" "$OUT/"
ls -la "$OUT"
echo "OK : bundle construit"
