import sys
from maraboupy.MarabouMain import main


def test_main():
    sys.argv = ["Marabou", "--version"]
    try:
        main()
    except SystemExit as e:
        assert e.code == 0
