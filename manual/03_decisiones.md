# 3. Decisiones

## `si` / `alt` / `fin si`

Un programa que siempre hace exactamente lo mismo no es muy útil. `si` deja
que tu programa **decida** qué hacer según una condición:

```
edad ← 15
si edad ≥ 18
    imprime("Puedes votar")
alt
    imprime("Todavía no puedes votar")
fin si
```

```console
Todavía no puedes votar
```

Piezas:

- `si CONDICIÓN` — si la condición es cierta, ejecuta lo que sigue.
- `alt` (de "alternativamente") — si no, ejecuta esto en su lugar. Es
  **opcional**: si no la necesitas, no la pongas.
- `fin si` — **obligatorio siempre**, marca dónde termina el bloque. A
  diferencia de otros lenguajes que usas la sangría (los espacios al
  principio de la línea) para saber dónde termina un bloque, Nemi **no**
  se fija en la sangría — la sangría de este manual es solo para que un
  humano lo lea cómodo. Lo que de verdad marca el final es `fin si`.

Sin `alt`, si la condición es falsa Nemi simplemente no ejecuta nada de ese
bloque y sigue con lo que venga después:

```
n ← 7
si n mod 2 = 0
    imprime(n, "es par")
fin si
imprime("fin del programa")
```

```console
fin del programa
```

(no se imprimió nada sobre "es par" porque 7 es impar — la condición fue
falsa, así que ese bloque completo se saltó.)

## Comparar valores

Ya viste `=` en el capítulo 2. La familia completa:

| Símbolo | ASCII | Significa |
|---|---|---|
| `=` | `=` | Igual a |
| `≠` | `!=` | Distinto de |
| `<` | `<` | Menor que |
| `≤` | `<=` | Menor o igual que |
| `>` | `>` | Mayor que |
| `≥` | `>=` | Mayor o igual que |

Cualquier comparación te da **`verdadero`** o **`falso`** — nunca escribes
esas dos palabras directamente en tu código (no son literales), solo
aparecen como *resultado* de comparar algo:

```
imprime(5 = 5)      # verdadero
imprime(5 = 6)       # falso
```

## Combinar condiciones: `y` / `o` / `no`

```
n ← 14
si n mod 2 = 0 y n > 10
    imprime(n, "es par y mayor que 10")
fin si

si no(n mod 2 = 0)
    imprime("es impar")
alt
    imprime("es par")
fin si
```

```console
14 es par y mayor que 10
es par
```

- `A y B` — cierto solo si **ambas** son ciertas.
- `A o B` — cierto si **al menos una** es cierta.
- `no A` — invierte: cierto si `A` es falso, y viceversa.

También existen los símbolos `∧` (y), `∨` (o), `¬` (no) si prefieres
notación matemática — son exactamente lo mismo, Nemi los trata idéntico.

## Un detalle sutil: bit vs. booleano

Esto no lo vas a necesitar todos los días, pero te va a ahorrar confusión
más adelante (en particular con el capítulo 8 y con `booleana.nemi` de la
biblioteca común). Nemi tiene **dos** formas de representar "verdad":

1. Un **booleano** de verdad — lo que produce una comparación (`=`, `<`,
   …). Se imprime como `verdadero`/`falso`.
2. Un **bit** — el entero `0` o `1`, usado en teoría de circuitos y tablas
   de verdad. Se imprime como `0`/`1`.

`y`/`o`/`no` (`∧`/`∨`/`¬`) funcionan con **ambos**, y el resultado "hereda"
el tipo de lo que les des: si les das bits (`0`/`1`), el resultado es un
bit; si les das booleanos (de una comparación), el resultado es booleano.

```
imprime(1 y 0)              # 0  (bit y bit -> bit)
imprime((5 = 5) y (3 = 3))  # verdadero  (booleano y booleano -> booleano)
```

No memorices esta regla ahora — solo recuerda que existe, para cuando veas
`0`/`1` donde esperabas `verdadero`/`falso` (o viceversa) y quieras saber
por qué.

## Ejercicios

1. Escribe un programa que, dado un número guardado en una variable, imprima
   "positivo", "negativo" o "cero" según corresponda (vas a necesitar `si`
   con más de dos casos — pista: puedes anidar un `si` dentro del `alt` de
   otro).
2. Escribe la condición "n es múltiplo de 3 **o** de 5" usando `mod` y `o`.
   Pruébala con `n = 9`, `n = 10`, `n = 7`.
3. ¿Qué imprime `imprime(no (5 > 3))`? Antes de correrlo, resuélvelo a mano.
4. Reescribe el ejemplo de "par y mayor que 10" usando `∧` en vez de `y`, y
   confirma que da exactamente lo mismo.

Siguiente: [**Capítulo 4 — Repetición**](04_repeticion.md)
