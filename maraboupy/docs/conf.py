# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))
from sphinx_gallery.sorting import FileNameSortKey

# -- Project information -----------------------------------------------------

project = 'Maraboupy'
copyright = '2020, The Marabou Team'
author = 'The Marabou Team'

# Documentation versioning
#version = '0.0.1'
#release = '0.0.1'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ['sphinx.ext.autodoc','sphinx.ext.linkcode','sphinx.ext.intersphinx',
              'sphinx.ext.napoleon', 'recommonmark', 'sphinx_gallery.gen_gallery'
]

# Add any paths that contain templates here, relative to this directory.
#templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'README*']


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'sphinx_rtd_theme'

# Style and colors for code blocks
pygments_style = 'sphinx'
#highlight_language = 'python3'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
#html_static_path = ['_static']

# Enable markdown pages to be used alongside reStructuredText
# pip install recommonmark
source_suffix = ['.rst', '.md']

sphinx_gallery_conf = {
     'examples_dirs': '../examples',   # path to your example scripts
     'gallery_dirs': 'Examples',  # path to where to save gallery generated output
     'within_subsection_order': FileNameSortKey
#     'filename_pattern': r'/[^_].*py', # uncomment this line to execute example files when building html
}

autodoc_default_options = {
    'show-inheritance': True
}

def linkcode_resolve(domain, info):
    """Link documentation to github"""
    if domain != 'py':
        return None
    if not info['module']:
        return None
    filename = info['module'].replace('.', '/')
    ext = "py"
    if "Core" in filename:
        ext = "cpp"
    return "https://github.com/NeuralNetworkVerification/Marabou/blob/master/%s.%s" % (filename, ext)
