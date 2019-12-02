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

    int importGates();
    int placeGates();

  public:
    forgedChip(){};
    ~forgedChip(){};

    int importData(const string &lefFileName,
                   const string &defFileName,
                   const string &conFileName);

    int genGDS(const string &gdsFileName);
};

#endif