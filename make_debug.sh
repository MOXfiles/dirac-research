#! /bin/sh
#
#   Build a debuggable, profilable version of the encoder
#
g++ -I. -lxparam -g -pg -O3 -march=athlon-xp -ffast-math -o debug_encoder encoder/encmain.cpp libdirac_encoder/*.cpp libdirac_common/*.cpp libdirac_motionest/*.cpp
#
#   Build a debuggable, profilable version of the decoder
#
g++ -I. -lxparam -g -pg -O3 -march=athlon-xp -ffast-math -o debug_decoder decoder/decmain.cpp libdirac_decoder/*.cpp libdirac_common/*.cpp libdirac_motionest/*.cpp
#
# END
