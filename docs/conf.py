# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/master/config

import os
import sys
import re
from subprocess import check_output

# -- Project information -----------------------------------------------------

project = 'Trusted Firmware-M'
copyright = '2017-2025, ARM CE-OSS'
author = 'ARM CE-OSS'
title = 'User Guide'

# -- Extract current version -------------------------------------------------

try:
    vrex = re.compile(r'TF-M(?P<GIT_VERSION>v.+?)'
                      r'(-[0-9]+-g)?(?P<GIT_SHA>[a-f0-9]{7,})?$')

    version = check_output("git describe --tags --always",
                            shell = True, encoding = 'UTF-8')

    _v = vrex.match(version)
    release = _v.group('GIT_VERSION')
    if _v.group('GIT_SHA'):
        version = release + "+" + _v.group('GIT_SHA')[:7]

except:
    version = release = 'Unknown'

# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.imgmath',
    'm2r2', #Support markdown files. Needed for external code.
    'sphinx.ext.autosectionlabel', #Make sphinx generate a label for each section
    'sphinxcontrib.plantuml', #Add support for PlantUML drawings
    'sphinxcontrib.rsvgconverter', #Add support for SVG to PDF
    'sphinx_tabs.tabs', #Enable tab extension in Sphinx
    'sphinx.ext.intersphinx', #Enable Intersphinx
]

intersphinx_mapping = {
    "TF-M-Tests": ("https://trustedfirmware-m.readthedocs.io/projects/tf-m-tests/en/latest/", None),
    "TF-M-Tools": ("https://trustedfirmware-m.readthedocs.io/projects/tf-m-tools/en/latest/", None),
    "TF-M-Extras": ("https://trustedfirmware-m.readthedocs.io/projects/tf-m-extras/en/latest/", None),
}

intersphinx_disabled_reftypes = ["*"]

#PlantUML
if 'PLANTUML_JAR_PATH' in os.environ:
    plantuml = 'java -jar ' + os.environ['PLANTUML_JAR_PATH']

# Make auto section labels generated be prefixed with file name.
autosectionlabel_prefix_document=True
# Add auto section label for level 2 headers only.
autosectionlabel_maxdepth=2

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
source_suffix = ['.rst', '.md']

# The master toctree document.
master_doc = 'index'

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = None

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path .
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'readme.rst',
                    'platform/cypress/psoc64/security/keys/readme.rst',
                    'lib/ext/**']

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'

html_theme_options = {
    'collapse_navigation' : False,
    'prev_next_buttons_location' : None,   # Hide Prev and Next buttons
    'display_version': True,    # Show version under logo
    'sticky_navigation': True,
    'navigation_depth': 3,
}

# Remove the "View page source" link from the top of docs pages
html_show_sourcelink = False

# Add any paths that contain custom static files (such as style sheets) here,
# relative to configuration directory. They are copied after the builtin static
# files, so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# Set the documentation logo relative to configuration directory
html_logo = '_static/images/tf_logo_white.png'

# Set the documentation favicon
html_favicon = '_static/images/favicon.ico'

#Disable adding conf.py copyright notice to HTML output
html_show_copyright = False

# Disable showing Sphinx footer message:
# "Built with Sphinx using a theme provided by Read the Docs. "
html_show_sphinx = False

#Add custom css for HTML. Used to allow full page width rendering
def setup(app):
    app.add_css_file('css/tfm_custom.css')

# -- Options for HTMLHelp output ---------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'TF-M doc'

# Enable figures and tables auto numbering
numfig = True
numfig_secnum_depth = 0
numfig_format = {
    'figure': 'Figure %s:',
    'table': 'Table %s:',
    'code-block': 'Listing %s:',
    'section': '%s'
}

# -- Options for LaTeX output ------------------------------------------------

latex_elements = {
    # 'papersize': 'letterpaper',
    # 'pointsize': '10pt',
    # 'preamble': '',
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'TF-M.tex', title,
     author, 'manual'),
]

language = 'en'
