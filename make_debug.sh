#! /bin/sh
#
#   Build debuggable, profilable versions of the command-line encoder and decoder
#
#   Check for argument
#
if test x"$1" = "x"; then
    echo
    echo "usage: make_debug.sh architecture-to-target"
    echo
    echo "E.g.   make_debug.sh k8        # opteron CPU"
    echo "       make_debug.sh pentium4"
    echo "       make_debug.sh athlon-xp"
    echo
    echo "For full details on GCC's targets see \"info gcc\" - Invoking GCC"
    exit 1
fi

make CXXFLAGS="-g -pg -O3 -ffast-math -march=$1"
