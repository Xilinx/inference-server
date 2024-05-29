# Copyright 2021 Xilinx, Inc.
# Copyright 2022 Advanced Micro Devices, Inc.
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

sys.path.insert(0, os.path.abspath(".."))

# to parse the CLI, we need to import it by file path since it's not in a Python package
imp.load_source("amdinfer_cli", os.path.abspath("../amdinfer"))


# -- Project information -----------------------------------------------------

project = "AMD Inference Server"
copyright = "2022 Advanced Micro Devices, Inc."
author = "Advanced Micro Devices, Inc."

# override this value to build different versions
version = "main"

if version != "main" and version != "dev":
    release = f"v{version}"
else:
    release = version

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinxcontrib.jquery",
    "breathe",
    # omitting the full C++ documentation generation for now
    # "exhale",
    # adds argparse directive to parse CLIs
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
    "sphinx_issues",
    "sphinx_tabs.tabs",
    # adds tooltips
    "sphinx_tippy",
    # add emoji
    "sphinxemoji.sphinxemoji",
    "sphinxcontrib.openapi",
    "sphinx_charts.charts",
    "sphinx_favicon",
]

# Breathe configuration
breathe_projects = {"amdinfer": "../build/docs/doxygen/xml"}
breathe_default_project = "amdinfer"

# Exhale configuration
exhale_args = {
    # These arguments are required
    "containmentFolder": "./cpp_api",
    "rootFileName": "cpp_root.rst",
    "rootFileTitle": "Code Documentation",
    "doxygenStripFromPath": ".",
    # Suggested optional arguments
    "createTreeView": True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
    "unabridgedOrphanKinds": {"define", "dir", "typedef", "variable", "file"},
    # exclude PIMPL classes
    "listingExclude": [r".*Impl$"],
}

# sphinxcontrib.confluencebuilder configuration
confluence_publish = True
confluence_space_name = "~varunsh"
confluence_parent_page = "AMDinfer"
confluence_page_hierarchy = True
confluence_ask_user = True
confluence_ask_password = True
confluence_server_url = "https://confluence.xilinx.com/"

# sphinx.ext.autodoc configuration
autodoc_default_options = {
    "members": True,
    "special-members": "__init__",
}

tippy_add_class = "has-tippy"
tippy_skip_urls = [
    # skip all URLs except those pointing to the glossary
    r"^((?!glossary\.html).)*$"
]
tippy_enable_wikitips = False
tippy_enable_doitips = False

sphinxemoji_style = "twemoji"


def hide_private_module(app, what, name, obj, options, signature, return_annotation):
    if signature is not None:
        new_signature = signature.replace("amdinfer._amdinfer", "amdinfer")
    else:
        new_signature = signature

    if return_annotation is not None:
        new_return = return_annotation.replace("amdinfer._amdinfer", "amdinfer")
    else:
        new_return = return_annotation

    return (new_signature, new_return)


# sphinx.ext.autosectionlabel configuration
# prefix all generated labels with the document
autosectionlabel_prefix_document = True
# only auto-label top-level headings to prevent duplication when the same title
# is used in different sections
autosectionlabel_maxdepth = 2

tree_path = f"https://github.com/Xilinx/inference-server/tree/{release}/%s"
blob_path = f"https://github.com/Xilinx/inference-server/blob/{release}/%s"
raw_path = f"https://github.com/Xilinx/inference-server/raw/{release}/%s"
xilinx_download = "https://www.xilinx.com/bin/public/openDownload?filename=%s"
github_onnx = (
    "https://github.com/onnx/models/raw/5faef4c33eba0395177850e1e31c4a6a9e634c82/%s"
)
vitis_ai_path = "https://github.com/Xilinx/Vitis-AI/tree/v3.0/%s"

# sphinx.ext.extlinks configuration. syntax is key: (url, caption). The key should not have underscores.
extlinks = {
    "amdinferTree": (tree_path, "%s"),
    "amdinferBlob": (blob_path, "%s"),
    "amdinferRawFull": (raw_path, None),
    "ubuntuPackages": ("https://packages.ubuntu.com/focal/%s", "%s"),
    "pypiPackages": ("https://pypi.org/project/%s", "%s"),
    "github": ("https://github.com/%s", "%s"),
    "xilinxDownload": (xilinx_download, "%s"),
    "githubOnnx": (github_onnx, None),
    "vitisAItree": (vitis_ai_path, None),
}

# sphinx-issues configuration
issues_default_group_project = "Xilinx/inference-server"

# strip leading $ from bash code blocks
copybutton_prompt_text = "$ "
copybutton_here_doc_delimiter = "EOF"
# selecting the literal block doesn't work to show the copy button correctly
# copybutton_selector = ":is(div.highlight pre, pre.literal-block)"

# raise a warning if a cross-reference cannot be found
nitpicky = True

# number all figures with captions
numfig = True

rst_prolog = """
.. include:: /prolog.rst
"""

rst_epilog = """
.. include:: /epilog.rst
"""

# Configure 'Edit on GitHub' extension
edit_on_github_project = "Xilinx/inference-server"
edit_on_github_branch = f"{release}/docs"

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "**/build/**",
    "**/uploads/**",
    "epilog.rst",
    "prolog.rst",
]


# -- Options for HTML output -------------------------------------------------

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]

html_last_updated_fmt = "%B %d, %Y"

favicons = [
    {"rel": "apple-touch-icon", "sizes": "180x180", "href": "apple-touch-icon.png"},
    {"rel": "icon", "type": "image/png", "sizes": "32x32", "href": "favicon-32x32.png"},
    {"rel": "icon", "type": "image/png", "sizes": "16x16", "href": "favicon-16x16.png"},
    {"rel": "manifest", "href": "site.webmanifest"},
    {"rel": "mask-icon", "href": "safari-pinned-tab.svg", "color": "#5bbad5"},
    {"name": "msapplication-TileColor", "content": "#ed1c24"},
    {"name": "theme-color", "content": "#ed1c24"},
]

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes. Attempts to use a custom Xilinx theme if it exists,
# otherwise the default theme is used
#
if os.path.exists("./_themes/xilinx"):
    html_title = "AMD Inference Server"
    html_logo = "_static/xilinx-header-logo.svg"
    html_theme = "xilinx"
    html_theme_path = ["./_themes"]
    html_theme_options = {
        "logo_only": False,
        "style_nav_header_background": "black",
        # set to true to hide the expand buttons on headings in the sidebar
        # until the heading is clicked on
        "collapse_navigation": False,
        "navigation_depth": 3,
        # prevent the sidebar from scrolling while the main window is scrolled
        "sticky_navigation": False,
    }
    using_rtd_theme = True

    # using this guide for enabling RTD-style version / language options:
    # https://tech.michaelaltfield.net/2020/07/23/sphinx-rtd-github-pages-2/
    html_context = {
        "display_lower_left": True,
        "current_language": "en",
        "current_version": version,
        "version": version,
    }

    html_context["languages"] = [("en", "/" + "inference-server/" + version + "/")]

    versions = ["0.1.0", "0.2.0", "0.3.0", "0.4.0"]
    versions.append("main")
    html_context["versions"] = []
    for version in versions:
        html_context["versions"].append(
            (version, "/" + "inference-server/" + version + "/")
        )

    html_css_files = [
        "custom.css",
        "https://cdn.datatables.net/1.13.4/css/jquery.dataTables.min.css",
        # "https://cdn.datatables.net/fixedcolumns/4.2.2/css/fixedColumns.dataTables.min.css",
    ]

    html_js_files = [
        "https://cdn.datatables.net/1.13.4/js/jquery.dataTables.min.js",
        # "https://cdn.datatables.net/fixedcolumns/4.2.2/js/dataTables.fixedColumns.min.js",
        "dataTables.js",
    ]


def setup(app):
    app.connect("autodoc-process-signature", hide_private_module)
