### Resumen Rápido de la Sintaxis de Zynk

#### 1. Variables y Asignación

* **Declaración:** Usas la palabra clave `let`:
```zynk
let x = 10
let msg = "Hola Zynk"

```


* **Reasignación:** Modificación directa sin `let`:
```zynk
x = 20

```


* **Eliminación de variables:** Usas `del`:
```zynk
del x

```



#### 2. Estructuras de Control de Flujo

* **Condicionales (`if ... then ... else ... fi`):**
Estilo shell/POSIX claro y explícito:
```zynk
if x >= 10 then
    echo "Es mayor o igual a 10"
else
    echo "Es menor"
fi

```


* **Bucle `for` (`for (init; cond; inc) do ... done`):**
```zynk
for (let i = 0; i < 5; i = i + 1) do
    echo i
done

```


* **Bucle `do ... while` / `do { ... } while cond`:**
En Zynk la estructura del `while` se abre con `do {` y la condición se evalúa al cerrar `} while`:
```zynk
do {
    echo "Sigo aqui"
} while restando > 0

```


* **Control de bucle:** `break` para salir.

#### 3. Funciones y Clases

* **Definición de Función:**
```zynk
fn mi_funcion {
    let respuesta = Net:recv($0, 1)
    return respuesta
}

```


*(Nota: los argumentos pasados a funciones o scripts se direccionan con `$0`, `$1`, etc.).*
* **Definición de Clase:**
```zynk
class MiEstructura {
    let estado = 1
}

```



#### 4. Tipos de Datos y Operadores

* **Cadenas de Texto:** Comillas dobles `"..."` con soporte nativo de escapes (`\n`, `\r`, `\\`) en el lexer o comillas simples `'...'`.
* **Comentarios:** Empiezan por `#` hasta el final de la línea.
* **Operadores Lógicos y Aritméticos:**
* Matematicas: `+`, `-`, `*`, `/`
* Comparación: `==`, `!=`, `<`, `>`, `<=`, `>=`
* Lógicos: `and`, `or`, `!`
* Comandos/Pipes: `|`, `||`



---



