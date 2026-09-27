#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["pcons>=0.24"]
# ///
"""Pcons build for ollama_gen.

Usage::

    uvx pcons -B build/pcons
    uvx pcons -B build/pcons VARIANT=debug
    uvx pcons -B build/pcons TESTS=0
    uvx pcons -B build/pcons all test
    uvx pcons -B build/pcons PCONS_INSTALL_PREFIX=/usr/local all install

``PREFIX`` adds a non-standard dependency prefix to pkg-config's search path.
``PCONS_FINAL_PREFIX`` controls the prefix written to ollama_gen.pc when an
install is staged somewhere other than its eventual runtime location.
"""

import os
from pathlib import Path

from pcons import Project, find_c_toolchain, get_platform, get_var

VERSION = "1.0.0"

project_dir = Path(__file__).parent.resolve()
platform = get_platform()


def option(name: str, default: bool = False) -> bool:
    """Read an ON/OFF build option."""
    return get_var(name, "1" if default else "0").lower() in (
        "1",
        "on",
        "true",
        "yes",
    )


# Make dependencies installed under a custom prefix visible to pkg-config.
extra_prefixes = [
    Path(prefix)
    for prefix in (get_var("PREFIX") or "").split(os.pathsep)
    if prefix
]
if extra_prefixes:
    os.environ["PKG_CONFIG_PATH"] = os.pathsep.join(
        [str(prefix / "lib" / "pkgconfig") for prefix in extra_prefixes]
        + [os.environ.get("PKG_CONFIG_PATH", "")]
    )

project = Project("ollama_gen", root_dir=project_dir)
if project.is_top_level:
    env = project.Environment(toolchain=find_c_toolchain())
    env.set_variant(get_var("VARIANT", "release"))
else:
    # Reuse the including project's compiler and variant when consumed with
    # add_subdirectory().
    env = project.parent.default_environment.clone()

env.cxx.set_standard(20)
if not platform.is_windows:
    # ollama_gen is static and is also linked into mxdbg's shared library.
    env.cxx.flags.append("-fPIC")

curl = project.find_package("libcurl")
jsoncpp = project.find_package("jsoncpp")
assert curl is not None and jsoncpp is not None

ollama_gen = project.StaticLibrary(
    "ollama_gen",
    env,
    sources=[project_dir / "mx2-ollama.cpp"],
)
ollama_gen.public.include_dirs.append(project_dir)
ollama_gen.link(curl, jsoncpp)

if project.is_top_level and option("TESTS", default=True):
    json_stream_test = project.Program(
        "ollama_gen_json_test",
        env,
        sources=[project_dir / "tests" / "json_stream_test.cpp"],
    )
    json_stream_test.link(ollama_gen)
    project.Test(
        "ollama_gen_json_stream",
        json_stream_test,
        env={"OPENAI_API_KEY": "", "ANTHROPIC_API_KEY": ""},
        labels=["unit"],
    )

if project.is_top_level:
    final_prefix = get_var(
        "PCONS_FINAL_PREFIX",
        get_var("PCONS_INSTALL_PREFIX", str(project_dir / "dist")),
    )
    pc_file = project.generate_pc_file(
        ollama_gen,
        version=VERSION,
        description="C++ client library for Ollama, OpenAI, and Anthropic",
        install_prefix=final_prefix,
    )
    project.Alias(
        "install",
        project.Install("lib", [ollama_gen]),
        project.Install("include", [project_dir / "mx2-ollama.hpp"]),
        project.Install("lib/pkgconfig", [pc_file]),
    )
