import hashlib
import re
from pathlib import Path

_DSL      = frozenset({"Internal DSL"})
_RUNTIME  = frozenset({"Core API - Runtime"})
_LEVELS   = frozenset({"Levels", "Complete Example", "See Also"})


def on_config(config):
    here = Path(__file__).parent
    config["extra_css"] = [_bust(here, css) for css in config["extra_css"]]
    if config["theme"].get("logo"):
        config["theme"]["logo"] = _bust(here, config["theme"]["logo"])
    return config


def _bust(here, css_path):
    full = here / "src" / css_path
    if not full.exists():
        return css_path
    digest = hashlib.md5(full.read_bytes()).hexdigest()[:8]
    return f"{css_path}?v={digest}"


def _parse(text):
    parts = re.split(r'^(## [^\n]+)', text, flags=re.MULTILINE)
    intro = parts[0]
    sections = []
    for i in range(1, len(parts), 2):
        heading = parts[i]
        body    = parts[i + 1] if i + 1 < len(parts) else ""
        name    = heading[3:].strip()
        sections.append((name, heading, body))
    return intro, sections


def _join(sections, names):
    return "".join(h + b for n, h, b in sections if n in names)


def on_pre_build(config):
    here = Path(__file__).parent
    src  = here / "src"

    (src / "platform.md").write_text(
        (here.parent / "platform.md").read_text()
    )
    (src / "physics.md").write_text(
        (here.parent / "pulse2d" / "graphics" / "readme.md").read_text()
    )

    text         = (here.parent / "api.md").read_text()
    intro, parts = _parse(text)

    # strip the H1 title from the intro; each file gets its own
    intro_body = re.sub(r'^# [^\n]+\n', '', intro, count=1)

    (src / "dsl.md").write_text(
        "# Internal DSL\n" + intro_body + _join(parts, _DSL)
    )
    (src / "runtime.md").write_text(
        "# Core API\n\n" + _join(parts, _RUNTIME)
    )
    (src / "levels.md").write_text(
        "# Levels\n\n" + _join(parts, _LEVELS)
    )
