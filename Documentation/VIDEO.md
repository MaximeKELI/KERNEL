# Sous-système vidéo

## Composants

| Module | Rôle |
|--------|------|
| `framebuffer` | FB logiciel (PMM) ou Multiboot2 GRUB |
| `vga` | Texte 80x25 @ 0xB8000 |
| `vga_graphics` | Mode 13h 320x200x8 @ 0xA0000 |
| `drm` | Détection GPU PCI |
| `video_core` | Devices, modes, buffers, flip |
| `video_output` | Démo, blit FB→VGA |
| `font8x8` | Police minimale |

## kshell (après `init-full`)

```text
video           # infos
video demo      # motif test sur framebuffer
video blit      # copie vers VGA graphique (QEMU -vga std)
video gfx       # active mode 13h
video clear
```

## QEMU avec affichage

```bash
make iso
qemu-system-x86_64 -cdrom build/kernel.iso -m 512M -vga std -serial mon:stdio
```

Sans `-display none` pour voir le blit VGA.

## Multiboot2

Le header GRUB demande 640x480x32 ; si GRUB fournit un FB, il est utilisé directement.
