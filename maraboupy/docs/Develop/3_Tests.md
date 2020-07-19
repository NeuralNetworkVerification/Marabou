# Tests

The maraboupy code is tested to ensure that methods run correctly and without errors, or that user
errors are caught and handled appropriately. These tests live in the maraboupy/test folder and are
defined in files with the "test_"prefix. Each test file contains a set of methods, where methods
beginning with "test_" are the tests, and other methods are helper methods. Note that no main method is
needed in these test files because tests are run using pytest.

New tests can be created by creating a new test file, or by adding new test methods to an existing test
file. Neural network files used in these tests live in the resources folder, located in the root Marabou
directory. When adding new tests or test files, try to follow the syntax of other test files or test methods.

This page covers some basic information about how to write these tests. More information can be found in the
[pytest](https://docs.pytest.org/en/stable/) documentation.

## Writing a test
When writing a test, make sure that pytest has been imported. Also, relative imports are used to import Marabou
so that the tests can be run from any directory regardless of if PYTHONPATH points to the Marabou directory.
For this reason, paths to test networks are also written relative to the location of the test folder. Combining
```
os.path.dirname(__file__)
```
with relative paths allows the test to find the test networks correctly.

The test methods should have a docstring as well to explain the purpose of the test. The Google docstring
format is not required though, because these methods are not used to generate API documentation.

Each test method should test some portion of the Maraboupy code, and assert statements should be used to
ensure that Maraboupy behaved correctly.

## Writing a temporary file
Pytest has a way of generating temporary files that can be used to test functions that write files. To use a
temporary file, add the "tmpdir" argument to your test method. You can use that variable to create a temporary
file using
```
tempFile = tmpdir.mkdir("tmpFolder").join("filename").strpath
```
Then you can use tempFile as your filename.