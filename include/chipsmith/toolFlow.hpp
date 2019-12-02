/**
 * Author:      Jude de Villiers
 * Origin:      E&E Engineering - Stellenbosch University
 * For:         Supertools, Coldflux Project - IARPA
 * Created:     2019-11-17
 * Modified:
 * license:
 * Description: All the chipSmith tools get executed here
 * File:        toolFlow.hpp
 */

#include <string>
#include <iostream>

#include "chipsmith/ParserLef.hpp"
#include "chipsmith/ParserDef.hpp"
#include "chipsmith/chipForge.hpp"

using namespace std;

int forgeChip(const string &lefFileName,
              const string &defFileName,
              const string &gdsFileName,
              const string &conFileName);
