#! /bin/sh
#
#   Build debuggable, profilable versions of the command-line encoder and decoder
#
#   Chack for argument
#
if test x"$1" = "x"; then
    echo ;
    echo usage: make_debug.sh architecture ;
    echo ;
    echo where architecture is pentium4, opteron, or another valid valud for -march= ;
    echo ;
    exit 1 ;
fi
#
#   build encoder
#
g++ -I. -lxparam -g -pg -O3 -march=$1 -ffast-math -o debug_encoder encoder/encmain.cpp libdirac_encoder/*.cpp libdirac_common/*.cpp libdirac_motionest/*.cpp
#
#   build decoder
#
g++ -I. -lxparam -g -pg -O3 -march=$1 -ffast-math -o debug_decoder decoder/decmain.cpp libdirac_decoder/*.cpp libdirac_common/*.cpp libdirac_motionest/*.cpp
#
# END
