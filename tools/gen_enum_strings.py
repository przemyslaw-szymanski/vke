#!/usr/bin/env python3
"""
Parse a C++ header file and generate:
  1. operator<< overloads for std::ostream
  2. std::formatter specializations for std::format
for all enum types found in the file.

Usage:
    python gen_enum_strings.py <header_file> [output_file]

If output_file is omitted, output is written to stdout.
"""

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# Data types
# ---------------------------------------------------------------------------

@dataclass
class EnumMember:
    name: str
    value_expr: Optional[str]  # raw expression string, or None if sequential


@dataclass
class EnumDef:
    name: str                    # e.g. "LEVEL"
    scope_path: list             # e.g. ["VKE", "RenderSystem", "FeatureLevels"]
    members: list                # list of EnumMember
    base_type: Optional[str]     # e.g. "uint8_t", or None


# ---------------------------------------------------------------------------
# Tokeniser
# ---------------------------------------------------------------------------

_TOKEN_RE = re.compile(
    r'[A-Za-z_][A-Za-z0-9_]*'   # identifiers / keywords
    r'|0[xX][0-9A-Fa-f]+'        # hex literals
    r'|[0-9]+'                    # decimal literals
    r'|[{}();,=:|<>*&]'          # punctuation we care about
    r'|[-+^~!]'                   # operators (for completeness)
)


def tokenize(content: str) -> list[str]:
    return _TOKEN_RE.findall(content)


# ---------------------------------------------------------------------------
# Pre-processing helpers
# ---------------------------------------------------------------------------

def strip_preprocessor(text: str) -> str:
    """Join backslash-continued lines, then remove preprocessor directives."""
    text = re.sub(r'\\\n', ' ', text)
    text = re.sub(r'^\s*#[^\n]*', '', text, flags=re.MULTILINE)
    return text


def strip_comments(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    text = re.sub(r'//[^\n]*', '', text)
    return text


# ---------------------------------------------------------------------------
# Parser
# ---------------------------------------------------------------------------

def parse_enums(filepath: str) -> list[EnumDef]:
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    content = strip_comments(content)
    content = strip_preprocessor(content)

    tokens = tokenize(content)
    enums: list[EnumDef] = []

    # scope_stack: list of (kind, name)
    # kind: 'namespace' | 'struct' | 'class' | 'union' | 'other'
    scope_stack: list[tuple[str, str]] = []

    i = 0
    n = len(tokens)

    while i < n:
        tok = tokens[i]

        # ---- namespace / struct / class / union ----------------------------
        if tok in ('namespace', 'struct', 'class', 'union'):
            kind = tok
            i += 1
            name = ''
            # Collect the name (first identifier after the keyword, if any)
            if i < n and re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', tokens[i]):
                name = tokens[i]
                i += 1
            # Advance until '{' or ';' (forward declarations stop at ';')
            while i < n and tokens[i] not in ('{', ';'):
                i += 1
            if i < n and tokens[i] == '{':
                scope_stack.append((kind, name))
                i += 1
            # If ';', it's a forward declaration – don't push scope, skip ';'
            elif i < n and tokens[i] == ';':
                i += 1

        # ---- untracked opening brace (method bodies, lambdas, etc.) -------
        elif tok == '{':
            scope_stack.append(('', ''))   # placeholder keeps '}' counts balanced
            i += 1

        # ---- enum ---------------------------------------------------------
        elif tok == 'enum':
            i += 1
            # Skip optional 'class' / 'struct'
            if i < n and tokens[i] in ('class', 'struct'):
                i += 1
            # Collect enum name
            enum_name = ''
            if i < n and re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', tokens[i]):
                enum_name = tokens[i]
                i += 1
            # Optional base type after ':'
            base_type: Optional[str] = None
            if i < n and tokens[i] == ':':
                i += 1
                base_parts: list[str] = []
                while i < n and tokens[i] not in ('{', ';'):
                    base_parts.append(tokens[i])
                    i += 1
                base_type = ' '.join(base_parts).strip()

            if i < n and tokens[i] == '{':
                i += 1
                members, i = _consume_enum_body(tokens, i)

                # Build scope path from current stack
                scope_path = [name for _, name in scope_stack if name]

                if enum_name:
                    enums.append(EnumDef(
                        name=enum_name,
                        scope_path=scope_path,
                        members=members,
                        base_type=base_type,
                    ))
            # If no '{' (forward declaration / typedef usage), skip ';'
            elif i < n and tokens[i] == ';':
                i += 1

        # ---- closing brace ------------------------------------------------
        elif tok == '}':
            if scope_stack:
                scope_stack.pop()
            i += 1

        # ---- everything else ----------------------------------------------
        else:
            i += 1

    return enums


def _consume_enum_body(tokens: list[str], start: int) -> tuple[list[EnumMember], int]:
    """Consume tokens for an enum body (after the opening '{').
    Returns (members, new_index) where new_index points past the closing '}'."""
    members: list[EnumMember] = []
    i = start
    n = len(tokens)
    depth = 1  # we are already inside the first '{'

    body_tokens: list[str] = []
    while i < n:
        tok = tokens[i]
        if tok == '{':
            depth += 1
            body_tokens.append(tok)
        elif tok == '}':
            depth -= 1
            if depth == 0:
                i += 1  # consume the closing '}'
                break
            body_tokens.append(tok)
        else:
            body_tokens.append(tok)
        i += 1

    members = _parse_enum_members(body_tokens)
    return members, i


def _parse_enum_members(tokens: list[str]) -> list[EnumMember]:
    """Parse the flat token list inside an enum body into EnumMember objects."""
    members: list[EnumMember] = []
    i = 0
    n = len(tokens)

    while i < n:
        tok = tokens[i]
        if not re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', tok):
            i += 1
            continue

        name = tok
        i += 1
        value_expr: Optional[str] = None

        if i < n and tokens[i] == '=':
            i += 1  # skip '='
            expr_parts: list[str] = []
            depth = 0
            while i < n:
                t = tokens[i]
                if t in ('(', '{'):
                    depth += 1
                elif t in (')', '}'):
                    if depth == 0:
                        break
                    depth -= 1
                elif t == ',' and depth == 0:
                    break
                expr_parts.append(t)
                i += 1
            value_expr = ' '.join(expr_parts).strip()

        # Skip ',' separator if present
        if i < n and tokens[i] == ',':
            i += 1

        members.append(EnumMember(name=name, value_expr=value_expr))

    return members


# ---------------------------------------------------------------------------
# Code generation helpers
# ---------------------------------------------------------------------------

def fully_qualified(enum: EnumDef) -> str:
    return '::'.join(enum.scope_path + [enum.name])


def _compute_member_values(members: list[EnumMember]) -> dict[str, int]:
    """Best-effort mapping of member name -> integer value."""
    result: dict[str, int] = {}
    counter = 0
    for m in members:
        if m.value_expr is None:
            result[m.name] = counter
            counter += 1
        else:
            val = _eval_expr(m.value_expr.strip(), result)
            result[m.name] = val
            counter = val + 1
    return result


def _eval_expr(expr: str, env: dict[str, int]) -> int:
    """Evaluate a simple C++ enum value expression using known member values."""
    # VKE_BIT(n) -> 1 << n
    bit_m = re.match(r'^VKE_BIT\s*\(\s*(\d+)\s*\)$', expr)
    if bit_m:
        return 1 << int(bit_m.group(1))
    # Try plain integer literal
    try:
        return int(expr, 0)
    except ValueError:
        pass
    # Tokenise and evaluate expression with | + - & ^ operators and known names
    tokens = re.findall(r'VKE_BIT\s*\(\s*\d+\s*\)|[A-Za-z_][A-Za-z0-9_]*|0[xX][0-9A-Fa-f]+|\d+|[|+\-&^~()]', expr)
    # Convert tokens to a safe evaluable form
    parts = []
    for tok in tokens:
        bit_m2 = re.match(r'^VKE_BIT\s*\(\s*(\d+)\s*\)$', tok)
        if bit_m2:
            parts.append(str(1 << int(bit_m2.group(1))))
        elif tok in env:
            parts.append(str(env[tok]))
        else:
            parts.append(tok)
    try:
        return int(eval(' '.join(parts)))  # safe: only ints and bitwise ops
    except Exception:
        return 0


def _is_alias_of_member(member: EnumMember, all_member_names: set[str]) -> bool:
    """Return True when the member's value is just another member's name (alias)."""
    if member.value_expr is None:
        return False
    expr = member.value_expr.strip()
    return bool(re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', expr)) and expr in all_member_names


def valid_switch_members(members: list[EnumMember]) -> list[EnumMember]:
    """Filter members that should appear in switch/case statements."""
    all_names = {m.name for m in members}
    values = _compute_member_values(members)
    seen_values: set[int] = set()
    result = []
    for m in members:
        if m.name.startswith('_'):
            continue  # sentinel like _MAX_COUNT
        if _is_alias_of_member(m, all_names):
            continue  # alias (e.g. UNKNOWN = _MAX_COUNT, DEFAULT = FIRST)
        val = values.get(m.name)
        if val is not None:
            if val in seen_values:
                continue  # duplicate value – keep only first occurrence
            seen_values.add(val)
        result.append(m)
    return result


def generate_ostream_operator(enum: EnumDef) -> str:
    fq = fully_qualified(enum)
    vm = valid_switch_members(enum.members)
    if not vm:
        return ''

    lines = [
        f'inline std::ostream& operator<<(std::ostream& os, {fq} val)',
        '{',
        '    switch (val)',
        '    {',
    ]
    for m in vm:
        lines.append(f'        case {fq}::{m.name}: os << "{m.name}"; break;')
    lines += [
        '        default: os << "UNKNOWN(" << static_cast<int>(val) << ")"; break;',
        '    }',
        '    return os;',
        '}',
    ]
    return '\n'.join(lines)


def generate_formatter(enum: EnumDef) -> str:
    fq = fully_qualified(enum)
    vm = valid_switch_members(enum.members)
    if not vm:
        return ''

    lines = [
        'template <>',
        f'struct std::formatter<{fq}> : std::formatter<std::string_view>',
        '{',
        f'    auto format({fq} val, std::format_context& ctx) const',
        '    {',
        '        std::string_view name;',
        '        switch (val)',
        '        {',
    ]
    for m in vm:
        lines.append(f'            case {fq}::{m.name}: name = "{m.name}"; break;')
    lines += [
        '            default: name = "UNKNOWN"; break;',
        '        }',
        '        return std::formatter<std::string_view>::format(name, ctx);',
        '    }',
        '};',
    ]
    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# Bitmask detection + generators
# ---------------------------------------------------------------------------

def _is_zero_value(expr: str) -> bool:
    try:
        return int(expr.strip(), 0) == 0
    except ValueError:
        return False


def _is_power_of_two_value(expr: str) -> bool:
    expr = expr.strip()
    if re.match(r'^VKE_BIT\s*\(', expr):
        return True
    try:
        val = int(expr, 0)
        return val > 0 and (val & (val - 1)) == 0
    except ValueError:
        return False


def is_bitmask_enum(members: list[EnumMember]) -> bool:
    """Return True if enough members have power-of-2 or VKE_BIT values."""
    explicit = [m for m in members
                if m.value_expr is not None and not m.name.startswith('_')]
    if not explicit:
        return False
    bit_count = sum(1 for m in explicit
                    if _is_power_of_two_value(m.value_expr) or _is_zero_value(m.value_expr))
    return bit_count > 0 and bit_count >= (len(explicit) + 1) // 2


def _bitmask_members(members: list[EnumMember]) -> tuple[list, list]:
    """Split valid members into (zero_members, bit_members)."""
    vm = valid_switch_members(members)
    zero_members = [m for m in vm if m.value_expr is not None and _is_zero_value(m.value_expr)]
    bit_members  = [m for m in vm if m.value_expr is not None and _is_power_of_two_value(m.value_expr)]
    return zero_members, bit_members


def generate_ostream_operator_bitmask(enum: EnumDef) -> str:
    fq = fully_qualified(enum)
    zero_members, bit_members = _bitmask_members(enum.members)
    if not bit_members and not zero_members:
        return ''

    zero_name = zero_members[0].name if zero_members else '0'

    lines = [
        f'inline std::ostream& operator<<(std::ostream& os, {fq} val)',
        '{',
        f'    using U = std::underlying_type_t<{fq}>;',
        '    if (static_cast<U>(val) == static_cast<U>(0))',
        '    {',
        f'        os << "{zero_name}";',
        '        return os;',
        '    }',
        '    bool first = true;',
        f'    auto check = [&]({fq} bit, const char* name)',
        '    {',
        '        if ((static_cast<U>(val) & static_cast<U>(bit)) == static_cast<U>(bit))',
        '        {',
        '            if (!first) os << "|";',
        '            os << name;',
        '            first = false;',
        '        }',
        '    };',
    ]
    for m in bit_members:
        lines.append(f'    check({fq}::{m.name}, "{m.name}");')
    lines += [
        '    if (first) os << static_cast<U>(val);',
        '    return os;',
        '}',
    ]
    return '\n'.join(lines)


def generate_formatter_bitmask(enum: EnumDef) -> str:
    fq = fully_qualified(enum)
    zero_members, bit_members = _bitmask_members(enum.members)
    if not bit_members and not zero_members:
        return ''

    zero_name = zero_members[0].name if zero_members else '0'

    lines = [
        'template <>',
        f'struct std::formatter<{fq}> : std::formatter<std::string>',
        '{',
        f'    auto format({fq} val, std::format_context& ctx) const',
        '    {',
        f'        using U = std::underlying_type_t<{fq}>;',
        '        if (static_cast<U>(val) == static_cast<U>(0))',
        f'            return std::formatter<std::string>::format("{zero_name}", ctx);',
        '        std::string result;',
        f'        auto check = [&]({fq} bit, const char* name)',
        '        {',
        '            if ((static_cast<U>(val) & static_cast<U>(bit)) == static_cast<U>(bit))',
        '            {',
        "                if (!result.empty()) result += '|';",
        '                result += name;',
        '            }',
        '        };',
    ]
    for m in bit_members:
        lines.append(f'        check({fq}::{m.name}, "{m.name}");')
    lines += [
        '        if (result.empty()) result = std::to_string(static_cast<U>(val));',
        '        return std::formatter<std::string>::format(result, ctx);',
        '    }',
        '};',
    ]
    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description='Generate C++ enum-to-string conversions from a header file.',
    )
    parser.add_argument(
        'header',
        type=Path,
        help='Path to the C++ header file to parse.',
    )
    parser.add_argument(
        'output',
        type=Path,
        nargs='?',
        default=None,
        help='Output file path. Defaults to stdout.',
    )
    args = parser.parse_args()

    filepath = str(args.header)
    outpath  = str(args.output) if args.output else None

    print(f'Parsing {filepath} ...', file=sys.stderr)
    enums = parse_enums(filepath)
    valid_enums = [e for e in enums if e.name and valid_switch_members(e.members)]
    print(f'Found {len(enums)} enum(s), {len(valid_enums)} with non-trivial members.', file=sys.stderr)

    # Use only the filename so the #include refers to the local directory
    include_path = Path(filepath).name

    out: list[str] = [
        '// Auto-generated by gen_enum_strings.py - do not edit manually.',
        '//',
        f'// Source: {include_path}',
        '',
        '#pragma once',
        '#ifdef OPTIONAL',
        '#pragma push_macro("OPTIONAL")',
        '#undef OPTIONAL',
        '#endif',
        '#include <format>',
        '#include <ostream>',
        '#include <string>',
        '#include <string_view>',
        '#include <type_traits>',
        '#include <utility>',  # std::to_underlying (C++23, fallback via underlying_type_t)
        f'#include "{include_path}"',
        '',
    ]

    # ---- operator<< --------------------------------------------------------
    out += [
        '// ================================================================',
        '// operator<< overloads (std::ostream)',
        '// ================================================================',
        '',
    ]
    for e in valid_enums:
        code = (generate_ostream_operator_bitmask(e) if is_bitmask_enum(e.members)
                else generate_ostream_operator(e))
        if code:
            out.append(code)
            out.append('')

    # ---- std::formatter ----------------------------------------------------
    out += [
        '// ================================================================',
        '// std::formatter specialisations (std::format / std::print)',
        '// ================================================================',
        '',
    ]
    for e in valid_enums:
        code = (generate_formatter_bitmask(e) if is_bitmask_enum(e.members)
                else generate_formatter(e))
        if code:
            out.append(code)
            out.append('')

    out += [
        '#ifdef OPTIONAL',
        '#pragma pop_macro("OPTIONAL")',
        '#endif',
    ]

    output = '\n'.join(out)

    if outpath:
        with open(outpath, 'w', encoding='utf-8') as f:
            f.write(output)
        print(f'Output written to {outpath}', file=sys.stderr)
    else:
        print(output)


if __name__ == '__main__':
    main()
