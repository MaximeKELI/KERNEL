#!/bin/bash
# Deployment script for kernel

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

TARGET="qemu"
USB_DEVICE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--target)
            TARGET="$2"
            shift 2
            ;;
        -d|--device)
            USB_DEVICE="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -t, --target TARGET  Deployment target (qemu|usb|iso)"
            echo "  -d, --device DEVICE  USB device for USB deployment"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Build kernel
echo -e "${GREEN}Building kernel for deployment...${NC}"
make clean
make

case $TARGET in
    qemu)
        echo -e "${GREEN}Deploying to QEMU...${NC}"
        make run
        ;;
    usb)
        if [ -z "$USB_DEVICE" ]; then
            echo -e "${RED}USB device not specified!${NC}"
            echo "Usage: $0 --target usb --device /dev/sdX"
            exit 1
        fi
        echo -e "${YELLOW}WARNING: This will overwrite $USB_DEVICE${NC}"
        read -p "Continue? (y/N) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo -e "${GREEN}Writing ISO to USB device...${NC}"
            sudo dd if=build/kernel.iso of=$USB_DEVICE bs=4M status=progress
            sync
            echo -e "${GREEN}USB deployment completed!${NC}"
        fi
        ;;
    iso)
        echo -e "${GREEN}ISO image ready at: build/kernel.iso${NC}"
        ;;
    *)
        echo -e "${RED}Unknown target: $TARGET${NC}"
        exit 1
        ;;
esac
