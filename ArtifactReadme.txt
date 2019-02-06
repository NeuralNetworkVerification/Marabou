*** Marabou CAV Artifact, February 2019 ***

Welcome to the Marabou artifact. This file contains
(i) information on the Marabou code and the various folders
contained within the artifact; and (ii) instructions for using the
tool and for running the experiments described in the paper.

The username and password for the virtual machine are: cav / ae
All the relevant files and directories are under: /home/cav/marabou


(i) Information regarding the Marabou code
------------------------------------------

[This part of the document assumes some familiarity with the concepts
described in the paper.]

The main components of the Marabou tool are:

1. Input parsers and interfaces for feeding queries into the tool
2. An InputQuery class that internally represents the query to be solved
3. A preprocessor that simplifies an InputQuery object.
4. An Engine class that orchestrates the solution of the query, by invoking
   the following additional components:

    5. A Simplex core, in charge of satisfying the linear constraints of
       the query
    6. PiecewiseLinearConstraint objects, in charge of representing and
       satisfying the non-linear constraints of the query
    7. An SMT core in charge of case splitting and backtracking
    8. Deduction engines, in charge of learning new facts and pruning the
       search space

9. A Divide-and-Conquer strategy that breaks down an input query into
   sub-queries that can be solved more efficiently, sequentially
   or concurrently.

Below we elaborate on each of these components. The interested reader may
look at the code comments for additional information, or also look at the
unit tests that accompany many of the classes for additional insight (these
can be found under the various "tests" folders).


1. Input Parsers [maraboupy folder, src/input_parsers, regress folder]

An input to Marabou is called a query. Typically, a query consists of
a neural network and some property concerning inputs and outputs. The
goal of Marabou is then to either find an input to the network such that
the input and its matching output satisfy the property, or declare that
no such input exists.

With the appearance of multiple deep neural network (DNN) verification
tools and techniques over the last couple of years, diverse input
formats have emerged, making it difficult to compare tools and run
them on queries provided by others. Tools are often proof-of-concept
implementations, and support specific benchmarks in a proprietary
format. As part of our effort to make the Marabou tool accessible to
many users, we added support for several input formats, described
below (instructions on using these formats are given in the second
part of this file):


   (a) The native Marabou C++ format:
       In this format the user compiles a query into Marabou using its
       internal C++ classes and constructs. We envision that this format
       will be most useful to users trying to integrate Marabou into
       another program.

   (b) The ACAS Xu format:
       The ACAS Xu format is a textual format, in which the deep neural
       network controllers for the Airborne Collision Avoidance System
       for Unmanned Aircraft are stored. Several verification approaches
       have been tested on these networks, and so supporting this format,
       even just for comparison purposes, is useful. When using this format,
       a user needs to run the Marabou executable, and provide as command
       line parameters a path to the ACAS Xu network in question. The
       property to be checked is provided as an additional, separate file,
       and is encoded in a simple, textual file.

       The parser in charge of handling inputs in this format can be
       found under the src/input_parsers folder.

   (c) The Berkeley format:
       This format is similar to the ACAS Xu format, in the sense that
       the networks are provided as simple, textual files. The main
       difference is that in the ACAS Xu format networks are described
       by their weight matrices; whereas in the Berkeley format, they are
       described as a set of equations. This makes it a little easier
       to manipulate them manually.

       The parser in charge of handling inputs in this format can also
       be found under the src/input_parsers folder.

   (d) The Python format:
       This interface provides a python interface, through which a
       user can provide the network and property in question. Most
       importantly, this interface can process networks stored in the
       common TensorFlow format, and seamlessly pass them into Marabou.

       The parser in charge of handling inputs in this format is found
       in the various files under the maraboupy directory.


2. InputQuery [src/Engine/InputQuery.{h,cpp}]

Each of the various input methods described above eventually produces an
InputQuery object: a single object that describes the neural network being
verified, and the property in question. The main fields of an InputQuery are:

  (a) Variables:
      A neural network is encoded by representing each neuron by one or
      more variables, and then trying to find a satisfying assignment for these
      variables. A satisfying assignment then corresponds a legal evaluation
      of the original network. Additional constraints on these variables express
      the property being verified.

      The input query maintains the number of variables in use, as well as
      initial lower and upper bounds for each of them.

  (b) Equations:
      The InputQuery also maintains a set of equations involving the variables.
      These can express, e.g., the weighted sum operations of the neural
      network.

      Variables and equations together can also capture properties being
      verified: for example, if a neural network has two output nodes n1
      and n2 and we wish to check whether it is possible that n2 > n1, we might
      represent n1 and n2 by variables x1 and x2, and also add the equation
      y = x2 - x1, for a fresh variable y. We would then set a lower bound 0 on
      y and have the tool look for a satisfying assignment.

  (c) Piecewise-Linear constraints (abbreviated pl-constraints):
      These piecewise-linear constraints represent the network's activation
      functions, or any piecewise-linear function that appears as part of the
      property being verified (e.g., some properties involve absolute values as
      part of L1-norm constraints. As absolute values are piecewise-linear
      functions, they can be encoded as pl-constraints).

      The InputQuery class does not assume the pl-constraints are any particular
      function; instead, it only assumes they inherit from the abstract
      PiecewiseLinearConstraint interface. This afford flexibility in case
      Marabou needs to be extended with additional kinds of pl-constraints
      in the future.

  (d) Network-level reasoning:
      The aforementioned ingredients of an input query (variables, equations and
      bounds) reduce the verification problem into a constraint solving problem,
      and do not keep a global overview of the network (e.g., its topology).
      However, network level reasoning is an important ingredient that can help
      in solving the query, e.g. by deducing new facts that can prune the search
      space. To support network level reasoning, an input query also stores the
      network's topology. Currently this information is stored within the
      SymbolicBoundTightener object stored as part of the input query. Making
      this infrastructure more general, so that it can easily be extended with
      additional kinds of network level reasoning, is a work in progress.


3. Preprocessor [src/Engine/Preprocessor.{h,cpp}]

In modern constraint solvers (e.g. SAT, SMT and LP solvers), preprocessing is a
very important step that can improve performance dramatically. In Marabou, the
Preprocessor class performs a preprocessing step that attempts to simplify the
user-provided InputQuery object.

Specifically, the following preprocessing steps are applied until saturation:

  (a) Bound tightening using equations:
      If a user provides an equation x = y + z and states the bounds

      -10 <= x <= 10, 0 <= y <= 1, 1 <= z <= 2

      this step will deduce tighter bounds for x: 1 <= x <= 3

  (b) Bound tightening using pl-constraints:
      If a user provides the constraint x = max( y, z ) and the bounds

      -10 <= x <= 10, 1 <= y <= 1.5, 0.5 <= z <= 2

      this step will deduce tighter bounds for x: 1 <= x <= 2

  (c) Variable elimination:
      Any variable that can be replaced with a constant or merged with another
      variable is removed; e.g.

      x = y + z and z = 5 is replaced with x = y + 5,

      and

      x = y + z and y = z is replaced with x = 2y

This preprocessing can remove many equations, variables and pl-constraints that
become obsolete, thus improving overall performance. In the future we plan to add
additional preprocessing steps that are common in the LP literature, e.g. in order
to improve the numerical properties of the input query.


4. Engine [src/Engine/Engine.{h,cpp}]

The Engine class is the main class of Marabou. It receives an input query
from one of the input parsers, invokes the preprocessor to simplify it, and then
attempts to solve it. It can terminate when a satisfying solution to the query
has been found, when the query is determined unsatisfiable, or if it receives
an external signal asking to stop (e.g., due to a timeout).

The Engine's main loop is in the solve() function. It handles various aspects
of the solving process, but the main algorithm is roughly as follows (the
first applicable step is applied):

   (a) If a piecewise-linear constraint had been violated and fixed more
       than a certain number of times, perform a case split on that constraint.
       [line 164 in Engine.cpp]

   (b) If the problem has become unsatisfiable, e.g. because for some
       variable a lower bound has been deduced that is greater than its
       upper bound, undo a previous case split (or return UNSAT if no
       such case split exists).
       [line 176 in Engine.cpp]

   (c) If there is a violated linear constraint, perform a simplex
       step.
       [line 223 in Engine.cpp]

   (d) If there is a violated piecewise-linear constraint, attempt
       to fix it.
       [line 207 in Engine.cpp]

   (e) All constraints are satisfied - return SAT.
       [line 202 in Engine.cpp]

Apart from these main steps, the engine also triggers deduction steps,
both at the neuron level and at the network level, according to certain
heuristics.
[e.g., lines 170, 211-212, and 217 in Engine.cpp]

In addition, the Engine also checks the numerical stability of the current
equations, and if they have become degraded it initiates a restoration process.
(Numerical instability is typically caused by performing floating point division
by small fractions as part of the simplex loop).
[lines 132-148 in Engine.cpp]


5. The Simplex core [src/engine/Tableau.{h,cpp} and dependent classes:
       	             src/engine/CostFunctionManager.{h,cpp},
     	             src/engine/ProjectedSteepestEdge.{h,cpp},
                     src/basis_factorization folder]

The simplex core is the part of the system in charge of making the
variable assignment satisfy the linear constraints. It does so by implementing
``phase one'' of the simplex algorithm - i.e., the part of simplex that finds
a feasible solution, without optimizing an objective function.

In each iteration, the simplex core changes the assignment of some variable x,
and consequently the assignment of any variable y that is connected to x by
a linear equation. Selecting x and determining its new assignment value is
performed using standard techniques. Specifically, we implemented the
revised simplex method in which the various linear constraints are kept in
implicit matrix form, and for variable selection we implemented the
steepest-edge and Harris' two-pass ratio test selection strategies.

Creating an efficient simplex solver is a complex task, which in other
tools is often delegated to external solvers such as GLPK or Gurobi.
Our motivation for implementing a proprietary solver in Marabou was twofold:
first, we observed that in the Reluplex tool, the repeated translation of
queries into GLPK and extraction of results from GLPK was a limiting
factor on performance; and second, that a black box simplex solver did
not afford the flexibility we needed in the context of DNN verification.
For example, in a standard simplex solver, variable assignments are typically
pressed against their upper or lower bounds, whereas in the context of a DNN
other assignments might be needed to satisfy the non-linear constraints.
Another example is in performing deduction steps that are crucial for
efficiently verifying a DNN, and whose effectiveness might depend on the
internal state of the simplex solver.

Explaining in greater detail how the simplex core works requires some
familiarity with the simplex algorithm, and so we don't elaborate on this here.
The main class is the Tableau class, and it internally uses other classes
in charge of various aspects of the algorithm such as variable selection
strategies and basis factorization. There exists many flavors of simplex,
and so we adhered to a modular design here - so that the various components
may be replaced or updated as needed.


6. The Piecewise-Linear Constraints [src/engine/PiecewiseLinearConstraint.{h,cpp},
                                     src/engine/ReluConstraint.{h,cpp},
				     src/engine/MaxConstraint.{h,cpp}]

Marabou is designed to support arbitrary piecewise-linear constraints that
may appear in neural networks or in properties being verified. The design
itself is modular and generic, and currently two concrete functions are supported:

   (a) the Rectified Linear Unit (ReLU) function, defined by
       y = ReLU(x) <--> y = max(0,x)

   (b) The Max function defined by y = max( x1, ... , xn )
       for an arbitrary number of operands.

Of course, other functions may be represented using these functions
(e.g., |x| = max(x, -x)), and one may even be represented in terms of the other,
e.g. max(x,y) = ReLU(x-y) + y. However, we observed that supporting these
functions natively allows better optimization and performance, and so we
support arbitrary constraints.

As we previously described, the Engine's main loop dedicates some of its
iterations to satisfying any currently violated pl-constraints. If such a
constraint exists, the Engine asks that constraint to suggest changes to
the assignment that would cause it to be satisfied, and then performs one
of these changes. Alternatively, in order to guarantee eventual termination,
if the Engine detects that a certain constraint is repeatedly not satisfied,
it will perform a case split on that constraint (via the SMT core).
This technique guarantees soundness and completeness; the proof is a simple
extension of the proof given in the Reluplex CAV'17 paper and is omitted
here.

Implementation-wise, piecewise-linear constraints are represented by objects
of classes that inherit from the PiecewiseLinearConstraint abstract class.
PiecewiseLinearConstraint defines the interface that each supported
piecewise-linear constraint needs to support. Currently the two supported
instances are the ReLU and Max function, but the design is modular in a sense
that new constraint types can easily be added. Some of the key interface methods
that a piecewise-linear constraint needs to provide are:

   (a) satisfied(): the constraint object needs to answer whether or not it
       is satisfied given the current assignment. For example, invoking this
       method for a ReLU constraint y = ReLU(x) given assignment x = y = 3
       would return true, whereas assignment x = -5, y = 3 would return false.

   (b) getPossibleFixes(): if the constraint is not satisfied by the current
       assignment, this method needs to return proposed changes to the assignment
       that would correct the issue. For example, for x = -5 and y = 3 the ReLU
       constraint from before might propose two possible fixes to the assignment:
       either assign x := 3 or y := 0, as either option would satisfy y = ReLU(x).

   (c) getCaseSplits(): this method asks the piecewise-linear constraint C to
       return the list of linear constraints c_1,...,c_n, such that C is
       equivalent to the disjunction c_1 \/ ... \/ c_n. For example, when this
       function is invoked for a Max constraint y = max(x_1, x_2), it would
       return the linear constraints c_1 = ( y = x_1 /\ x_1 >= x_2 ) and
       c_2 = ( y = x_2 /\ x_2 >= x_1 ). These constraints satisfy the requirement
       that the original constraint y = max(x_1, x_2) is equivalent to c_1 \/ c_2.

   (d) getEntailedTightenings(): as part of Marabou's effort of deducing tighter
       variable bounds, piecewise-linear constraints are repeatedly informed of
       changes pertaining to variables that they affect - and in particular,
       information regarding the lower and upper bounds of these variables. The
       getEntailedTightenings() function is used to query the constraint for tighter
       variable bounds, which may be deduced from the current information. For
       example, suppose a ReLU constraint y = ReLU(x) is informed of the upper
       bounds x <= 5 and y <= 7; in this case, invoking the aforementioned method
       would return the tighter bound y <= 5. This bound, in turn, may be used by
       Marabou for further deduction steps.

Implementing some of these interface methods is mandatory, in order to guarantee
soundness and termination, whereas other methods are optional. For example, a
constraint must implement the the satisfied() and getCaseSplits() methods, but
may leave the getPossibleFixes() and getEntailedTightenings() methods empty.


7. The SMT Core [src/Engine/SmtCore.{h,cpp}]

The SMT core is the system component in charge of case splits. If we detect
that a certain pl-constraint, e.g. x = max(y,z) is repeatedly not satisfied
by the values assigned to x, y and z by the simplex core, the SMT core may
decide that this constraint needs to be split on. When that happens, the
constraint in question provides the linear constraints with which it can be
replaced - in this case, (x=y and x>=z) OR (x=z and x>=y). The SMT core then
asserts one of the disjuncts each time, and if contradiction is reached it
backtracks and asserts another disjunct. It takes care of storing and restoring
the various system components that are affected.

Also, the SMT core has several heuristics for deciding which constraints should
be split on and when - some of which are controlled by the system configuration.
We omit the details here.


8. The Deduction Engines [src/engine/RowBoundTightener.{h,cpp},
                          src/engine/ConstraintBoundTightener.{h,cpp},
                          src/engine/SymbolicBoundTightener.{h,cpp}]

In the usual manner of SMT solvers, Marabou's solving process is comprised of
search and deduction. The search portion is managed by the SMT and Simplex
cores, whereas the deduction steps are managed by various bound tighteners:
components whose goal is to deduce new facts about variable bounds, that can
curtail the search space. The motivation is that tighter variable bounds can
cause pl-constraints to become "stuck" in one of their linear phases, which
allows Marabou to replace them with linear constraints. Since linear constraints
are much cheaper to handle than piecewise linear ones, the overall performance
improves.

Deduction steps are performed throughout the solution phases, according to
certain heuristics. Currently there are three kinds of deductions, two of
which we already mentioned when we discussed the preprocessor:

  (a) Bound tightening using equations:
      As part of the simplex process, Marabou repeatedly encounters new
      equations after each simplex iteration. These equations can be used
      for bound tightening. For example, if the equation x = y + z is
      encountered, and the current variable bounds are

      -10 <= x <= 10, 0 <= y <= 1, 1 <= z <= 2

      this step will deduce tighter bounds for x: 1 <= x <= 3

      This kind of tightening is managed by the RowBoundTightener class.

  (b) Bound tightening using pl-constraints:
      The pl-constraints are repeatedly consulted, to see if they can deduce
      tighter bounds. For example, for the constraint x = max( y, z ) and
      the bounds

      -10 <= x <= 10, 0 <= y <= 1.5, 1 <= z <= 2

      this step will deduce tighter bounds for x: 0 <= x <= 2

      The set of pl-constraints is mostly static. However, this step is repeated
      because bound tightening operations have a cascading nature: a bound
      tightened, e.g., using method (a) may lead to tighter bounds in
      method (b).

      This kind of tightening is managed by the ConstraintBoundTightener class.

These two preprocessing steps are done at the equation level (method a) or at
the pl-constraint level (method b). A third method, called symbolic bound
tightening, uses the network topology to perform bound tightening in yet another
way. For more details on symbolic bound tightening, see the paper and the
references therein. Symbolic bound tightening is managed by the
SymbolicBoundTightener class.


9. A Divide-and-Conquer Strategy [maraboupy/DnC.py
                                  maraboupy/DnCSolver.py
                                  maraboupy/DnCParallelSolver.py
                                  maraboupy/NumTasksDecider.py]

The divide-and-conquer(DnC) strategy is implemented as a wrapper around the
Marabou core. The main class of the DnC mode is DnC.py.

When tasked with a query, the DnC mode repeatedly breaks it down  into simpler
sub-queries, which are solved independently. Concretely, The DnC mode first
tries to solve the original query with a short timeout T (e.g., 5 seconds). If
the query is not solved, the DnC solver breaks the query down into N sub-queries
and tries to solve each of those sub-queries with a higher timeout m * T, where
m is larger than 1. If a timeout occured again for certain sub-query \phi, the
DnC solver will further break it down further with a even higher timeout m * m * T.
The process keeps going, until all "leaf"-sub-queries are proven UNSAT, or
any sub-query is proven SAT, at which point the satisfiability of the original query
can be deduced.
The algorithm described above can be found in the DnCParallelSolver.py,
which implements a version of the algorithm that supports parallel execution.
DnCSolver.py contains the DnCSolver class that calls the methods in
DnCParallelSolver.py.

The sub-queries are created by bisecting the interval of one of the input neuron.
NumTasksdecider.py contains different heuristics to decide which interval to
bisect. The default heuristic involves randomly sampling points along each input
dimension and compute the sum of the differences of the activation patterns
between adjacent points. An activation pattern of an input and a neural network
is a bit-vector where each bit represents whether a neuron in the network is active
or not given the input. Intuitively, the heuristic above computes the influence
of an input interval on the activation function of the network, and bisect the
interval with the highest influence.


Additional pieces of the code:

  - The "common" folder contains utility classes

  - The "tools" folder contains the CxxTest tool for unit testing, and also the
    boost library for command line parsing




(ii) Using the tool and running the experiments
-----------------------------------------------

1. Compilation and Tests

The artifact contains the code for Marabou and its various interfaces,
as described in the paper and in this readme file. Compiling the code
is performed as follows:

      - Open a terminal (ctrl + alt + t)
      - Navigate to the main dir:
        cd marabou
      - Run "make":
        make

The code in the provided VM has already been compiled, so the re-compilation
should end fairly quickly. Still, it will also run the unit tests, which are
rerun with every invocation of "make". To recompile from scratch, run

      - make clean && make

In addition, it is also possible to run a small set of regression tests, by
invoking

      - make regress

After a successful compilation, the Marabou executable, marabou.elf, can be
found under the bin directory.

2. The Various Interfaces

   (a) The C++ interface
       Marabou queries can be encoded by writing code that populates Marabou's
       InputQuery object, and compiling it against Marabou. This approach may
       prove useful, e.g., if Marabou is used within another framework, or for
       automation purposes.

       An example of this interface can be found under cpp_interface_example.
       In order to compile and run that example, run

       - cd cpp_interface_example
       - make
       - ./example.elf

       To see the encoding, open the main.cpp file in the aforementioned folder.
       The code therein populates the various fields of an InputQuery object
       that describe a verification query: the equations, piecewise-linear
       constraints and variabe bounds. It then invokes the Marabou engine,
       obtains a SAT answer, and prints out the satisfying assignment.

       The comments in the main.cpp file describe the encoded network.

       Feel free to change some of the constraints, recompile the code and run
       it again to obtain different results. You might try to, e.g., change
       the bounds of variable x5 (lines 48-49) from

       	   0.5 <= x5 <= 1

       to
	     6 <= x5 <= 7

       and see that the problem becomes UNSAT.

       Examples of additional queries of this kind, including some that use
       the Max pl-constraint, can be found under the "regress" folder, e.g. in
       the file regress/max_feasible_1.h.

   (b) Marabou executable
       Marabou can be invoked by running the executable file created under the
       bin folder, and supplying it with a neural network and a property. For
       example, try running:

       - ./bin/marabou.elf executable_example/acas.nnet executable_example/property.txt

       This tells Marabou to find an assignment to the network described in the
       nnet file (in this case, one of the ACAS Xu networks), that satisfies
       the property described in property.txt. If a satsifying assignment is
       found it is printed out.

       The property files are encoded in a simple textual format, where the x
       variables represent the networks inputs, the y variables represent its
       outputs, and only linear constraints are allowed.

       The particular property file in the example limits the range of the
       network's 5 inputs, and requires that the first output, y0, be the minimal
       of the 5 outputs. For an ACAS network, this means the network advises
       that the drone continue to fly straight, i.e. issuses a COC
       (clear-of-conflict) advisory.

       The initial query is SAT, i.e. there exists an input within the specified
       range for which the network's output is COC. Feel free to change some of
       the constraints and see what happens. For example, changing the first
       two lines in the file into:

           x0 >= -0.30
           x0 <= -0.29

       Will make the query UNSAT. This is because input x0 represents the
       distance to an intruder drone, and the change means that the
       intruder is very close - and so the network will never advise COC.


   (c) Python interface
       The Python interface resides in the maraboupy folder, and can be compiled
       by running

       - cd maraboupy && make

       (it is already precompiled in the artifact).

       Running it is then performed by writing a Python script that prepares an
       input query and invokes the engine. This Python script will usually
       define the property to be checked explicitly, and will use a parser
       for reading a neural network stored in a file.

       A couple of examples reside within the python_examples directory. To
       run the first, use:

       - cd python_examples
       - python3 ./generate_adversarial_example.py

       In this example, we read a neural network stored in a
       TensorFlow protobuff file (in this case, a small convolutional
       network for digit recognition), and search for adversarial
       inputs for that network. An adversarial input is a misclassified
       input, obtained by slightly perturbing a properly classified
       input. Here, we start from an image correctly classified as the
       digit 1 (stored in the inputs/145.npy file), and look for
       slight perturbations that would cause it to be misclassified as
       the digit 3. The allowed amount of perturbation is delta =
       0.08, measured in L-infinity distance. Marabou returns an UNSAT
       answer, implying that no adversarial inputs exists within
       distance delta of our starting image.

       Feel free to change the query, e.g. by increasing delta (line
       32) to 0.1. This tells Marabou to search for adversarial inputs
       that are a little more distant, and indeed it is then able to find
       an adversarial input.

       Another example, in which the network is not stored as a
       TensorFlow model but is instead encoded directly through the
       Python interface is available in the simple.py file. To run it,
       perform:

       - cd python_examples (no need to repeat if already in this dir)
       - python3 ./simple.py

       The comments inside the file describe the network and
       property. You can see the network's equations, activation
       functions and variable bounds encoded directly into an
       InputQuery object and then passed into the solver.

       As usual, feel free to change some of the constraints and rerun
       the example. You might try to, e.g., change the bounds of
       variable x5 (lines 33-34) from

       	   0.5 <= x5 <= 1

       to
	     6 <= x5 <= 7

       and see that the problem becomes UNSAT.


3. Using the Divide-and-Conquer mode

To use the devide-and-conquer (DnC) mode, the python API must be installed.
The DnC mode accepts the same format of property description as the Marabou
engine. For now, it requires the availability of both the protobuf format and
the nnet format of the network with the same suffix. The conversion between
protobuf and nnet can be easily conducted using the following scripts:
https://github.com/sisl/NNet/tree/master/scripts

The network is passed to the DnC solver using the flag -n, and the property
is passed to it using the flag -q.
In addition to these two mandatory flags, there are several additional flags.
Call
      - python3 ./maraboupy/DnC.py --help
to see the full list of options.

As an example of using the DnC mode, try running:
      - python3 ./maraboupy/DnC.py -n executable_example/acas -q executable_example/property.txt -w 4 --log-file executable_example/log.txt 

This checks the property on the network "acas" using 4 cores and writing the
satisfying model as well as detailed statistics to executable_example/log.txt


4. Experiments described in the paper


The machines we used to run the experiments all had the following
specifications:
  - OS: Ubuntu 16.04.1 LTS
  - Cores: 8 (altough Reluplex itself is single-threaded)
  - RAM: 32 gigabyte
  - CPU: Intel(R) Xeon(R) CPU E5-2637 v4 @ 3.50GHz


TODO: scripts for running the experiments
TODO: complete logs




IMPORTANT NOTE:

Most of the experiments described in the paper take a long time to
run. Running them all sequently on a laptop will probably take a few
weeks (we ran the experiments in parallel, using several machines).
Below we mention some of the faster experiments - the reviewers may
wish to focus on them. For the longer experiments, we propose to let
Reluplex run for a while, and then look at the (partial) output in the
statistics file. For these cases, we have provided a complete set of logs
generated on our machines under the "complete_logs" folder; and the
reviewers may compare the prefix of the statistics logs that they've
obtained to those in our files, to see that they match. For example,
if a partial Reluplex log contains information up to iteration #5000,
the information for this iteration can be compared to that in the
complete_logs folder, to see that, e.g., that the number of explored
states match.
