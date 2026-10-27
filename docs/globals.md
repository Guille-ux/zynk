## 🕒 Funciones de Sistema y Utilidades

### `arg(n)`
* **Descripción:** Devuelve el argumento n de los que recibio el programa (note que el primero es el número de argumentos).


* **Uso típico:** Acceder a la información que se le proporciono al programa como argumento.


* **Retorno:** Una string (`ShellValue`) con el valor del argumento.
  
### `float(string)`
* **Descripción:** convierte una cadena de texto a un double.


* **Uso típico:** extraer números de un archivo de texto.


* **Retorno:** Un número decimal (`ShellValue` numérico) con el valor de la string.
  
### `str(number)`
* **Descripción:** devuelve un número representado en forma de cadena de texto.


* **Uso típico:** Convertir un valor númerico en string para poder usar un número como llave de un diccionario.


* **Retorno:** una string con el valor del número que se le paso como argumento.

### `clock()`

* **Descripción:** Devuelve el tiempo actual del sistema en segundos (con precisión de microsegundos mediante `gettimeofday`).


* **Uso típico:** Ideal para medir tiempos de ejecución y hacer **benchmarks** de rendimiento en Zynk.


* **Retorno:** Un número decimal (`ShellValue` numérico) con los segundos transcurridos.

### `system(cmd)`

* **Descripción:** Ejecuta un comando en la terminal del sistema operativo anfitrión.


* **Argumentos:**
* `cmd`: Un string con el comando a ejecutar.




* **Retorno:** Devuelve `1` si se ejecutó, o `0` si faltaban argumentos (`argc < 1`).



---

## 🔌 Carga de Código y Extensiones

### `include(file)`

* **Descripción:** Carga un plugin nativo dinámico de forma multiplataforma (`.so` en Linux o `.dll` en Windows) buscando la función de registro `register_lib`.


* **Argumentos:**
* `file`: Ruta del archivo de la librería compartida.




* **Retorno:** `1` si la carga y el registro tuvieron éxito, o `0` si hubo algún fallo.



### `import(file)`

* **Descripción:** Lee un archivo fuente de Zynk entero, lo tokeniza, lo analiza sintácticamente (`parse`), lo ejecuta en un contexto aislado nuevo y devuelve una instancia con su entorno. ¡Básicamente tu sistema de módulos!


* **Argumentos:**
* `file`: Ruta del script de Zynk a importar.




* **Retorno:** Un objeto/instancia (`newInstanceVal`) que encapsula el contexto de ejecución del archivo importado.



---

## 📁 E/S de Archivos y Consola

### `write(path content)`

* **Descripción:** Crea o sobrescribe un archivo de texto escribiendo contenido en él.


* **Argumentos:**
* `path`: Ruta del archivo de destino (String).


* `content`: Texto que se quiere escribir (String).




* **Retorno:** `1` si se escribió correctamente, o `0` si falló al abrir el archivo o los tipos no coinciden.



### `read(path)`

* **Descripción:** Lee por completo el contenido de un archivo de texto en disco.


* **Argumentos:**
* `path`: Ruta del archivo a leer (String).




* **Retorno:** Un string con todo el contenido del archivo, o un string vacío `""` si el archivo no existe o no se pudo leer.



### `input(prompt?)`

* **Descripción:** Muestra un mensaje opcional por pantalla y lee una línea introducida por el usuario desde la consola (`stdin`), eliminando el salto de línea final.


* **Argumentos:**
* `prompt` *(opcional)*: Mensaje de texto a mostrar antes de leer.




* **Retorno:** Un string con lo que tecleó el usuario (o `"NULL"` si falló la lectura).



---

## 📦 Estructuras de Datos y Secuencias

### `Array()`

* **Descripción:** Constructor de arrays dinámicos orientados a objetos. Crea un contexto interno y expone métodos vinculados.


* **Métodos del objeto Array devuelto:**
* `push(val)`: Añade un elemento al final del array dinámico.


* `pop()`: Saca y devuelve el último elemento del array.


* `len()`: Devuelve el tamaño actual de elementos almacenados.




* **Retorno:** Una instancia de objeto Zynk que actúa como un array dinámico.



### `len(str)`

* **Descripción:** Calcula la longitud de una cadena de texto.


* **Argumentos:**
* `str`: El string del que quieres saber el tamaño.




* **Retorno:** Número de caracteres del string (o `0` si no es un string válido).



### `index(container i)`

* **Descripción:** Permite acceder de forma indexada a un elemento dentro de una secuencia (strings o arrays).


* **Argumentos:**
* `container`: Un string o un array.


* `i`: Índice numérico de la posición.




* **Retorno:**
* Si es un string, devuelve un nuevo string con el carácter único en esa posición.


* Si es un array, devuelve el `ShellValue` almacenado en esa posición.






