/**
 * Author:      Jude de Villiers
 * Origin:      E&E Engineering - Stellenbosch University
 * For:         Supertools, Coldflux Project - IARPA
 * Created:     2020-04-21
 * Modified:
 * license:
 * Description: Creates and populates the GDS chip
 * File:        chipFill.hpp
 */

#ifndef chipfill
#define chipfill

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <cmath>
#include "toml/toml.hpp"
#include "chipsmith/ParserLef.hpp"
#include "chipsmith/ParserDef.hpp"
#include "gdscpp/gdsCpp.hpp"

using namespace std;

int constrain(int inVal, int lowerLimit, int upperLimit);

class chipSmith{
  private:
    string name;

    lef_file lefFile;
    def_file defFile;

    gdscpp gdsF;

    vector<string> usedGates;

    map<string, string> gdsFileLoc;
    map<string, string> lef2gdsNames;
    map<string, string> gdsFillFileLoc;

    // map<string, vector<int>> cellSize;
    vector<vector<vector<bool>>> grid;
    // grid[0] - All; grid[n] - M_n;

    bool fillEnable = true;
    unsigned int gateHeight = 0;
    float PTLwidth = 0;
    vector<int> fillCor;
    unsigned int gridSize = 0;
    map<string, int> GateBiasCorX;

    // Grid limits (size of the grid)
    unsigned int gridLX = 0;
    unsigned int gridLY = 0;

    map<string, vector<int>> cellSizes;

    int importGates();
    int importFill();
    int placeGates();
    int placeNets();
    int placeFill();
    int placeBias();

  public:
    chipSmith(){};
    ~chipSmith(){};

    int importData(const string &lefFileName,
                   const string &defFileName,
                   const string &conFileName);

    int toGDS(const string &gdsFileName);
};

#endif