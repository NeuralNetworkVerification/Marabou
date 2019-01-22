'''
/* *******************                                                        */
/*! \file compare.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Rachel Lim
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/
'''

from enum import Enum
from format import bcolors
from threading import Timer
import subprocess, shlex
import os

Status = Enum('Status', 'SAT UNSAT PARSE_FAILURE')

def run(cmd, timeout_sec=120, stdout=subprocess.PIPE):
	proc = subprocess.Popen(shlex.split(cmd), stdout=stdout, stderr=subprocess.PIPE)
	kill_proc = lambda p: p.kill()
	timer = Timer(timeout_sec, kill_proc, [proc])
	try:
		timer.start()
		stdout, stderr = proc.communicate()
	finally:
		timer.cancel()

def compare_output(filename, output_dir="./"):
	marabou_op = os.path.join(output_dir, os.path.basename(filename) + ".m.out")
	glpsol_op = os.path.join(output_dir, os.path.basename(filename) + ".g.out")
	f = open(marabou_op, 'w')
	run('glpsol %s -o %s --nomip' % (filename, glpsol_op))
	run('./mps_gensol.elf %s' % filename, stdout=f)
	# glpsol = subprocess.call(['glpsol', filename, '--write', 'glpsol-wr.tmp', '--nomip'], stdout=subprocess.PIPE)
	# marabou = subprocess.call(['./mps_gensol.elf', filename], stdout=f)

	marabou_status = None
	glpk_status = None
	f.close()

	try:
		f = open(marabou_op, 'r')
		for l in f:
			if 'Status' in l:
				if 'UNSAT' in l:
					marabou_status = Status.UNSAT
				elif 'PARSE_FAILURE' in l:
					marabou_status = Status.PARSE_FAILURE
				else:
					marabou_status = Status.SAT
				break
		f.close()
	except:
		pass

	try:
		f = open(glpsol_op, 'r')
		for l in f:
			if 'Status' in l:
				glpk_status = Status.UNSAT if 'UNDEFINED' in l else Status.SAT
	except:
		pass

	result = ""
	if glpk_status is None:
		result += "%sGLPK%s failed to run."% (bcolors.FAIL, bcolors.ENDC)
	if marabou_status is None:
		result += "%sMarabou%s failed to run."% (bcolors.FAIL, bcolors.ENDC)
	if marabou_status is Status.PARSE_FAILURE:
		result += "%sMarabou%s failed to parse mps file." % (bcolors.WARNING, bcolors.ENDC)
	if result:
		return result

	if glpk_status == marabou_status:
		return "%ssame%s. Both: %s" % (bcolors.OKGREEN, bcolors.ENDC, marabou_status.name)
	else:
		return "%sdifferent%s. Marabou: %s, Glpk: %s" % (bcolors.FAIL, bcolors.ENDC, marabou_status.name, glpk_status.name)
