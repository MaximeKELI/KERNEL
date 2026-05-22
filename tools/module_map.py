#!/usr/bin/env python3
"""Classify kernel modules by implementation depth."""

import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KERNEL = ROOT / "kernel"

STUB_PATTERNS = (
    re.compile(r"/\*\s*TODO:\s*Implement", re.I),
    re.compile(r"^\s*printk\([^)]*Initialized", re.I),
)
TODO_RE = re.compile(r"\bTODO\b", re.I)


def count_lines(path: Path) -> int:
    try:
        return len(path.read_text(encoding="utf-8", errors="replace").splitlines())
    except OSError:
        return 0


def classify_c_file(path: Path) -> str:
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = [ln for ln in text.splitlines() if ln.strip() and not ln.strip().startswith("//")]
    n = len(lines)
    todo_count = len(TODO_RE.findall(text))
    has_stub = any(p.search(text) for p in STUB_PATTERNS)
    fn_count = len(re.findall(r"^\w[\w\s\*]*\s+\w+\s*\([^;]*\)\s*\{", text, re.M))

    if n <= 25 and (has_stub or fn_count <= 2):
        return "stub"
    if n <= 60 and (has_stub or todo_count >= 2):
        return "partial"
    if todo_count >= 4 and n < 120:
        return "partial"
    if fn_count >= 5 and n >= 80 and not has_stub:
        return "full"
    if n >= 100 and todo_count <= 2:
        return "full"
    return "partial"


def main() -> None:
    by_dir: dict[str, dict[str, list[str]]] = {}
    totals = {"full": 0, "partial": 0, "stub": 0}

    for path in sorted(KERNEL.rglob("*.c")):
        rel = path.relative_to(KERNEL)
        top = rel.parts[0] if len(rel.parts) > 1 else "(root)"
        cat = classify_c_file(path)
        totals[cat] += 1
        by_dir.setdefault(top, {"full": [], "partial": [], "stub": []})
        by_dir[top][cat].append(f"{rel} ({count_lines(path)} lines)")

    out = ROOT / "Documentation" / "MODULE_MAP.md"
    out.parent.mkdir(parents=True, exist_ok=True)

    lines = [
        "# Cartographie des modules du noyau",
        "",
        "Classification automatique (heuristique) :",
        "",
        "- **full** : logique substantielle, plusieurs fonctions",
        "- **partial** : code présent mais TODO / fonctionnalités incomplètes",
        "- **stub** : surtout `*_init()` + message ou squelette minimal",
        "",
        f"**Totaux** : {totals['full']} complets, {totals['partial']} partiels, {totals['stub']} stubs "
        f"({sum(totals.values())} fichiers `.c` sous `kernel/`).",
        "",
    ]

    for top in sorted(by_dir):
        lines.append(f"## `{top}/`")
        lines.append("")
        for cat in ("full", "partial", "stub"):
            items = by_dir[top][cat]
            if not items:
                continue
            lines.append(f"### {cat} ({len(items)})")
            lines.append("")
            for item in items:
                lines.append(f"- `{item}`")
            lines.append("")

    out.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {out}")
    print(f"Totals: {totals}")


if __name__ == "__main__":
    main()
