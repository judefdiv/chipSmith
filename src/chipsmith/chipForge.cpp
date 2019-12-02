/**
 * Author:      Jude de Villiers
 * Origin:      E&E Engineering - Stellenbosch University
 * For:         Supertools, Coldflux Project - IARPA
 * Created:     2019-12-02
 * Modified:
 * license:
 * Description: Creates the GDS chip
 * File:        chipForge.cpp
 */

#include "chipsmith/chipForge.hpp"

/**
 * [forgedChip::importData description]
 * @param  lefFileName [The LEF file to be imported]
 * @param  defFileName [The DEF file to be imported]
 * @param  conFileName [The toml config file to be imported]
 * @return         [0 - All good; 1 - Error]
 */

int forgedChip::importData(const string &lefFileName, const string &defFileName, const string &conFileName){

  this->lefFile.importFile(lefFileName);
  // lefFile.to_str();
  this->defFile.importFile(defFileName);
  defFile.to_str();

  // checking what cells are used
  bool fFlag;
  for(auto &itComp: this->defFile.comps){
    fFlag = false;
    for(auto &itList: this->usedGates){
      if(!itComp.getCompType().compare(itList)){
        fFlag = true;
        break;
      }
    }
    if(!fFlag){
      this->usedGates.push_back(itComp.getCompType());
    }
  }

  // cout << "Used gates:" << endl;
  // for(auto &itList: this->usedGates){
  //   cout << itList << endl;
  // }

  // Config file
  const auto mainConfig  = toml::parse(conFileName);
  this->gdsFileLoc = toml::get<map<string, string>>(mainConfig.at("GDS_CELL_LOCATIONS"));
  this->lef2gdsNames = toml::get<map<string, string>>(mainConfig.at("GDS_MAIN_STR_NAME"));

  return 0;
}


/**
 * [forgedChip::genGDS - Creates the GDS file]
 * @param  gdsFileName [The GDS file to be created]
 * @return         [0 - All good; 1 - Error]
 */

int forgedChip::genGDS(const string &gdsFileName){



  this->importGates();
  this->placeGates();
  // gdsBegin(FileName);

  // GenCells();
  // GenNets();
  // GenComp();

  // gdsStrStart("StrName");
  //   gdsSREF("SPECIALNETS_track", false, 1, 0, 0, 0);
  //   gdsSREF("NETS_track", false, 1, 0, 0, 0);
  //   gdsSREF("SPECIALNETS_via", false, 1, 0, 0, 0);
  //   gdsSREF("NETS_via", false, 1, 0, 0, 0);
  //   gdsSREF("Components", false, 1, 0, 0, 0);
  //   gdsSREF("Components_Pins", false, 1, 0, 0, 0);
  // gdsStrEnd();

  // gdsEnd();

  gdsSTR GDSmainSTR;

  GDSmainSTR.name = "chipSmith";
  GDSmainSTR.SREF.push_back(drawSREF("Components", 0, 0));
  // arrSTR.back().SREF.push_back(drawSREF("Nets", 0, 0));

  gdsFile.setSTR(GDSmainSTR);
  gdsFile.write(gdsFileName);

  return 0;
}

/**
 * [forgedChip::placeGates - Reads the locations of the gates in the DEF file and places them in the gate file]
 * @return [0 - All good; 1 - Error]
 */

int forgedChip::placeGates(){
  gdsSTR GDSdefSTR;
  GDSdefSTR.name = "Components";

  for(auto &itComps: defFile.comps){
    GDSdefSTR.SREF.push_back(drawSREF(this->lef2gdsNames[itComps.getCompType()],
                                      itComps.getCorX() * 10,
                                      itComps.getCorY() * 10));
  }

  gdsFile.setSTR(GDSdefSTR);

  return 0;
}

/**
 * [forgedChip::importGates - Defines the GDS structures for all the gates]
 * @return [0 - All good; 1 - Error]
 */

int forgedChip::importGates(){
  cout << "Defining gate structures." << endl;
  map<string, string>::iterator itName;

  int mactoIndex;
  gdsSTR GDSlefSTR;

  for(const auto &itList: this->usedGates){

    // making sure that the gate in the def file is defined in the config file
    itName = this->lef2gdsNames.find(itList);
    if(itName == this->lef2gdsNames.end()){
      cout << "Gate: \"" << itList << "\" is missing from the config file" << endl;
      // return 1;
    }

    itName = this->gdsFileLoc.find(this->lef2gdsNames[itList]);
    if(itName != this->gdsFileLoc.end()){
      cout << "Importing GDS: " << itName->second << endl;
      gdsFile.importGDSfile(itName->second);
    }
    else{
      // lef file import
      mactoIndex = -1;
      for(unsigned int i = 0; i < lefFile.macros.size(); i++){
        if(!lefFile.macros[i].name.compare(itList)){
          mactoIndex = i;
          break;
        }
      }
      if(mactoIndex != -1){
        cout << "Importing LEF: " << itList << endl;
        GDSlefSTR.name = itList;
        GDSlefSTR.BOUNDARY.clear();
        GDSlefSTR.BOUNDARY.push_back(draw2ptBox(200,
                                                0,
                                                0,
                                                this->lefFile.macros[mactoIndex].getSizeX() * 1000,
                                                this->lefFile.macros[mactoIndex].getSizeY() * 1000));

        gdsFile.setSTR(GDSlefSTR);
      }
      else{
        cout << "Missing GDS and LEF definition for gate: \"" << itList << "\"" << endl;
        return 1;
      }
    }


  }

  cout << "Importing the gates GDS files, done." << endl;
  return 0;
}