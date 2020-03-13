/**
 * Author:      Jude de Villiers
 * Origin:      E&E Engineering - Stellenbosch University
 * For:         Supertools, Coldflux Project - IARPA
 * Created:     2019-12-02
 * Modified:
 * license:
 * Description: Creates the GDS chip
 * File:        chipForge.hpp
 */

#ifndef chipforge
#define chipforge

class def_component;
class def_net;

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include "toml/toml.hpp"
#include "chipsmith/ParserLef.hpp"
#include "chipsmith/ParserDef.hpp"
#include "gdscpp/gdsCpp.hpp"


using namespace std;


class forgedChip{
  private:
    string name;

    lef_file lefFile;
    def_file defFile;

    gdscpp gdsFile;

    vector<string> usedGates;

    map<string, string> gdsFileLoc;
    map<string, string> lef2gdsNames;
    map<string, string> gdsFillFileLoc;

    map<string, vector<int>> cellSize;
    vector<vector<vector<bool>>> grid;
    // grid[0] - All; grid[n] - Mn;

    // fill area, must rename variables
    bool fillEnable = true;
    int gridShiftX = 0;
    int gridShiftY = 0;
    int gridOffsetX = 0;
    int gridOffsetY = 0;
    int gridSizeX = 0;
    int gridSizeY = 0;


    vector<int> fillCor;
    unsigned int gridSize = 0;

    int importGates();
    int importFill();
    int placeGates();
    int placeNets();
    int placeFill();

  public:
    forgedChip(){};
    ~forgedChip(){};

    int importData(const string &lefFileName,
                   const string &defFileName,
                   const string &conFileName);

    int genGDS(const string &gdsFileName);
};

#endif