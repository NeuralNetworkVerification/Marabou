import shutil
import sys

if __name__ == '__main__':
    src = sys.argv[1]
    dest = sys.argv[2]
    shutil.copy2(src, dest)
