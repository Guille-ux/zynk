#!/bin/bash

# Ir a la carpeta donde vive este script
cd "$(dirname "$0")"

NANORC_DIR="$HOME/.config/nano"
GLOBAL_NANORC="$HOME/.nanorc"
INCLUDE_LINE='include "~/.config/nano/zynk.nanorc"'

echo "Installing Zynk Extension for nano..."

# 1. Crear la carpeta de configuración de nano si no existe
mkdir -p "$NANORC_DIR"

# 2. Copiar el archivo de sintaxis
if [ -f "zynk.nanorc" ]; then
    cp "zynk.nanorc" "$NANORC_DIR/"
    echo "✔️ zynk.nanorc copied to $NANORC_DIR/"
else
    echo "❌ Err: 'zynk.nanorc' not found on this dir."
    exit 1
fi

# 3. Comprobar y añadir el include en ~/.nanorc de forma segura
touch "$GLOBAL_NANORC"

if grep -Fxq "$INCLUDE_LINE" "$GLOBAL_NANORC"; then
    echo "Already included on .nanorc $GLOBAL_NANORC."
else
    echo "" >> "$GLOBAL_NANORC" 
    echo "$INCLUDE_LINE" >> "$GLOBAL_NANORC"

fi

echo "Install Complete!"
