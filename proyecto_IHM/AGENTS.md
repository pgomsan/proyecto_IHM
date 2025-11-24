# AGENTS.md

## Visión general del proyecto

Este proyecto implementa una herramienta digital que simula la
experiencia real de resolver ejercicios de navegación sobre la carta
náutica del Estrecho de Gibraltar. La aplicación permite registrar
usuarios, gestionar sesiones, proponer problemas tipo test y editar la
carta mediante herramientas como puntos, líneas, arcos, regla, compás,
transportador y zoom. El objetivo es reproducir el entorno del examen de
forma fiel dentro de una interfaz moderna construida con Qt.

------------------------------------------------------------------------

## Agentes del sistema

### **UserAgent**

Gestiona toda la lógica relacionada con los usuarios. - Registro con
validación de nickname, email, contraseña, edad y avatar. -
Autenticación. - Modificación del perfil (excepto nickname). - Cierre de
sesión y creación de objetos Session.

### **ProblemAgent**

Controla el acceso a los problemas de navegación. - Proponer un problema
aleatorio o mostrar listado completo. - Mostrar enunciado y respuestas
mezcladas. - Validar la respuesta seleccionada. - Reportar
aciertos/fallos al SessionAgent.

### **SessionAgent**

Registra la actividad del usuario. - Crear sesiones al cerrar sesión. -
Guardar aciertos y fallos. - Mostrar y filtrar el historial por fechas.

### **MapEditorAgent**

Gestiona las herramientas aplicadas sobre la carta. - Marcar puntos. -
Dibujar líneas y arcos. - Añadir texto. - Cambiar color o grosor. -
Eliminar marcas. - Limpiar la carta. - Mover transportador y medir
ángulos. - Medir distancias con regla o compás. - Mostrar/ocultar
extremos de latitud/longitud. - Zoom in/out.

### **ToolAgent**

Administra la herramienta activa. - Activar/desactivar herramientas. -
Mantener color/grosor seleccionados. - Coordinarse con MapEditorAgent.

### **PersistenceAgent**

Encargado de la persistencia utilizando la librería proporcionada. -
Guardar/cargar usuarios. - Guardar/cargar sesiones. - Acceso a Problem y
Answer.

### **UIAgent**

Coordina la interfaz Qt. - Gestión de ventanas, diálogos y mensajes. -
Aplicación de estilos. - Adaptabilidad al redimensionado. - Comunicación
entre vista y lógica de agentes.

------------------------------------------------------------------------

# Repository Guidelines

## Project Structure & Module Organization

-   `main.cpp` arranca la aplicación Qt y carga `MainWindow`.
-   `mainwindow.h/.cpp` contienen la lógica principal; `mainwindow.ui`
    define la interfaz visual.
-   `CMakeLists.txt` configura Qt6 (Core, Widgets) y los pasos de
    deploy.
-   `build/` es un directorio generado y no debe comitearse. Recursos
    nuevos deben ir en carpetas dedicadas e incluirse en CMake.

## Build, Test, and Development Commands

-   Configurar: `cmake -S . -B build`.
-   Compilar: `cmake --build build`.
-   Instalar/deploy: `cmake --install build`.
-   Ejecutar desde build: macOS
    `./build/proyecto_IHM.app/Contents/MacOS/proyecto_IHM`;
    Linux/Windows `./build/proyecto_IHM`.
-   Tests (futuros): `ctest --test-dir build`.

## Coding Style & Naming Conventions

-   C++17 + convenciones Qt.
-   Identación 4 espacios, llaves en línea nueva.
-   Clases PascalCase; variables camelCase.
-   Slots/signals con nombres tipo Qt.
-   Lógica en `.cpp`, no en `.ui`.
-   Uso de `unique_ptr`/`shared_ptr`, `QString`, `QVector`, y parámetros
    `const &` cuando sea posible.

## Testing Guidelines

-   Tests con Qt Test (`<QtTest>`), en `tests/`.
-   Registrar test con `qt_add_executable` y `qt_add_test`.
-   Tests deterministas, sin temporizadores ni red.

## Commit & Pull Request Guidelines

-   Commits en imperativo: "Add window actions", "Fix layout".
-   Commits pequeños y enfocados, incluyendo cambios en `.ui` y CMake.
-   PRs con alcance, pasos de prueba y notas por plataforma.
-   El proyecto debe compilar antes del PR.

## Security & Configuration Tips

-   No comitear rutas del instalador de Qt, archivos temporales o
    scripts generados.
-   Mantener secretos fuera del código; usar variables de entorno si
    hace falta.
