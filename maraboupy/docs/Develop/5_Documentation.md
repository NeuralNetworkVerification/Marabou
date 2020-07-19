# Documentation

Additional pages of documentation can be created to improve this documentation by adding a markdown or
reStructuredText file to an existing subfolder of maraboupy/docs. Note that the documentation files are prefixed with
a number to indicate the order in which the files should appear in the html documentation.

To add a new section of documentation, you can add a subfolder to the maraboupy/docs folder and fill it with
your documentation markdown or reStructuredText files. In addition, you'll need to edit the maraboupy/docs/index.rst
file to add your subfolder to the toctree. Follow the existing format for Setup to see how this can be done. The
"maxdepth" option determines how many levels deep should show in your menu. The "glob" option allows you to pull in
all files in a directory. The "caption" option specifies that name that will be displayed in the menu.