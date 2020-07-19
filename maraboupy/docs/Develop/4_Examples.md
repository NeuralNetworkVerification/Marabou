# Examples

Example files can be helpful to show users a variety of ways that Maraboupy can be used.
The example files are simple scripts that live in the maraboupy/examples folder that contain
commented demonstrations of different Maraboupy functions.

The example files are automatically pulled into this documentation using sphinx_gallery, which
creates html pages with alternating text and code blocks. Text blocks are generated from 
comment lines that begin with a "# %%" comment line. Code blocks begin when the lines of
comments end. In the future, these examples could be run when building the documentation, and 
outputs or plots from execution could be incorporated into the html. However, the examples 
currently are not executed, which is why the bottom of the html files says the examples 
run in 0 seconds. Also, the bottom of the html files allows a user to download a python or notebook
file for the example directly from the html documentation.

The example files should begin with a comment block that has the example name
followed by a line of equals lines. This is the standard way to write a header line in
reStucturedText format. Comment lines can also leverage the reStructredText format to make headers,
links, bold text, etc. 

The names of the example files are prefixed with a number, which indicates the order in which the
examples will appear in the menu of the html documentation.

When adding a new example file, it is a good idea to build the documentation locally and ensure that
the html generated from the example looks correct. Instructions for building the documentation can
be found in the README.md of the maraboupy/docs folder.