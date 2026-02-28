#!/usr/bin/env python3
"""
Post-build patcher for Emscripten's LEGACY_GL_EMULATION JavaScript output.

Emscripten's LEGACY_GL_EMULATION has several issues when used with games that
rely heavily on the fixed-function OpenGL pipeline:

1. getCurTexUnit() crashes when s_texUnits is null (before GLImmediate is
   fully initialized).
2. Several GL functions throw "TODO" errors instead of being no-ops.

This script patches the generated JS to work around these issues.
Run it after `cmake --build` on the Emscripten output.

Usage:
    python3 patch_emscripten_js.py <path/to/billyfrontier.js>
"""

import sys
import re


def patch(content: str) -> str:
    patches_applied = []

    # 1. Make getCurTexUnit null-safe
    old = "function getCurTexUnit(){return s_texUnits[s_activeTexture]}"
    new = ("function getCurTexUnit(){"
           "if(!s_texUnits||!s_texUnits[s_activeTexture])return null;"
           "return s_texUnits[s_activeTexture]}")
    if old in content:
        content = content.replace(old, new, 1)
        patches_applied.append("getCurTexUnit null-safety")

    # 2. hook_enable: guard against null getCurTexUnit()
    old = "hook_enable:cap=>{var cur=getCurTexUnit();switch"
    new = "hook_enable:cap=>{var cur=getCurTexUnit();if(!cur)return;switch"
    if old in content:
        content = content.replace(old, new, 1)
        patches_applied.append("hook_enable null guard")

    # 3. hook_disable: guard against null getCurTexUnit()
    old = "hook_disable:cap=>{var cur=getCurTexUnit();switch"
    new = "hook_disable:cap=>{var cur=getCurTexUnit();if(!cur)return;switch"
    if old in content:
        content = content.replace(old, new, 1)
        patches_applied.append("hook_disable null guard")

    # 4. getCurTexUnit().env accesses: null-safe
    old_p = "var env=getCurTexUnit().env"
    new_p = "var _cu=getCurTexUnit();if(!_cu)return;var env=_cu.env"
    count = content.count(old_p)
    if count > 0:
        content = content.replace(old_p, new_p)
        patches_applied.append(f"getCurTexUnit().env null guard (x{count})")

    # 5. Convert TODO throws to silent returns
    #    These are unimplemented GL functions that throw instead of being no-ops.
    todo_throws = [
        'throw"glLightModelfv: TODO: "+pname',
        'throw"glLightfv: TODO: "+pname',
        'throw"glMaterialfv: TODO"+face',
        'throw"glMaterialfv: TODO: "+pname',
        'throw"glTexGeni: TODO"',
    ]
    for throw_str in todo_throws:
        if throw_str in content:
            content = content.replace(throw_str, "return", 1)
            patches_applied.append(f"TODO throw -> return: {throw_str[:40]}...")

    # 6. Also handle single-quoted variants (different Emscripten versions)
    for throw_str in todo_throws:
        sq = throw_str.replace('"', "'")
        if sq in content:
            content = content.replace(sq, "return", 1)
            patches_applied.append(f"TODO throw -> return (sq): {sq[:40]}...")

    return content, patches_applied


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <path/to/billyfrontier.js>", file=sys.stderr)
        sys.exit(1)

    js_path = sys.argv[1]

    with open(js_path, "r") as f:
        content = f.read()

    content, patches = patch(content)

    with open(js_path, "w") as f:
        f.write(content)

    if patches:
        print(f"Applied {len(patches)} patches to {js_path}:")
        for p in patches:
            print(f"  - {p}")
    else:
        print(f"No patches needed for {js_path}")


if __name__ == "__main__":
    main()
