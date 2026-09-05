// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include <cassert>
#include <chrono>
#include <stack>
#include <map>
#include <filesystem>
#include <unordered_set>
//#include "D:\Simon projects\NeuralNetworks\nnet.h"
//#include "D:\Simon projects\NeuralNetworks\NNetProxy.h"

#define FAST_NET

#ifdef FAST_NET
#include "D:\Simon projects\NeuralNetworks\CheckersFast.h"
using NNet = chkf::net;
#else
#include "D:\Simon projects\NeuralNetworks\CheckersNet.h"
using NNet = chk::net;
#endif

#endif //PCH_H
