"""Recursive-descent parser for Nemi (grammar in spec §10).

One method per grammar production; a single token of lookahead is enough. The
expression methods form the precedence tower of §11 (lowest to highest):

    or → and → not → comparison → sum → product → unary → postfix → primary

Block boundaries are found by explicit closers (``fin …``) plus the ``alt``
else keyword, so the parser never relies on indentation.

One deliberate extension beyond §10: array literals ``[a, b, c]`` are accepted
as a primary. This lets a host program or the test driver construct
array/matrix inputs; it never clashes with postfix indexing because that
requires a preceding primary.
"""

from . import ast_nodes as ast
from .errors import ParseError
from .tokens import TokenKind as T

_COMPARISONS = frozenset(
    {T.EQ, T.NE, T.LT, T.LE, T.GT, T.GE, T.IN, T.WITHIN, T.NOTIN, T.SUBSETEQ, T.SUBSET}
)
# T.WITHIN ("en") is a word-only synonym for T.IN (∈) here -- it has no
# ASCII-symbol equivalent otherwise, unlike !=/<=/>=. It stays a single
# lexer token reused in two grammar positions: this one (membership) and
# right after "cada" in _for_each (loop syntax) -- no ambiguity between the
# two because _for_each consumes it by direct position, never through this
# general comparison production.


class Parser:
    """Parses a token list into a :class:`nemi.ast_nodes.Program`."""

    def __init__(self, tokens):
        self._tokens = tokens
        self._i = 0

    # -- token cursor ---------------------------------------------------
    def _peek(self):
        return self._tokens[self._i]

    def _advance(self):
        tok = self._tokens[self._i]
        if tok.kind is not T.EOF:
            self._i += 1
        return tok

    def _check(self, kind):
        return self._peek().kind is kind

    def _match(self, kind):
        if self._check(kind):
            return self._advance()
        return None

    def _expect(self, kind, what):
        tok = self._peek()
        if tok.kind is not kind:
            raise ParseError(
                f"se esperaba {what}, se encontró {tok.kind.name}", tok.location
            )
        return self._advance()

    # -- program / definitions -----------------------------------------
    def parse_program(self):
        # programa ::= { definición | instrucción }
        # Definitions and top-level statements may interleave; we sort them into
        # `definitions` (hoisted) and `main` (run in source order).
        program = ast.Program()
        while not self._check(T.EOF):
            if self._check(T.FUNCTION) or self._check(T.PROCEDURE):
                program.definitions.append(self._definition())
            else:
                program.main.append(self._statement())
        return program

    def _definition(self):
        tok = self._peek()
        if tok.kind is T.FUNCTION:
            is_function = True
        elif tok.kind is T.PROCEDURE:
            is_function = False
        else:
            raise ParseError(
                "se esperaba 'función' o 'procedimiento'", tok.location
            )
        self._advance()
        name = self._expect(T.IDENT, "nombre de definición").value
        self._expect(T.LPAREN, "'('")
        params = self._params()
        self._expect(T.RPAREN, "')'")
        body = self._block()
        self._expect(T.END, "'fin'")
        # closing keyword must match the opener
        closer = T.FUNCTION if is_function else T.PROCEDURE
        self._expect(closer, "'función'/'procedimiento' después de 'fin'")
        return ast.Definition(name, params, body, is_function, tok.location)

    def _params(self):
        params = []
        if self._check(T.IDENT):
            params.append(self._advance().value)
            while self._match(T.COMMA):
                params.append(self._expect(T.IDENT, "nombre de parámetro").value)
        return params

    # -- blocks and statements -----------------------------------------
    def _at_block_end(self):
        # A block ends at a closer ('fin'), the else keyword ('alt'), or EOF.
        return self._peek().kind in (T.END, T.ELSE, T.EOF)

    def _block(self):
        statements = []
        while not self._at_block_end():
            statements.append(self._statement())
        return statements

    def _statement(self):
        kind = self._peek().kind
        if kind is T.FOR:
            # disambiguate 'para cada ... en ...' from 'para ident <- ... hasta ...'
            # (one token of extra lookahead; EOF always follows any real
            # token, so self._i + 1 is always in bounds here)
            if self._tokens[self._i + 1].kind is T.EACH:
                return self._for_each()
            return self._for()
        if kind is T.WHILE:
            return self._while()
        if kind is T.IF:
            return self._if()
        if kind is T.RETURN:
            return self._return()
        if kind is T.PROSE:
            tok = self._advance()
            return ast.Prose(tok.value, tok.location)
        if kind is T.ASSERT:
            return self._assert()
        if kind is T.TRACE:
            return self._trace()
        if kind is T.IDENT:
            return self._assign_or_call()
        tok = self._peek()
        raise ParseError(
            f"{tok.kind.name} inesperado al inicio de la instrucción", tok.location
        )

    def _assert(self):
        # aserción ::= "afirma" expresión [ "," cadena ]   (spec §21.2, v0.2)
        tok = self._advance()  # afirma
        condition = self._expression()
        message = None
        if self._match(T.COMMA):
            message = self._expect(T.STRING, "cadena de mensaje de la aserción").value
        return ast.Assert(condition, message, tok.location)

    def _trace(self):
        # rastreo ::= "traza" expresión   (spec §21.3, v0.2 [OPC])
        tok = self._advance()  # traza
        expression = self._expression()
        return ast.Trace(expression, tok.location)

    def _for(self):
        tok = self._advance()  # para
        var = self._expect(T.IDENT, "variable de ciclo").value
        self._expect(T.ASSIGN, "'←'")
        start = self._expression()
        self._expect(T.TO, "'hasta'")
        end = self._expression()
        self._match(T.REPEAT)  # optional 'repite'
        body = self._block()
        self._expect(T.END, "'fin'")
        self._expect(T.FOR, "'para' después de 'fin'")
        return ast.ForLoop(var, start, end, body, tok.location)

    def _for_each(self):
        # ciclo_cada ::= "para" "cada" ident "en" expresión [ "repite" ]
        #                bloque "fin" "para"   (spec §20.3, v0.2)
        tok = self._advance()  # para
        self._advance()  # cada
        var = self._expect(T.IDENT, "variable de ciclo").value
        self._expect(T.WITHIN, "'en'")
        collection = self._expression()
        self._match(T.REPEAT)  # optional 'repite'
        body = self._block()
        self._expect(T.END, "'fin'")
        self._expect(T.FOR, "'para' después de 'fin'")
        return ast.ForEach(var, collection, body, tok.location)

    def _while(self):
        tok = self._advance()  # mientras
        condition = self._expression()
        body = self._block()
        self._expect(T.END, "'fin'")
        self._expect(T.WHILE, "'mientras' después de 'fin'")
        return ast.WhileLoop(condition, body, tok.location)

    def _if(self):
        tok = self._advance()  # si
        condition = self._expression()
        self._match(T.THEN)  # optional 'entonces'
        then_body = self._block()
        else_body = None
        if self._match(T.ELSE):  # 'alt'
            else_body = self._block()
        self._expect(T.END, "'fin'")
        self._expect(T.IF, "'si' después de 'fin'")
        return ast.If(condition, then_body, else_body, tok.location)

    def _return(self):
        tok = self._advance()  # regresa
        value = None
        if not self._at_block_end():
            value = self._expression()
        return ast.Return(value, tok.location)

    def _assign_or_call(self):
        tok = self._peek()
        name = self._advance().value  # IDENT
        if self._check(T.LPAREN):
            call = self._finish_call(name, tok.location)
            return ast.ExprStatement(call, tok.location)
        # otherwise a place (assignment target): ident { '[' expr ']' }
        indices = []
        while self._match(T.LBRACKET):
            indices.append(self._expression())
            self._expect(T.RBRACKET, "']'")
        self._expect(T.ASSIGN, "'←' (asignación) o '(' (llamada)")
        value = self._expression()
        return ast.Assign(name, indices, value, tok.location)

    # -- expressions (precedence tower) --------------------------------
    def _expression(self):
        return self._logic_or()

    def _logic_or(self):
        left = self._logic_and()
        while self._check(T.OR):
            op = self._advance()
            right = self._logic_and()
            left = ast.Binary(T.OR, left, right, op.location)
        return left

    def _logic_and(self):
        left = self._logic_not()
        while self._check(T.AND):
            op = self._advance()
            right = self._logic_not()
            left = ast.Binary(T.AND, left, right, op.location)
        return left

    def _logic_not(self):
        # expr_no ::= [ "¬" | "no" ] expr_comp
        if self._check(T.NOT):
            op = self._advance()
            operand = self._comparison()
            return ast.Unary(T.NOT, operand, op.location)
        return self._comparison()

    def _comparison(self):
        left = self._sum()
        if self._peek().kind in _COMPARISONS:
            op = self._advance()
            right = self._sum()
            return ast.Binary(op.kind, left, right, op.location)
        return left

    def _sum(self):
        left = self._product()
        while self._peek().kind in (T.PLUS, T.MINUS):
            op = self._advance()
            right = self._product()
            left = ast.Binary(op.kind, left, right, op.location)
        return left

    def _product(self):
        left = self._unary()
        while self._peek().kind in (T.TIMES, T.DIVIDE, T.MOD):
            op = self._advance()
            right = self._unary()
            left = ast.Binary(op.kind, left, right, op.location)
        return left

    def _unary(self):
        # expr_unaria ::= [ "−" | "¬" ] expr_postfija
        if self._peek().kind in (T.MINUS, T.NOT):
            op = self._advance()
            operand = self._postfix()
            return ast.Unary(op.kind, operand, op.location)
        return self._postfix()

    def _postfix(self):
        expr = self._primary()
        while self._match(T.LBRACKET):
            index = self._expression()
            self._expect(T.RBRACKET, "']'")
            expr = ast.Index(expr, index, expr.location)
        return expr

    def _primary(self):
        tok = self._peek()
        kind = tok.kind

        if kind is T.INT:
            self._advance()
            return ast.IntLiteral(tok.value, tok.location)
        if kind is T.REAL:
            self._advance()
            return ast.RealLiteral(tok.value, tok.location)
        if kind is T.STRING:
            self._advance()
            return ast.StringLiteral(tok.value, tok.location)
        if kind is T.LBRACKET:
            return self._array_literal()
        if kind is T.LBRACE:
            return self._set_literal()
        if kind is T.EMPTYSET:
            self._advance()
            return ast.SetLiteral([], tok.location)
        if kind is T.IDENT:
            self._advance()
            if self._check(T.LPAREN):
                return self._finish_call(tok.value, tok.location)
            return ast.Variable(tok.value, tok.location)
        if kind is T.LPAREN:
            self._advance()
            expr = self._expression()
            self._expect(T.RPAREN, "')'")
            return expr
        if kind is T.LFLOOR:
            self._advance()
            expr = self._expression()
            self._expect(T.RFLOOR, "'⌋'")
            return ast.Unary(T.LFLOOR, expr, tok.location)
        if kind is T.LCEIL:
            self._advance()
            expr = self._expression()
            self._expect(T.RCEIL, "'⌉'")
            return ast.Unary(T.LCEIL, expr, tok.location)
        if kind is T.SQRT:
            self._advance()
            operand = self._unary()  # √ binds like a unary prefix (√ expr_unaria)
            return ast.Unary(T.SQRT, operand, tok.location)

        raise ParseError(
            f"{kind.name} inesperado en la expresión", tok.location
        )

    def _array_literal(self):
        tok = self._expect(T.LBRACKET, "'['")
        elements = []
        if not self._check(T.RBRACKET):
            elements.append(self._expression())
            while self._match(T.COMMA):
                elements.append(self._expression())
        self._expect(T.RBRACKET, "']'")
        return ast.ArrayLiteral(elements, tok.location)

    def _set_literal(self):
        # conjunto_lit ::= "{" [ args ] "}" | "∅"  (spec §20.1, v0.2)
        tok = self._expect(T.LBRACE, "'{'")
        elements = []
        if not self._check(T.RBRACE):
            elements.append(self._expression())
            while self._match(T.COMMA):
                elements.append(self._expression())
        self._expect(T.RBRACE, "'}'")
        return ast.SetLiteral(elements, tok.location)

    def _finish_call(self, name, location):
        self._expect(T.LPAREN, "'('")
        args = []
        if not self._check(T.RPAREN):
            args.append(self._expression())
            while self._match(T.COMMA):
                args.append(self._expression())
        self._expect(T.RPAREN, "')'")
        return ast.Call(name, args, location)


def parse(tokens):
    """Convenience wrapper: ``parse(tokens) -> Program``."""
    return Parser(tokens).parse_program()
