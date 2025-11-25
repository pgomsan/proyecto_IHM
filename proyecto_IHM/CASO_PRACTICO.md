# Caso práctico: pizarra de navegación (Curso 2025-2026)

## 1. Caso de estudio

Herramienta para preparar ejercicios de navegación sobre la carta
náutica del estrecho de Gibraltar en exámenes de patrón de embarcación
de recreo. El profesorado puede proyectar resoluciones y el alumnado
puede practicar con un conjunto de problemas tipo test, almacenando su
evolución. El examen real se hace en papel; la aplicación debe simular
el proceso con la mayor fidelidad posible.

Los exámenes son tipo test (enunciado + 4 respuestas, solo una correcta)
y el alumnado resuelve sobre la carta:

- Marcar puntos y trazar líneas.
- Dibujar circunferencias o arcos.
- Tomar distancias entre dos puntos.
- Medir ángulos.
- Escribir cálculos o anotaciones.

La aplicación debe cubrir dos grupos de funciones: (1) gestión de
alumnos y problemas (registro, login, histórico) y (2) edición sobre la
carta.

## 2. Escenarios generales

### 2.1. Registrarse en la aplicación

Juan descarga la herramienta, accede a “registrarse” e introduce:
nickname “jgarcia”, contraseña “passPER21!”, email “jgarcia@gmail.com”,
fecha de nacimiento y avatar opcional (puede usar el por defecto). El
sistema valida y rechaza entradas que incumplen restricciones; tras
corregir, completa el registro.

Validaciones:

- Nickname único, 6-15 caracteres o dígitos, sin espacios; permite
  guion o subguion.
- Password de 8-20 caracteres con mayúsculas, minúsculas, dígitos y
  carácter especial `!@#$%&*()-+=`.
- Email con formato válido.
- Usuario debe tener más de 16 años.

### 2.2. Autentificarse

Juan elige “autenticarse”, introduce nickname y contraseña. El sistema
comprueba y concede acceso.

### 2.3. Cerrar sesión

Tras 4 ejercicios, Juan cierra sesión para que Susana entre. La sesión
almacena aciertos y fallos.

### 2.4. Realizar un problema

Juan pide un problema; puede ser aleatorio o desde la lista completa.
El sistema muestra enunciado y respuestas barajadas en cada intento.
Juan responde y el sistema verifica.

### 2.5. Modificar perfil

Juan cambia avatar (u otros datos salvo nickname). El sistema valida y
actualiza.

### 2.6. Mostrar resultados

Juan consulta aciertos y errores; puede filtrar por fecha (ej. últimos
días).

## 3. Escenarios de edición sobre la carta

La aplicación debe permitir elegir color/grosor, equivalente a usar
lápiz/bolígrafo.

### 3.1. Marcar un punto

Selecciona herramienta de puntos y marca la posición; se muestra con
forma/color elegido.

### 3.2. Trazar una línea

Dibuja línea entre dos puntos (ej. Tarifa-Tánger) con color elegido.

### 3.3. Trazar un arco

Usa compás digital: selecciona centro y traza arco con grosor/color
preestablecido.

### 3.4. Anotar texto

Selecciona herramienta de texto, elige posición y escribe; se muestra
con color/tamaño preestablecido.

### 3.5. Cambiar el color de una marca

Selecciona marca (punto/línea/arco/texto) y nuevo color; la marca se
actualiza.

### 3.6. Eliminar una marca

Usa “goma” para borrar la marca seleccionada.

### 3.7. Limpiar la carta

Elimina todas las marcas y presenta una carta nueva.

### 3.8. Desplazar el transportador para medir ángulos

Mueve el transportador hasta centrarlo en la línea o punto; lee el
ángulo. Para trazar con ángulo, coloca el centro y marca un punto en el
ángulo deseado. (Escenario compuesto que combina dibujar línea, mover
transportador y anotar texto).

### 3.9. Tomar una distancia en la carta

Usa regla o compás: mide entre dos puntos y lleva la medida a la escala
vertical de la carta para calcular subdivisiones equivalentes.

### 3.10. Marcar extremos de un punto en la carta

Solicita mostrar proyección del punto sobre escalas horizontales y
verticales (lat/long); luego puede ocultarlas.

### 3.11. Realizar zoom

Zoom in/out sobre la carta para leer detalles o alejar la vista.

## 4. Recursos para realizar la práctica

- `carta_nautica.jpg` como base de ejercicios.
- CSS con estilos para mostrar transportador y regla mediante path SVG
  embebido.
- Proyecto PoiUPV como base para zoom sobre imagen.

## 5. Modelo de datos

- **User**: nickname (no modificable), email, password, avatar,
  birthdate, sesiones.
- **Session**: timestamp, hits, faults (inmutable tras crear).
- **Problem**: texto del enunciado, lista de 4 respuestas.
- **Answer**: texto de la respuesta, validez (booleana).

## 6. Librería proporcionada para la persistencia de los datos

Pendiente de realizar y publicar.

## 7. Ayudas a la programación

Pendiente de publicar.

## 8. Instrucciones de entrega

- Fecha de entrega: 16 de diciembre (única para todos los grupos).
- Un miembro sube el ZIP a la tarea con nombres del grupo en comentarios
  y URI del repositorio con permisos de acceso.

## 9. Evaluación

- Debe reflejarse el proceso de desarrollo: documentación de diseño
  conceptual, diseño físico, prototipos.
- Proyectos que no compilen o no muestren pantalla principal arrancan
  con nota 0.
- Incluir diálogos de confirmación y error necesarios.
- Usar hojas de estilo para formato.
- Se evaluarán principios y guías de diseño de interfaz vistos en teoría.
- Ventana principal redimensionable; controles se ajustan al espacio.
- Se aplica normativa de integridad y honestidad académica de la UPV y
  ETSInf (con herramientas anti plagio).
