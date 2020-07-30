# Maraboupy examples

This folder contains a collection of example files showing how Maraboupy can be used.
These files can be run after Marabou and Maraboupy have been built. Make sure that
the Marabou root directory has been added to PYTHONPATH. These examples use networks
stored in the root directory's resources folder.

## Adding new example files
New example files can be added to this folder, and they will automatically be included
in the documentation created in the maraboupy/docs folder. To make the derived documentation
look correct, follow these basic rules:
1. A comment block should be at the top and contain the title followed by a line of "="
2. The remaining lines of the comment block can give information or documentation about the file
3. Begin comment lines with a preceding "# %%" to denote the beginning of a text block
4. Subsections can be created in a comment line by writing the subsection title followed 
by a line of "-" characters
5. Append a number prefix to the name of the file to indicate order of files used in documentation

Following these steps will produce documentation with alternating code and text blocks. See
one of the existing files as an example.
