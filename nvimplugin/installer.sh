#!/bin/bash


cd "$(dirname "$0")"


DEST_DIR="$HOME/.config/nvim"

echo "Preparing Nvim Plugin Install on: $DEST_DIR"


mkdir -p "$DEST_DIR"


for carpeta in ftdetect syntax lua ftplugin; do
    if [ -d "$carpeta" ]; then
        echo "Copying Folder: $carpeta..."
        cp -r "$carpeta" "$DEST_DIR/"
    fi
done



echo "Install Completed!"
