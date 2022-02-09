# Copyright 2021 Xilinx Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#

import imp
import os
import sys

sys.path.insert(0, os.path.abspath("../src/python"))

# to parse the CLI, we need to import it by file path since it's not in a Python package
imp.load_source("proteus_cli", os.path.abspath("../proteus"))


# -- Project information -----------------------------------------------------

project = "Xilinx Inference Server"
copyright = "2021, Xilinx Inc."
author = "Xilinx Inc."

with open(os.path.abspath("../VERSION")) as f:
    raw_version = f.read().strip()

version = "main"
release = f"v{raw_version}"

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "breathe",
    "sphinxarg.ext",
    "sphinxcontrib.confluencebuilder",
    "sphinx.ext.autodoc",
    # automatically labels headings
    "sphinx.ext.autosectionlabel",
    # used to define templatized links (see config below)
    "sphinx.ext.extlinks",
    # adds .nojekyll to the generated HTML for GitHub
    "sphinx.ext.githubpages",
    "sphinx.ext.napoleon",
    "sphinx_copybutton",
]

# Breathe configuration
breathe_projects = {"proteus": "../build/docs/doxygen/xml"}
breathe_default_project = "proteus"

# sphinxcontrib.confluencebuilder configuration
confluence_publish = True
confluence_space_name = "~varunsh"
confluence_parent_page = "Proteus"
confluence_page_hierarchy = True
confluence_ask_user = True
confluence_ask_password = True
confluence_server_url = "https://confluence.xilinx.com/"

# sphinx.ext.autodoc configuration
autosectionlabel_prefix_document = True

# sphinx.ext.extlinks configuration. syntax is key: (url, caption). The key should not have underscores.
extlinks = {
    "ubuntupackages": ("https://packages.ubuntu.com/bionic/%s", "%s"),
    "pypipackages": ("https://pypi.org/project/%s", "%s"),
    "github": ("https://github.com/%s", "%s"),
    "xilinxdownload": (
        "https://www.xilinx.com/bin/public/openDownload?filename=%s",
        None,
    ),
}

# strip leading $ from bash code blocks
copybutton_prompt_text = "$ "

# raise a warning if a cross-reference cannot be found
nitpicky = True

# number all figures with captions
numfig = True

# Configure 'Edit on GitHub' extension
edit_on_github_project = "Xilinx/inference-server"
edit_on_github_branch = "main/docs"

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes. Attempts to use a custom Xilinx theme if it exists,
# otherwise the default theme is used
#
if os.path.exists("./_themes/xilinx"):
    html_theme = "xilinx"
    html_theme_path = ["./_themes"]

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]

html_last_updated_fmt = "%B %d, %Y"
