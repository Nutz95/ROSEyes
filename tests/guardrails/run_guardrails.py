#!/usr/bin/env python3
"""ROSEyes firmware/source quality guardrails."""

from __future__ import annotations

import re
import sys
from pathlib import Path

MAX_LINES = 400
MAX_PUBLIC_METHODS = 30

ALLOW_SHORT_NAMES = {
    "x",
    "y",
    "z",
    "w",
    "h",
    "i",
    "j",
    "cx",
    "cy",
    "dx",
    "dy",
    "r",
    "id",
    "ok",
    "ms",
    "ip",
    "x0",
    "y0",
    "x1",
    "y1",
    "x_",
    "y_",
    "z_",
    "w_",
    "h_",
}

CLASS_DEFINE_RE = re.compile(
    r"^\s*(?:class|struct|enum\s+class)\s+([A-Za-z_][A-Za-z0-9_]*)\b"
)
PUBLIC_METHOD_RE = re.compile(
    r"^\s*(?:virtual\s+|static\s+|explicit\s+)*"
    r"(?:[\w:<>\*&\s]+)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*(?:const\s*)?(?:=\s*0\s*)?;"
)
CTOR_RE = re.compile(r"^\s*(?:explicit\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\(")
DOC_BEFORE_RE = re.compile(r"/\*\*|///")
SHORT_IDENT_RE = re.compile(r"\b([a-z]{1,2})\b")


def firmware_headers(repo_root: Path) -> list[Path]:
    include_dir = repo_root / "firmware" / "include"
    return sorted(include_dir.glob("*.h"))


def firmware_sources(repo_root: Path) -> list[Path]:
    src_dir = repo_root / "firmware" / "src"
    return sorted(src_dir.glob("*.cpp"))


def check_one_type_per_header(headers: list[Path]) -> list[str]:
    errors: list[str] = []
    for header in headers:
        text = header.read_text(encoding="utf-8", errors="ignore")
        types = CLASS_DEFINE_RE.findall(text)
        # NetworkSeedConfig.h is macros only — skip empty
        if not types:
            continue
        if len(types) != 1:
            errors.append(
                f"{header}: expected exactly 1 class/struct/enum class, found {types}"
            )
    return errors


def check_file_size(paths: list[Path]) -> list[str]:
    errors: list[str] = []
    for path in paths:
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        if len(lines) > MAX_LINES:
            errors.append(f"{path}: {len(lines)} lines exceeds {MAX_LINES}")
    return errors


def check_public_method_docs_and_count(headers: list[Path]) -> list[str]:
    errors: list[str] = []
    for header in headers:
        lines = header.read_text(encoding="utf-8", errors="ignore").splitlines()
        in_public = False
        public_methods: list[tuple[int, str]] = []
        class_name = None
        for index, line in enumerate(lines):
            type_match = CLASS_DEFINE_RE.match(line)
            if type_match:
                class_name = type_match.group(1)
            if re.match(r"\s*public\s*:", line):
                in_public = True
                continue
            if re.match(r"\s*private\s*:", line) or re.match(r"\s*protected\s*:", line):
                in_public = False
                continue
            if not in_public:
                continue
            ctor = CTOR_RE.match(line)
            if ctor and class_name and ctor.group(1) == class_name:
                public_methods.append((index, ctor.group(1)))
                continue
            method = PUBLIC_METHOD_RE.match(line)
            if method:
                name = method.group(1)
                if name in {"if", "for", "while", "switch", "return"}:
                    continue
                public_methods.append((index, name))

        if len(public_methods) > MAX_PUBLIC_METHODS:
            errors.append(
                f"{header}: {len(public_methods)} public methods exceeds {MAX_PUBLIC_METHODS}"
            )

        for line_index, method_name in public_methods:
            window = "\n".join(lines[max(0, line_index - 12) : line_index])
            if not DOC_BEFORE_RE.search(window):
                errors.append(
                    f"{header}:{line_index + 1}: public method '{method_name}' missing doc comment"
                )
    return errors


def check_short_identifiers(paths: list[Path]) -> list[str]:
    errors: list[str] = []
    # Focus on declarations: look for suspicious parameter / member names in headers.
    param_re = re.compile(r"\b(?:int|float|double|uint\d+_t|size_t|bool|char)\s+([a-zA-Z_][a-zA-Z0-9_]*)\b")
    for path in paths:
        if path.suffix != ".h":
            continue
        for line_no, line in enumerate(
            path.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1
        ):
            if line.strip().startswith("//") or line.strip().startswith("*"):
                continue
            for match in param_re.finditer(line):
                name = match.group(1)
                if name.lower() in ALLOW_SHORT_NAMES:
                    continue
                if len(name) <= 2 and name.lower() == name:
                    errors.append(
                        f"{path}:{line_no}: cryptic identifier '{name}' — use a meaningful name"
                    )
    return errors


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: run_guardrails.py <repo_root>")
        return 2
    repo_root = Path(sys.argv[1]).resolve()
    headers = firmware_headers(repo_root)
    sources = firmware_sources(repo_root)
    all_paths = headers + sources

    errors: list[str] = []
    errors.extend(check_one_type_per_header(headers))
    errors.extend(check_file_size(all_paths))
    errors.extend(check_public_method_docs_and_count(headers))
    errors.extend(check_short_identifiers(headers))

    if errors:
        print(f"Guardrails FAILED ({len(errors)} issue(s)):")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"Guardrails OK ({len(headers)} headers, {len(sources)} sources)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
