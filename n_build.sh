#!/bin/bash
# build_n.sh - Build Vue y correr Neutralino

# Ruta a client y neutralino
CLIENT_DIR="./client"
NEU_DIR="./neutralino"

echo "🚀 Haciendo build de Vue..."
cd "$CLIENT_DIR" || exit
bun run build

echo "📦 Copiando build a Neutralino..."
cp -r dist/* "../$NEU_DIR/resources/"

echo "🧹 Borrando dist/ para liberar espacio..."
rm -rf dist

echo "🖥 Ejecutando Neutralino..."
cd "../$NEU_DIR" || exit
bunx @neutralinojs/neu run
