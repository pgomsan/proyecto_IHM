# AGENTS.md

## Visión general del proyecto

Herramienta para practicar ejercicios de navegación sobre la carta
náutica del Estrecho de Gibraltar, replicando el material del examen
(papel, lápiz, goma, regla, compás y transportador). La aplicación se
centra en dos bloques: gestión de usuarios/sesiones/problemas tipo test
e interacción sobre la carta con marcas, medidas y anotaciones. Debe
simular el proceso real sin penalizar al alumno frente al examen en
papel.

### Escenarios principales

- Gestión general: registro con validaciones, autenticación, cierre de
  sesión, propuesta de problema (aleatoria o lista completa), respuesta
  y verificación, modificación de perfil salvo nickname, y consulta de
  resultados con filtro por fecha.
- Edición sobre la carta: marcar puntos, líneas y arcos; anotar texto;
  cambiar color/grosor; borrar marcas; limpiar la carta; desplazar
  transportador para medir o trazar con ángulo; medir distancias con
  regla/compás; mostrar/ocultar proyección a escalas de lat/long; zoom
  in/out.

------------------------------------------------------------------------

## Agentes del sistema

### **UserAgent**

Gestiona toda la lógica relacionada con los usuarios. - Registro con
validación de nickname único (6-15 chars sin espacios, guion o
subguion), email válido, contraseña fuerte (8-20 chars, mayúsculas,
minúsculas, dígitos y símbolo !@#$%&*()-+=) y edad >16; admite avatar
opcional. - Autenticación por nickname + password. - Modificación del
perfil (email, password, avatar, birthdate; nunca nickname). - Cierre de
sesión y creación de objetos Session.

### **ProblemAgent**

Controla el acceso a los problemas de navegación. - Proponer un problema
aleatorio o mostrar listado completo. - Mostrar enunciado y respuestas
mezcladas en cada intento. - Validar la respuesta seleccionada y
registrar el resultado. - Reportar aciertos/fallos al SessionAgent.

### **SessionAgent**

Registra la actividad del usuario. - Crear sesiones al cerrar sesión. -
Guardar aciertos y fallos. - Mostrar y filtrar el historial por fechas
(ej. últimos días).

### **MapEditorAgent**

Gestiona las herramientas aplicadas sobre la carta. - Marcar puntos. -
Dibujar líneas y arcos. - Añadir texto. - Cambiar color o grosor. -
Eliminar marcas individualmente o limpiar la carta. - Mover transportador
para medir o trazar con ángulo. - Medir distancias con regla o compás. -
Mostrar/ocultar extremos de latitud/longitud de un punto. - Zoom in/out.

### **ToolAgent**

Administra la herramienta activa. - Activar/desactivar herramientas. -
Mantener color/grosor seleccionados. - Coordinarse con MapEditorAgent.

### **PersistenceAgent**

Encargado de la persistencia utilizando la librería proporcionada. -
Guardar/cargar usuarios. - Guardar/cargar sesiones. - Acceso a Problem y
Answer. - Integrar la librería de persistencia cuando esté publicada.

### **UIAgent**

Coordina la interfaz Qt. - Gestión de ventanas, diálogos y mensajes. -
Aplicación de estilos. - Adaptabilidad al redimensionado. - Comunicación
entre vista y lógica de agentes.

### Modelo de datos y recursos

- Modelo: User (nickname, email, password, avatar, birthdate, sesiones),
  Session (timestamp, hits, faults), Problem (texto, respuestas),
  Answer (texto, validez).
- Recursos: `resources/carta_nautica.jpg`, estilos CSS para mostrar
  transportador y regla mediante path SVG, y proyecto base PoiUPV para
  zoom sobre imagen. Librería de persistencia y ayudas de programación:
  pendientes de publicar.

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
