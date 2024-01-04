import sys
import maraboupy.MarabouCore

def main():
    exitCode = maraboupy.MarabouCore.maraboupyMain(sys.argv)
    exit(exitCode)
