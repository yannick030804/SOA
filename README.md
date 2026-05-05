# SOA

Proyecto en C para inspeccionar imagenes de sistemas de ficheros `EXT2` y `FAT16`.

El programa detecta automaticamente el tipo de sistema de ficheros a partir del fichero y ejecuta la operacion pedida:
- `--info`: muestra informacion general del sistema de ficheros.
- `--tree`: muestra el arbol de directorios.
- `--cat`: muestra el contenido de un fichero dentro de una imagen `FAT16`.

## Que hace el proyecto

Actualmente el proyecto soporta:
- `EXT2 --info`
- `EXT2 --tree`
- `FAT16 --info`
- `FAT16 --tree`
- `FAT16 --cat`

Estado actual:
- `EXT2 --cat` no esta implementado.

## Estructura

- [src/fsutils.c](/Users/yannicksuchy/La%20Salle%202025%20-%202026/SOA/Practica/SOA/src/fsutils.c): punto de entrada, validacion de parametros, apertura de imagenes y deteccion del sistema de ficheros.
- [src/ext2.c](/Users/yannicksuchy/La%20Salle%202025%20-%202026/SOA/Practica/SOA/src/ext2.c): lectura y visualizacion de informacion de `EXT2`.
- [src/fat16.c](/Users/yannicksuchy/La%20Salle%202025%20-%202026/SOA/Practica/SOA/src/fat16.c): lectura y visualizacion de informacion de `FAT16` y recorrido recursivo del arbol de directorios.
- [data/ext2](/Users/yannicksuchy/La%20Salle%202025%20-%202026/SOA/Practica/SOA/data/ext2): imagenes de ejemplo `EXT2`.
- [data/fat16](/Users/yannicksuchy/La%20Salle%202025%20-%202026/SOA/Practica/SOA/data/fat16): imagenes de ejemplo `FAT16`.

## Compilacion

La forma mas simple es compilar desde la raiz del proyecto con:

```bash
make
```

Esto genera el ejecutable `fsutils`.

Si quieres compilar manualmente, usa:

```bash
cc -Wall -Wextra src/fsutils.c src/ext2.c src/fat16.c -o fsutils
```

## Ejecucion

La forma recomendada para ejecutar el programa es desde la raiz:

```bash
./fsutils --info <imagen>
./fsutils --tree <imagen>
./fsutils --cat <imagen> <fichero>
```

Tambien puede ejecutarse desde `src` si el binario esta ahi.

El programa busca las imagenes automaticamente en:
- `data/ext2`
- `data/fat16`
- `../data/ext2`
- `../data/fat16`

Por eso funciona tanto desde la raiz como desde `src`.

Uso general:

```bash
./fsutils --info <fichero>
./fsutils --tree <fichero>
./fsutils --cat <fichero> <archivo>
```

## Ejemplos

Mostrar informacion de una imagen `EXT2`:

```bash
cd src
./fsutils --info studentext100MB
```

Mostrar informacion de una imagen `FAT16`:

```bash
cd src
./fsutils --info studentfat100MB
```

Mostrar el arbol de directorios de una imagen `FAT16`:

```bash
cd src
./fsutils --tree studentfat100MB
```

Mostrar el contenido de un fichero dentro de una imagen `FAT16`:

```bash
cd src
./fsutils --cat studentfat100MB practica.c
```

## Como funciona

### `--info`

Para `EXT2`, el programa:
- lee el superbloque a partir del offset `1024`
- extrae campos como numero de inodos, bloques, bloque inicial, nombre del volumen y fechas
- formatea las fechas y muestra la informacion por pantalla

Para `FAT16`, el programa:
- lee el boot sector
- extrae tamano de sector, sectores por cluster, numero de FATs, entradas del root y etiqueta
- muestra la informacion por pantalla

### `--tree` en `FAT16`

La opcion `--tree` en `FAT16`:
- lee los datos base del boot sector
- calcula donde empiezan la FAT, el directorio raiz y la zona de datos
- construye un arbol en memoria usando nodos enlazados
- recorre directorios de forma recursiva
- sigue la cadena de clusters mediante la FAT
- imprime el arbol con formato visual tipo `tree`

Durante el recorrido:
- se ignoran entradas borradas
- se ignoran nombres largos (`LFN`)
- se ignoran `.` y `..`
- se enlazan hijos y hermanos para representar la jerarquia de carpetas y ficheros

### `--cat` en `FAT16`

La opcion `--cat` en `FAT16`:
- busca el fichero dentro del sistema de ficheros
- localiza su primer cluster y su tamano
- sigue la cadena de clusters en la FAT
- imprime el contenido del fichero por pantalla

## Limitaciones actuales

- `EXT2 --cat` no esta implementado.
- El arbol `FAT16` muestra nombres cortos en formato 8.3.
- El programa asume imagenes validas y no implementa manejo exhaustivo de errores de lectura.

## Salida esperada

Ejemplo de salida para `--tree` en una imagen `FAT16`:

```text
.
├── ASO
│   ├── SHOPPI~1.TXT
│   └── PIPES.SH
├── DONUT
│   ├── DONUT.C
│   └── DONETE~1
│       └── DONETE~1.C
└── SO
    └── PRACTICA.C
```
