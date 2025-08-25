# Smart - Interfaz Gráfica de Monitoreo

**Smart** es una aplicación de escritorio multiplataforma para monitoreo de sistema, desarrollada con tecnologías modernas del ecosistema JavaScript y web.

## Tecnologías principales

- **Vue 3**: Framework progresivo para construir interfaces de usuario reactivas.
- **Vite**: Herramienta de desarrollo y build ultrarrápida para proyectos frontend.
- **Bun**: Runtime moderno para JavaScript/TypeScript, utilizado para scripts y gestión de dependencias.
- **Neutralinojs**: Framework ligero para crear aplicaciones de escritorio usando tecnologías web, sin depender de Electron.

## Estructura del proyecto

- `/client`:  
  Código fuente de la interfaz gráfica (Vue + Vite).  
  Incluye componentes, vistas, assets y configuración de frontend.

- `/neutralino`:  
  Configuración y recursos para empaquetar y ejecutar la app como escritorio con Neutralinojs.  
  Incluye binarios para distintas plataformas, configuración, logs y recursos estáticos.

- `n_build.sh`:  
  Script automatizado para construir el frontend y lanzar la aplicación de escritorio.  
  Realiza el build con Bun, copia los archivos generados y ejecuta Neutralinojs.

## Instalación y ejecución

1. Instala las dependencias del frontend:
   ```sh
   cd client
   bun install
   ```

2. Desde la raíz del proyecto, ejecuta el script de build y arranque:
   ```sh
   ./n_build.sh
   ```

Esto compilará la interfaz gráfica, copiará los archivos generados a la carpeta de recursos de Neutralino y abrirá la aplicación de escritorio.

## Recursos y configuración

- La configuración principal de Neutralinojs se encuentra en `/neutralino/neutralino.config.json`.
- Los recursos estáticos y archivos generados por el build se ubican en `/neutralino/resources/`.

---