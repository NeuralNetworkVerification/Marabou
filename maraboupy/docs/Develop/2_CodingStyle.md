# Coding Style Guidelines

Writing code with a consistent style improves readability, reduces the chance of making errors, and 
allows automatic API documentation to be generated correctly. 
This page focuses only on python coding style. See the 
[wiki](https://github.com/NeuralNetworkVerification/Marabou/wiki/Coding-Guidelines)
for the C++ Marabou style guide. 

This page is relevant for writing source python files in the maraboupy directory. 
For information on creating new [tests](3_Tests.md), [examples](4_Examples.md), or [documentation](5_Documentation.md), see
the other pages of documentation.

## Code

An extensive style guide for writing python code can be found 
[here](https://www.python.org/dev/peps/pep-0008/#introduction). Generally, code should be written to match the
style of existing code.

1. Names of classes should start with a capital letter, while methods and attributes should start with a lower-case letter.
2. Camelcase is preferred, though some existing methods instead use underscores to sepearate words.
3. Names of classes, methods, and variables should be descriptive yet concise.
4. Use spaces, not tabs.
5. Add comments as needed to make understanding and modifying your code easier for future developers.
6. Each method (except \_\_init\_\_) and class should have a docstring (see [below](#docstrings)).
7. When in doubt, just follow the existing python style.


## Docstrings
Docstrings are comment blocks that appear at the very beginning of a file, class, or method
that give information about that file, class, or method.
When the API documentation is built, the docstrings from maraboupy python files are used to
automatically generate API documentation. For this reason, it is important to add these docstrings
and ensure that they are formatted correctly. There are a few valid docsting styles, but the style
used in maraboupy is the Google style. This style is briefly explained here, but more information
and examples can be found [here](https://sphinxcontrib-napoleon.readthedocs.io/en/latest/example_google.html).

It is also important to check that docstrings have been formatted correctly by building the html documentation
and looking at the modified API documentation to ensure that it looks correct. The documentation can be built
by following the instructions in the maraboupy/docs README.md file.

### File docstrings
Each file begins with a docstring that explains the contributors, copyright information, and information
about the file. In the future, the docstring can be created and updated automatically by the Marabou team.
The file docstring follows this format:
```
'''
Top contributors (to current version):
    - contributer1
    - contributer2

This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.

Brief description of file.

Longer description if necessary.
'''
```

### Class docstring
The class docstring should be defined immediately after the class is defined, and there should be no docstring
after the \_\_init\_\_ method. Class docstrings take the form:
```
'''Brief description of class

Longer description of class (If needed. Can be ommitted.)

Args:
    arg1 (type): Description of arg1.
    arg2 (type, optional): Description of arg2. Defaults to <default value>.
    
Attributes:
    attr1 (type): Description of attr1
    attr2 (type): Description of attr1
'''
```

It is important to have empty lines between the different docstring sections, as shown in the above example. 
The location of the colon after the type is also important. If the 
class is not initialized with arguments, the Args section can be ommitted. Attributes from a parent class do not need
to be included.

### Method docstring
Method docstrings are very similar to class docstrings, except the Attributes section will be skipped, and a Returns
section will be used if the function returns something. Method docstrings take the form:
```
'''Brief descripton of method

Longer description of method (If needed. Can be ommitted.)

Args:
    arg1 (type): Description arg1.
    arg2 (type, optional): Description of arg2. Defaults to <default value>.
    
Returns:
    (return Type): Description of return 
'''
```

If there are multiple returns, use the format
```
'''
Returns:
    (tuple): tuple containing:
        - return1 (type): Description of return1
        - return2 (type): Description of return2
'''
```

If the type is a class defined in maraboupy, you can create a link to that class using something like
```
'''
Returns:
    (:class:`~marabopy.MarabouUtils.Equation`): New Maraboupy equation
'''
```
A return type like this will show up as a link that will take you to documentation for the MarabouUtils.Equation class.

If a method is private and shouldn't be used by users, and therefore shouldn't be included in the API documentation, add
```
'''

:meta private:
'''
```
at the very end of the docstring.