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
  cout << "Importing data." << endl;

  this->lefFile.importFile(lefFileName);
  // lefFile.to_str();
  this->defFile.importFile(defFileName);
  // defFile.to_str();

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
  const auto mainConfig = toml::parse(conFileName);
  this->gdsFileLoc      = toml::get<map<string, string>>(mainConfig.at("GDS_CELL_LOCATIONS"));
  this->lef2gdsNames    = toml::get<map<string, string>>(mainConfig.at("GDS_MAIN_STR_NAME"));
  this->gdsFillFileLoc  = toml::get<map<string, string>>(mainConfig.at("GDS_LOCATIONS"));


  /***************************************************************************
   ************************** Fill Parameters ********************************
   ***************************************************************************/

  const auto &Para = toml::find(mainConfig, "PARAMETERS");

  auto element = toml::find(Para, "fill");
  string gridToBe = toml::get<string>(element);
  if(!gridToBe.compare("Y")){
    this->fillEnable = true;
  }
  else if(!gridToBe.compare("N")){
    this->fillEnable = false;
  }
  else{
    cout << "Warning, \"" << gridToBe << "\" is not valid. There shall be no fill." << endl;
    this->fillEnable = false;
  }

  element = toml::find(Para, "gridOffsetX");
  this->gridShiftX = toml::get<int>(element);

  element = toml::find(Para, "girdOffsetY");
  this->gridShiftY = toml::get<int>(element);

  element = toml::find(Para, "fillCor");
  this->fillCor = toml::get<vector<int>>(element);

  element   = toml::find(Para, "gridSize");
  this->gridSize    = toml::get<int>(element);

  // this->gridOffsetX = this->fillCor[0];
  // this->gridOffsetY = this->fillCor[1];

  this->gridOffsetX = this->fillCor[0] + this->gridShiftX;
  this->gridOffsetY = this->fillCor[1] + this->gridShiftY;

  this->gridSizeX = (fillCor[2] - fillCor[0])/this->gridSize;
  this->gridSizeY = (fillCor[3] - fillCor[1])/this->gridSize;

  cout << "Grid size: " << this->gridSizeX << "x" << this->gridSizeY << endl;

  cout << "Defining grid." << endl;

  vector<bool> yFill;
  yFill.resize(this->gridSizeY, true);

  this->grid.resize(7);

  for(auto &foo: this->grid){
    foo.resize(this->gridSizeX, yFill);
  }

  cout << "Defining grid, done." << endl;

  /***************************************************************************
   ******************************* Cell Size *********************************
   ***************************************************************************/

  // this->cellSize = toml::get<map<string, vector<int>>>(mainConfig.at("GDS_CELL_SIZES"));

  for(auto const &itMac: this->lefFile.macros){
    this->cellSize.insert(pair<string,vector<int>>(itMac.name, {(int)itMac.sizeX, (int)itMac.sizeY}));
  }

  cout << "Sneak peak into cellSize" << endl;
  for(auto const& [key, val]: this->cellSize){
    cout << key << ":" << val[0] << "," << val[1] << endl;
  }

  cout << "Importing data, done." << endl;
  return 0;
}


/**
 * [forgedChip::genGDS - Creates the GDS file]
 * @param  gdsFileName [The GDS file to be created]
 * @return         [0 - All good; 1 - Error]
 */

int forgedChip::genGDS(const string &gdsFileName){
  gdsSTR GDSmainSTR;

  this->importGates();
  // if(this->fillEnable) this->importFill();
  this->importFill();
  this->placeGates();
  this->placeNets();
  if(this->fillEnable) this->placeFill();

  GDSmainSTR.name = "chipSmith";
  GDSmainSTR.SREF.push_back(drawSREF("Components", 0, 0));
  GDSmainSTR.SREF.push_back(drawSREF("Nets", 0, 0));
  GDSmainSTR.SREF.push_back(drawSREF("Vias", 0, 0));
  if(this->fillEnable) GDSmainSTR.SREF.push_back(drawSREF("Fill", -this->gridShiftX, -this->gridShiftY));
  if(this->fillEnable) GDSmainSTR.SREF.push_back(drawSREF("Fill", 0, 0));

  gdsFile.setSTR(GDSmainSTR);
  gdsFile.write(gdsFileName);

  return 0;
}

/**
 * [forgedChip::placeGates - Reads the locations of the gates in the DEF file and places them in the gate file]
 * @return [0 - All good; 1 - Error]
 */

int forgedChip::placeGates(){
  cout << "Placing gates." << endl;

  gdsSTR GDSdefSTR;
  GDSdefSTR.name = "Components";

  for(auto &itComps: defFile.comps){
    GDSdefSTR.SREF.push_back(drawSREF(this->lef2gdsNames[itComps.getCompType()],
                                      itComps.getCorX() * 10,
                                      itComps.getCorY() * 10));
  }

  gdsFile.setSTR(GDSdefSTR);

  cout << "Placing gates, done." << endl;

  return 0;
}

/**
 * [forgedChip::placeFill description]
 * @return [0 - All good; 1 - Error]
 */

int forgedChip::placeFill(){
  cout << "Placing fill." << endl;
  gdsSTR GDSfill;
  vector<gdsSTR> GDSfil;
  GDSfill.name = "Fill";
  GDSfil.resize(7);

  const vector<string> gdsFillName = {"fillAll", "fillM1", "fillM2", "fillM3", "fillM4", "fillM5", "fillM6"};

  /***************************************************************************
   ******************************* Fill All **********************************
   ***************************************************************************/
  /*
   * Fill the whole circuit
   */

  cout << "\tFilling the whole circuit." << endl;

  GDSfil[0].name = "FillAll";
  GDSfill.SREF.push_back(drawSREF("FillAll", 0, 0));

  // for(unsigned int i = 0; i < this->grid[0].size(); i++){
  //   for(unsigned int j = 0; j < this->grid[0][i].size(); j++){
  //     if(this->grid[0][i][j]){
  //       GDSfil[0].SREF.push_back(drawSREF("fillAll", (gridOffsetX * 1000) + (i*10000), (gridOffsetY * 1000) + (j*10000)));
  //     }
  //   }
  // }

  /***************************************************************************
   ****************************** Fill M4 & M6 *******************************
   ***************************************************************************/
  /*
   * Fill where there are no cells
   */

  cout << "\tFilling M4 & M6, no gates." << endl;

  GDSfil[4].name = "FillM4";
  GDSfil[6].name = "FillM6";
  GDSfill.SREF.push_back(drawSREF("FillM4", 0, 0));
  GDSfill.SREF.push_back(drawSREF("FillM6", 0, 0));

  for(const auto &comps: defFile.comps){

    int x_lower_limit = (comps.corX * 10) - (gridOffsetX *1000);
    int y_lower_limit = (comps.corY * 10) - (gridOffsetY *1000);
    int x_upper_limit = (comps.corX * 10) - (gridOffsetX *1000) + (this->cellSize[comps.compName][0] *1000);
    int y_upper_limit = (comps.corY * 10) - (gridOffsetY *1000) + (this->cellSize[comps.compName][1] *1000);

    x_lower_limit /= 10000;
    y_lower_limit /= 10000;
    x_upper_limit /= 10000;
    y_upper_limit /= 10000;

    // cout << "x_1,y_1 = " << x_lower_limit << "," <<  y_lower_limit << endl;
    // cout << "x_2,y_2 = " << x_upper_limit << "," <<  y_upper_limit << endl;

    for(unsigned int i = x_lower_limit; i < x_upper_limit; i++){
      for(unsigned int j = y_lower_limit; j < y_upper_limit; j++){
        this->grid[4][i][j] = false;
        this->grid[6][i][j] = false;
      }
    }

  }

  /***************************************************************************
   ******************************* Fill M2 ***********************************
   ***************************************************************************/
  /*
   * Fill where there are no vias
   */

  cout << "\tFilling M2, no vias." << endl;

  GDSfil[2].name = "FillM2";
  GDSfill.SREF.push_back(drawSREF("FillM2", 0, 0));

  int corX, corY;

  for(auto &itNet: this->defFile.nets){
    for(int i = 0; i < itNet.routes.size() -1; i++){
      corX = (itNet.routes[i].ptX.back() * 10) - (gridOffsetX * 1000);
      corY = (itNet.routes[i].ptY.back() * 10) - (gridOffsetY * 1000);
      corX /= 10000;
      corY /= 10000;
      this->grid[2][corX][corY] = false;
    }
  }

  cout << "Creating fill for Vias, done." << endl;

  /***************************************************************************
   ********************************* M1 & M3 *********************************
   ***************************************************************************/
  /**
   * Fill where there is no routing
   */

  cout << "\tFilling M1 & M3, no tracks." << endl;

  GDSfil[1].name = "FillM1";
  GDSfil[3].name = "FillM3";
  GDSfill.SREF.push_back(drawSREF("FillM1", 0, 0));
  GDSfill.SREF.push_back(drawSREF("FillM3", 0, 0));

  // metal1 metal2
  int corXto, corYto, corXfrom, corYfrom, metalLayer;

  for(auto &itNet: this->defFile.nets){
    for(auto &itRoute: itNet.routes){

      for(unsigned int i = 0; i < itRoute.ptX.size() -1; i++){
        if(!itRoute.LAYER.compare("metal1")){
          metalLayer = 1;
        }
        else if(!itRoute.LAYER.compare("metal2")){
          metalLayer = 3;
        }
        else{
          cout << "Fill is not sure what metal layer for routing..." << endl;
        }

        corXfrom   = (itRoute.ptX[i] * 10) - (gridOffsetX * 1000);
        corXto     = (itRoute.ptX[i+1] * 10) - (gridOffsetX * 1000);
        corYfrom   = (itRoute.ptY[i] * 10) - (gridOffsetY * 1000);
        corYto     = (itRoute.ptY[i+1] * 10) - (gridOffsetY * 1000);
        corXfrom  /= 10000;
        corXto    /= 10000;
        corYfrom  /= 10000;
        corYto    /= 10000;

        // if(corYfrom < 0)
        //   corYfrom = 0;
        // if(corYto < 0)
        //   corYto = 0;
        // if(corXfrom < 0)
        //   corXfrom = 0;
        // if(corXto < 0)
        //   corXto = 0;

        // if(gridSizeX <= corXfrom || gridSizeX <= corXto || gridSizeY <= corYfrom || gridSizeY <= corYto){
        //   // cout << "skipping" << endl;
        //   continue;
        // }

        // cout << "x:" << corXfrom << " -> " << corXto << endl;
        // cout << "Y:" << corYfrom << " -> " << corYto << endl;

        if((itRoute.ptX[i] - itRoute.ptX[i+1]) == 0){
          //vertical
          if(corYto > corYfrom){
            for(int j = corYfrom; j <= corYto; j++){
              this->grid[metalLayer][corXfrom][j] = false;
            }
          }
          else{
            for(int j = corYfrom; j >= corYto; j--){
              this->grid[metalLayer][corXfrom][j] = false;
            }
          }
        }
        else{
          //horizontal
          if(corXto > corXfrom){
            for(int j = corXfrom; j <= corXto; j++){
              this->grid[metalLayer][j][corYfrom] = false;
            }
          }
          else{
            for(int j = corXfrom; j >= corXto; j--){
              this->grid[metalLayer][j][corYfrom] = false;
            }
          }
        }
      }
    }
  }

  /***************************************************************************
   ******************************* Fill others *******************************
   ***************************************************************************/

  cout << "\tFilling M5?" << endl;

  GDSfil[5].name = "FillM5";
  GDSfill.SREF.push_back(drawSREF("FillM5", 0, 0));

  /***************************************************************************
   ******************************* Plot Grid *********************************
   ***************************************************************************/

  for(unsigned int i = 0; i < this->gridSizeX; i++){
    for(unsigned int j = 0; j < this->gridSizeY; j++){
      for(unsigned int k = 0; k < gdsFillName.size(); k++){
        if(k == 5){
          continue;
        }
        if(this->grid[k][i][j] == true){
          GDSfil[k].SREF.push_back(drawSREF(gdsFillName[k], (gridOffsetX * 1000) + (i*10000), (gridOffsetY * 1000) + (j*10000)));
        }
      }
    }
  }

  gdsFile.setSTR(GDSfill);
  gdsFile.setSTR(GDSfil);

  cout << "Placing fill, done." << endl;

  return 0;
}

/**
 * [forgedChip::placeNets - Reads the description of the nets in the DEf file and routes them in the GDS file]
 * @return [0 - All good; 1 - Error]
 */

int forgedChip::placeNets(){
  cout << "Routing nets." << endl;
  gdsSTR GDSroute;
  gdsSTR GDSvia;

  GDSroute.name = "Nets";
  GDSvia.name = "Vias";

  vector<int> corX;
  vector<int> corY;

  for(auto &itNet: this->defFile.nets){
    for(auto &itPath: itNet.routes){
      if(itPath.ptX.size() == 1){
        // Skips single point/dimension tracks, can be due to via placement
        continue;
      }

      corX.clear();
      corY.clear();

      for(unsigned int i = 0; i < itPath.ptX.size(); i++){
        corX.push_back(itPath.ptX[i] * 10);
        corY.push_back(itPath.ptY[i] * 10);
      }
      if(!itPath.LAYER.compare("metal1")){
        GDSroute.PATH.push_back(drawPath(10, 4500, corX, corY));
      }
      else if(!itPath.LAYER.compare("metal2")){
        GDSroute.PATH.push_back(drawPath(30, 4500, corX, corY));
      }
    }
  }

  // /**
  //  * VIAS
  //  */

  for(auto &itNet: this->defFile.nets){
    for(int i = 0; i < itNet.routes.size() -1; i++){
      GDSvia.SREF.push_back(drawSREF("ViaM1M3", itNet.routes[i].ptX.back() * 10, itNet.routes[i].ptY.back() * 10));
    }
  }

  this->gdsFile.setSTR(GDSroute);
  this->gdsFile.setSTR(GDSvia);

  cout << "Routing nets, done." << endl;

  return 0;
}

/**
 * [forgedChip::importFill - Defines the fill structures]
 * @return [0 - All good; 1 - Error]
 */

int forgedChip::importFill(){
  cout << "Defining fill structures." << endl;

  map<string, string>::iterator itLoc;

  for(itLoc = this->gdsFillFileLoc.begin(); itLoc != this->gdsFillFileLoc.end(); itLoc++){
    cout << "Importing GDS: " << itLoc->second << endl;
    gdsFile.importGDSfile(itLoc->second);
  }

  cout << "Defining fill structures, done." << endl;
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
        if(!GDSlefSTR.name.compare("PAD")){
          GDSlefSTR.BOUNDARY.push_back(draw2ptBox(10,
                                                  0,
                                                  0,
                                                  this->lefFile.macros[mactoIndex].getSizeX() * 1000,
                                                  this->lefFile.macros[mactoIndex].getSizeY() * 1000));
          GDSlefSTR.BOUNDARY.push_back(draw2ptBox(30,
                                                  0,
                                                  0,
                                                  this->lefFile.macros[mactoIndex].getSizeX() * 1000,
                                                  this->lefFile.macros[mactoIndex].getSizeY() * 1000));
        }
        else{
          GDSlefSTR.BOUNDARY.push_back(draw2ptBox(200,
                                                  0,
                                                  0,
                                                  this->lefFile.macros[mactoIndex].getSizeX() * 1000,
                                                  this->lefFile.macros[mactoIndex].getSizeY() * 1000));
        }
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