from compare import compare_output
import os
import shutil
import sys

def recursively_compare(input_dir, output_dir, level=1):
	for f in os.listdir(input_dir):
		rel_path = os.path.join(input_dir, f)
		if os.path.isfile(rel_path) and os.path.splitext(rel_path)[1] == ".mps":
			# compare
			print ("\t" * level + f + ":"),
			print compare_output(rel_path, output_dir)
		elif os.path.isdir(rel_path):
			print ("\t" * level + f + ":")
			recursively_compare(rel_path, output_dir, level+1)

def main(input_dir, output_dir):

	try:
		os.mkdir(output_dir)
	except:
		print "Using %s as output folder" % output_dir
	print (os.path.abspath(input_dir) + ":")
	recursively_compare(input_dir, output_dir)

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print "Need to supply input arguments: directory to read .mps files from, output directory for tmp logs, e.g."
		print "> python compare_all.py <DIR WITH MPS FILES> <DIR FOR OUTPUT FILES>"
		exit(1)
	input_dir = sys.argv[1]
	output_dir = sys.argv[2]
	main(input_dir, output_dir)
