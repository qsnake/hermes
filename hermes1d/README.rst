===================
Welcome to Hermes1D
===================

Hermes1D is an experimental C++ library for the solution of ordinary
differential equations (ODE) and one-dimensional partial differential equations
(PDE) with higher-order finite element methods (hp-FEM). In contrast to
traditional time-stepping ODE solvers, Hermes1D constructs the solution using a
variational principle. It starts from a weak formulation of the ODE/PDE problem
and allows the equations to be defined in a very general implicit
(vector-valued) form F(y, y', t) = 0. The approximation is a continuous,
piecewise-polynomial function defined in the entire interval (0, T). In
contrast to time-stepping schemes, the finite element approach makes it
possible to prescribe boundary conditions either at the beginning or at the end
of the time interval (combinations are possible for systems). The hp-FEM
discretization leads to a system of nonlinear algebraic equations that is
solved via the Newton's method or JFNK. Sphinx documentation  is still in
progress. Hermes1D comes with a free interactive online lab powered by UNR HPC
cluster.


License
=======

Hermes1D is distributed under the BSD license (it covers all files in the
Hermes1D repository unless stated otherwise), see the LICENSE file for a
copyright/license information and the AUTHORS file for the list of authors.


Compilation
===========

Install the required libraries first. If you are using a Debian-based system,
do:

$ apt-get install cmake g++

$ cmake .
$ make


Examples
========

$ cd examples/laplace
$ ./laplace
$ gnuplot
gnuplot> plot './solution.gp' w l


Documentation
=============

Install the required libraries first, if you are using a Debian-based system,
do:

$ apt-get install python-sphinx

then:

$ cd doc
$ make html
$ firefox _build/html/index.html


Python
======

python -c 'print "set(WITH_PYTHON yes)\n"' > CMake.vars
cmake .
make
cd examples/schroedinger
./schroedinger
