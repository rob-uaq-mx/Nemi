#!/usr/bin/env python3
"""manual_to_html.py -- convert the closed set of cross-linked docs
(manual/*.md, Nemi.md, bibcom/README.md) to standalone HTML, into docs/ at
the repo root. Used both by WinNemi's "Ayuda" menu (see WinNemi.cpp's
IDM_HELP, and Windows/installer/WinNemi.iss which packages docs/*) and by
GitHub Pages (which serves docs/ directly, see docs/index.html).

Usage (no arguments -- operates on fixed paths relative to this script):
    python manual_to_html.py

The three sources cross-link only to each other (manual/*.md links to
sibling chapters and to "../Nemi.md"/"../bibcom/README.md"; bibcom/README.md
links back to "../manual/*.md"; Nemi.md has no outgoing .md links) -- a
closed set, verified by grep before writing this. Every ".md" link found in
these three sources is therefore rewritten to ".html", and the output
mirrors the same relative layout so those links keep resolving:

    docs/manual/*.html      (from manual/*.md)
    docs/Nemi.html          (from Nemi.md)
    docs/bibcom/README.html (from bibcom/README.md)

Requires `pandoc` on PATH (tested with 3.8.2.1). The output is committed to
the repo, like Windows/apps/toolbar_std.bmp -- rerun this by hand whenever
one of the three sources changes, don't wire it into every build.
"""
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE / ".." / ".."
DOCS_DIR = REPO_ROOT / "docs"
CSS_PATH = HERE / "manual.css"

# Every cross-link in this closed set is a relative ".md" link (with or
# without a leading "../"); every target is one of the three sources being
# converted, so it's always safe to rewrite to ".html".
MD_LINK = re.compile(r'\]\(((?:\.\./)*[\w./-]+)\.md\)')


def convert(md_path: Path, out_path: Path) -> None:
    source = md_path.read_text(encoding="utf-8")
    source = MD_LINK.sub(r"](\1.html)", source)

    # No --metadata title: pandoc's html5 template renders a *visible*
    # "<h1 class="title">" for an explicit title, which would duplicate the
    # chapter's own "# ..." heading. Left unset, pandoc still fills <title>
    # in <head> (browser tab text) from the source filename on its own.
    result = subprocess.run(
        [
            "pandoc",
            "-f", "markdown",
            "-t", "html5",
            "--standalone",
            "--embed-resources",
            "--css", str(CSS_PATH),
        ],
        input=source,
        capture_output=True,
        text=True,
        encoding="utf-8",
    )
    if result.returncode != 0:
        raise RuntimeError(f"pandoc failed on {md_path}:\n{result.stderr}")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(result.stdout, encoding="utf-8")


def main() -> int:
    manual_dir = REPO_ROOT / "manual"
    jobs = [(md, DOCS_DIR / "manual" / (md.stem + ".html"))
            for md in sorted(manual_dir.glob("*.md"))]
    jobs.append((REPO_ROOT / "Nemi.md", DOCS_DIR / "Nemi.html"))
    jobs.append((REPO_ROOT / "bibcom" / "README.md", DOCS_DIR / "bibcom" / "README.html"))

    missing = [md for md, _ in jobs if not md.is_file()]
    if missing:
        for md in missing:
            print(f"no se encontró {md}", file=sys.stderr)
        return 1

    for md_path, out_path in jobs:
        convert(md_path, out_path)
        print(f"{md_path.relative_to(REPO_ROOT)} -> {out_path.relative_to(REPO_ROOT)}")

    print(f"{len(jobs)} archivo(s) convertido(s) en {DOCS_DIR}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
