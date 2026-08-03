# 9. Cadenas de texto

Ya usaste cadenas desde el capítulo 1 (`"Hola, Nemi"`) para imprimir texto.
Ahora vas a **operarlas**: medirlas, tomar un carácter, unir dos cadenas, y
convertir entre texto y número.

## Longitud e indexación

Una cadena se comporta, para efectos de indexar, como un arreglo de
caracteres — base 1, igual que todo lo demás en Nemi:

```
s ← "algoritmo"
imprime(long(s))
imprime(s[1])
imprime(s[5])
```

```console
9
a
r
```

`s[1]` es el primer carácter (`"a"`), `long(s)` cuenta cuántos caracteres
tiene.

## Unir cadenas: `concatena`

No existe un operador `+` para cadenas en Nemi (`+` es solo para números y
bits, capítulo 2) — para unir texto se usa la función `concatena`:

```
s ← "algoritmo"
imprime(concatena(s, "s"))
imprime(concatena("Hola, ", "mundo"))
```

```console
algoritmos
Hola, mundo
```

## Convertir entre texto y número: `texto` / `valor`

- `texto(x)` — convierte un número a la cadena que lo representa.
- `valor(c)` — convierte un carácter dígito (`"0"`…`"9"`) al entero que
  representa.

```
imprime(texto(42))
imprime(concatena("El resultado es ", texto(7 · 6)))
imprime(valor("5"))
imprime(valor("5") + valor("3"))
```

```console
42
El resultado es 42
5
8
```

La segunda línea es el patrón más común: quieres construir un mensaje que
mezcle texto fijo con un número calculado — como `concatena` solo acepta
cadenas, conviertes el número con `texto(...)` primero.

## Recorrer una cadena con `para`

Como una cadena se indexa igual que un arreglo, recorrerla es el mismo
patrón del capítulo 6:

```
función cuenta_vocales(s)
    total ← 0
    para i ← 1 hasta long(s)
        c ← s[i]
        si c = "a" o c = "e" o c = "i" o c = "o" o c = "u"
            total ← total + 1
        fin si
    fin para
    regresa total
fin función

imprime(cuenta_vocales("algoritmo"))
```

```console
4
```

## Ejercicios

1. Escribe `función es_palindromo(s)` que regrese `1` si `s` se lee igual
   al derecho y al revés (pista: compara `s[i]` con `s[long(s) - i + 1]`
   para cada `i`, o revisa `invierte` en la
   [biblioteca común](10_biblioteca_comun.md) si prefieres no
   reescribirlo). Pruébala con `"reconocer"` y con `"algoritmo"`.
2. Escribe un programa que convierta un número de dos dígitos (por ejemplo
   `47`) a texto y confirme, usando `valor` sobre cada carácter del
   resultado, que puedes reconstruir el número original.
3. Usa `concatena` dentro de un `para` para construir la cadena `"12345"`
   uniendo, uno por uno, `texto(1)` hasta `texto(5)`.
4. ¿Qué crees que da `valor("a")` (una letra, no un dígito)? Antes de
   correrlo: ¿tiene sentido que Nemi lo permita, dado lo que hace `valor`?

Siguiente: [**Capítulo 10 — La biblioteca común**](10_biblioteca_comun.md)
