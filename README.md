# SysMon - Interfaz Gráfica

Este proyecto es una interfaz gráfica multiplataforma desarrollada con **Vue 3**, **Vite**, **Bun** y **Neutralinojs**.

## Tecnologías utilizadas

- **Vue 3**: Framework progresivo para construir interfaces de usuario modernas y reactivas.
- **Vite**: Herramienta de construcción rápida para proyectos frontend, utilizada para el desarrollo y build de la aplicación Vue.
- **Bun**: Runtime moderno para JavaScript y TypeScript, usado para gestionar dependencias y scripts.
- **Neutralinojs**: Framework ligero para crear aplicaciones de escritorio multiplataforma usando tecnologías web.

## Estructura del proyecto

- `/client`: Código fuente de la interfaz gráfica (Vue + Vite).
- `/neutralino`: Configuración y recursos para empaquetar y ejecutar la app como escritorio con Neutralinojs.
- `n_build.sh`: Script para construir el frontend y lanzar la aplicación de escritorio.

## Cómo ejecutar

1. Instala dependencias en `/client`:
   ```sh
   cd client
   bun install
   ```

2. Ejecuta el script de build y arranque:
   ```sh
   ./n_build.sh
   ```

Esto compilará la interfaz, copiará los archivos necesarios y abrirá la aplicación de escritorio.

---