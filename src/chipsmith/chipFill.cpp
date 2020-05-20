/**
 * Author:      Jude de Villiers
 * Origin:      E&E Engineering - Stellenbosch University
 * For:         Supertools, Coldflux Project - IARPA
 * Created:     2020-04-21
 * Modified:
 * license:
 * Description: Creates and populates the GDS chip
 * File:        chipFill.cpp
 */

#include "chipsmith/chipFill.hpp"

/**
 * [chipSmith::genGDS - Creates the GDS file]
 * @param  gdsFileName [The GDS file to be created]
 * @return         [0 - All good; 1 - Error]
 */

int chipSmith::toGDS(const string &gdsFileName){
  gdsSTR GDSmainSTR;

  this->importGates();
  if(this->fillEnable) this->importFill();
  this->placeGates();
  this->placeNets();
  this->placeBias();
  if(this->fillEnable) this->placeFill();

  GDSmainSTR.name = fileRenamer(gdsFileName, "", "");
  GDSmainSTR.SREF.push_back(drawSREF("Components", 0, 0));
  GDSmainSTR.SREF.push_back(drawSREF("Nets", 0, 0));
  GDSmainSTR.SREF.push_back(drawSREF("Vias", 0, 0));
  GDSmainSTR.SREF.push_back(drawSREF("Biases", 0, 0));
  if(this->fillEnable) GDSmainSTR.SREF.push_back(drawSREF("Fill", 0, 0));

  gdsF.setSTR(GDSmainSTR);
  gdsF.write(gdsFileName);

  return 0;
}

/**
 * [chipSmith::importData description]
 * @param  lefFileName [The LEF file to be imported]
 * @param  defFileName [The DEF file to be imported]
 * @param  conFileName [The toml config file to be imported]
 * @return         [0 - All good; 1 - Error]
 */

int chipSmith::importData(const string &lefFileName, const string &defFileName, const string &conFileName){
  cout << "Importing data." << endl;

  this->lefFile.importFile(lefFileName);
  this->defFile.importFile(defFileName);

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
  this->GateBiasCorX    = toml::get<map<string, int>>(mainConfig.at("Biasing_Coordinate"));

  /***************************************************************************
   ************************** Fill Parameters ********************************
   ***************************************************************************/

  const auto &Para = toml::find(mainConfig, "Parameters");

  auto element = toml::find(Para, "fill");
  this->fillEnable = toml::get<bool>(element);

  element = toml::find(Para, "fillCor");
  this->fillCor = toml::get<vector<int>>(element);

  element = toml::find(Para, "gateHeights");
  this->gateHeight = toml::get<int>(element) * 1000;

  element = toml::find(Para, "PTLwidth");
  this->PTLwidth = toml::get<float>(element) * 1000;

  element   = toml::find(Para, "gridSize");
  this->gridSize    = toml::get<int>(element);

  gridLX = (fillCor[2] - fillCor[0])/this->gridSize;
  gridLY = (fillCor[3] - fillCor[1])/this->gridSize;

  cout << "Grid size: " << gridLX << "x" << gridLY << endl;

  cout << "Defining grid." << endl;

  vector<bool> yFill;
  yFill.resize(gridLY, true);

  this->grid.resize(7);

  for(auto &foo: this->grid){
    foo.resize(gridLX, yFill);
  }

  cout << "Defining grid, done." << endl;

  /***************************************************************************
   ******************************* Cell Size *********************************
   ***************************************************************************/

  // this->cellSize = toml::get<map<string, vector<int>>>(mainConfig.at("GDS_CELL_SIZES"));

  // for(auto const &itMac: this->lefFile.macros){
  //   this->cellSize.insert(pair<string,vector<int>>(itMac.name, {(int)itMac.sizeX, (int)itMac.sizeY}));
  // }

  // cout << "Sneak peak into cellSize" << endl;
  // for(auto const& [key, val]: this->cellSize){
  //   cout << key << ":" << val[0] << "," << val[1] << endl;
  // }

  cout << "Importing data, done." << endl;
  return 0;
}

/**
 * [chipSmith::placeGates - Reads the locations of the gates in the DEF file and places them in the gate file]
 * @return [0 - All good; 1 - Error]
 */

int chipSmith::placeGates(){
  cout << "Placing gates." << endl;

  gdsSTR GDSdefSTR;
  GDSdefSTR.name = "Components";

  for(auto &itComps: defFile.comps){
    GDSdefSTR.SREF.push_back(drawSREF(this->lef2gdsNames[itComps.getCompType()],
                                      itComps.getCorX() * 10,
                                      itComps.getCorY() * 10));
  }

  gdsF.setSTR(GDSdefSTR);

  cout << "Placing gates, done." << endl;

  return 0;
}

/**
 * [forgedChip::placeNets - Reads the description of the nets in the DEf file and routes them in the GDS file]
 * @return [0 - All good; 1 - Error]
 */

int chipSmith::placeNets(){
  cout << "Routing nets." << endl;
  gdsSTR GDSroute;
  gdsSTR GDSvia;

  GDSroute.name = "Nets";
  GDSvia.name = "Vias";

  /**
   * Routes/tracks/PTLs
   */

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
        GDSroute.PATH.push_back(drawPath(10, this->PTLwidth, corX, corY));
      }
      else if(!itPath.LAYER.compare("metal2")){
        GDSroute.PATH.push_back(drawPath(30, this->PTLwidth, corX, corY));
      }
    }
  }

  /**
   * VIAS
   */

  for(auto &itNet: this->defFile.nets){
    for(int i = 0; i < itNet.routes.size() -1; i++){
      GDSvia.SREF.push_back(drawSREF("ViaM1M3", itNet.routes[i].ptX.back() * 10, itNet.routes[i].ptY.back() * 10));
    }
  }

  this->gdsF.setSTR(GDSroute);
  this->gdsF.setSTR(GDSvia);

  cout << "Routing nets, done." << endl;

  return 0;
}

/**
 * [chipSmith::placeFill - Checks where fill is needed and does it...]
 * @return [0 - All good; 1 - Error]
 */

int chipSmith::placeFill(){
  cout << "Placing fill." << endl;
  gdsSTR GDSfill;
  GDSfill.name = "Fill";

  const vector<string> gdsFillName = {"fillAll", "fillM1", "fillM2", "fillM3", "fillM4", "fillM5via", "fillM6"};
  vector<gdsSTR> GDSfil;
  GDSfil.resize(gdsFillName.size());

  /***************************************************************************
   ******************************* Fill All **********************************
   ***************************************************************************/
  /*
   * Fill the whole circuit
   */

  cout << "Filling the whole circuit." << endl;

  GDSfil[0].name = "FillAll";
  GDSfill.SREF.push_back(drawSREF("FillAll", 0, 0));

  cout << "Filling the whole circuit, done." << endl;

  /***************************************************************************
   ****************************** Fill M4 & M6 *******************************
   ***************************************************************************/
  /*
   * Fill where there are no cells
   */

  cout << "Filling M4 & M6, around gates." << endl;

  GDSfil[4].name = "FillM4";
  GDSfil[6].name = "FillM6";
  GDSfill.SREF.push_back(drawSREF("FillM4", 0, 0));
  GDSfill.SREF.push_back(drawSREF("FillM6", 0, 0));

  /**
   * Find the index of the Components in STR vector
   */

  unsigned int compIndex;
  for(compIndex = 0; compIndex < this->gdsF.STR.size(); compIndex++){
    if(!this->gdsF.STR[compIndex].name.compare("Components")){
      break;
    }
  }

  /**
   * Modifying the grid
   */

  for(const auto &comps: this->gdsF.STR[compIndex].SREF){

    // cout << "N: " << comps.name << " " << comps.xCor << ", " << comps.yCor << endl;

    int x_0 = (comps.xCor) - (fillCor[0] *1000) + (this->cellSizes[comps.name][0]);
    int y_0 = (comps.yCor) - (fillCor[1] *1000) + (this->cellSizes[comps.name][1]);
    int x_1 = (comps.xCor) - (fillCor[0] *1000) + (this->cellSizes[comps.name][2]);
    int y_1 = (comps.yCor) - (fillCor[1] *1000) + (this->cellSizes[comps.name][3]);

    // cout << "x_1, y_1; x_2, y_2: " << x_0 << ", " <<  y_0 << "; "<< x_1 << ", " <<  y_1 << endl;

    x_0 = constrain(x_0 / 10000, 0, gridLX);
    y_0 = constrain(y_0 / 10000, 0, gridLY);
    x_1 = constrain(x_1 / 10000, 0, gridLX);
    y_1 = constrain(y_1 / 10000, 0, gridLY);


    for(unsigned int i = x_0; i < x_1; i++){
      for(unsigned int j = y_0; j < y_1; j++){
        this->grid[4][i][j] = false;
        this->grid[6][i][j] = false;
      }
    }
  }

  cout << "Filling M4 & M6, around gates, done." << endl;

  /***************************************************************************
   ******************************* Fill M2 ***********************************
   ***************************************************************************/
  /*
   * Fill where there are no vias
   */

  cout << "Filling M2, around vias." << endl;

  GDSfil[2].name = "FillM2";
  GDSfill.SREF.push_back(drawSREF("FillM2", 0, 0));

  /**
   * Getting the size of the VIA Structure
   */

  int viaSize[4];

  for(unsigned int i = 0; i < this->gdsF.STR.size(); i++){
    if(!this->gdsF.STR[i].name.compare("ViaM1M3")){
      this->gdsF.calculate_STR_bounding_box(i, viaSize);
      // cout << "Via size: " << viaSize[0] << ", " <<  viaSize[1] << "; "<< viaSize[2] << ", " <<  viaSize[3] << endl;
      break;
    }
  }

  /**
   * Find the index of the Vias in STR vector
   */

  unsigned int viaIndex;
  for(viaIndex = 0; viaIndex < this->gdsF.STR.size(); viaIndex++){
    if(!this->gdsF.STR[viaIndex].name.compare("Vias")){
      break;
    }
  }

  /**
   * Modifying the grid
   */

  for(const auto &vias: this->gdsF.STR[viaIndex].SREF){
    int x_0 = (vias.xCor) - (fillCor[0] *1000) + (viaSize[0]);
    int y_0 = (vias.yCor) - (fillCor[1] *1000) + (viaSize[1]);
    int x_1 = (vias.xCor) - (fillCor[0] *1000) + (viaSize[2]);
    int y_1 = (vias.yCor) - (fillCor[1] *1000) + (viaSize[3]);

    x_0 = constrain(round((float)x_0 / 10000), 0, gridLX);
    y_0 = constrain(round((float)y_0 / 10000), 0, gridLY);
    x_1 = constrain(round((float)x_1 / 10000), 0, gridLX);
    y_1 = constrain(round((float)y_1 / 10000), 0, gridLY);

    for(unsigned int i = x_0; i < x_1; i++){
      for(unsigned int j = y_0; j < y_1; j++){
        this->grid[2][i][j] = false;
      }
    }
  }

  cout << "Filling M2, around vias, done." << endl;

  /***************************************************************************
   ********************************* M1 & M3 *********************************
   ***************************************************************************/
  /**
   * Fill where there is no routing
   */

  cout << "Filling M1 & M3, around tracks." << endl;

  GDSfil[1].name = "FillM1";
  GDSfil[3].name = "FillM3";
  GDSfill.SREF.push_back(drawSREF("FillM1", 0, 0));
  GDSfill.SREF.push_back(drawSREF("FillM3", 0, 0));

  /**
   * Find the index of the Nets in STR vector
   */

  unsigned int netIndex;
  for(netIndex = 0; netIndex < this->gdsF.STR.size(); netIndex++){
    if(!this->gdsF.STR[netIndex].name.compare("Nets")){
      break;
    }
  }

  /**
   * Modifying the grid
   */

  for(const auto &path: this->gdsF.STR[netIndex].PATH){

    for(unsigned int i = 0; i < path.xCor.size() -1; i++){

      int x_0 = (path.xCor[i]) - (fillCor[0] *1000);
      int y_0 = (path.yCor[i]) - (fillCor[1] *1000);
      int x_1 = (path.xCor[i+1]) - (fillCor[0] *1000);
      int y_1 = (path.yCor[i+1]) - (fillCor[1] *1000);

      // cout << "[" << path.layer/10 << "]: " << x_0 << ", " <<  y_0 << "; "<< x_1 << ", " <<  y_1 << endl;

      x_0 /= 10000;
      y_0 /= 10000;
      x_1 /= 10000;
      y_1 /= 10000;

      if(x_0 == x_1){
        //vertical

        if((x_0 < 0) || (x_0 >= gridLX)){
          continue;
        }
        else if((y_0 < 0 && y_1 < 0) || (y_0 >= gridLY && y_1 >= gridLY)){  // if route is off grid
          continue;
        }

        y_0 = constrain(y_0, 0, gridLY-1);
        y_1 = constrain(y_1, 0, gridLY-1);

        if(y_1 > y_0){
          for(int j = y_0; j <= y_1; j++){
            this->grid[path.layer/10][x_0][j] = false;
          }
        }
        else{
          for(int j = y_0; j >= y_1; j--){
            this->grid[path.layer/10][x_0][j] = false;
          }
        }
      }
      else if(y_0 == y_1){
        //horizontal

        if((y_0 < 0) || (y_0 >= gridLY)){
          continue;
        }
        else if((x_0 < 0 && x_1 < 0) || (x_0 >= gridLX && x_1 >= gridLX)){  // if route is off grid
          continue;
        }

        x_0 = constrain(x_0, 0, gridLX-1);
        x_1 = constrain(x_1, 0, gridLX-1);

        if(x_1 > x_0){
          for(int j = x_0; j <= x_1; j++){
            this->grid[path.layer/10][j][y_0] = false;
          }
        }
        else{
          for(int j = x_0; j >= x_1; j--){
            this->grid[path.layer/10][j][y_0] = false;
          }
        }
      }
      else{
        cout << "Error: Non Manhattan routes..." << endl;
      }
    }
  }

  cout << "Filling M1 & M3, around tracks, done." << endl;

  /***************************************************************************
   ******************************* Fill others *******************************
   ***************************************************************************/

  cout << "Filling M5, around gates and biasing tracks." << endl;

  GDSfil[5].name = "FillM5";
  GDSfill.SREF.push_back(drawSREF("FillM5", 0, 0));

  /**
   * Modifying the grid
   */

  for(const auto &comps: this->gdsF.STR[compIndex].SREF){

    // cout << "N: " << comps.name << " " << comps.xCor << ", " << comps.yCor << endl;

    int x_0 = (comps.xCor) - (fillCor[0] *1000) + (this->cellSizes[comps.name][0]);
    int y_0 = (comps.yCor) - (fillCor[1] *1000) + (this->cellSizes[comps.name][1]);
    int x_1 = (comps.xCor) - (fillCor[0] *1000) + (this->cellSizes[comps.name][2]);
    int y_1 = (comps.yCor) - (fillCor[1] *1000) + (this->cellSizes[comps.name][3]);

    // cout << "x_1, y_1; x_2, y_2: " << x_0 << ", " <<  y_0 << "; "<< x_1 << ", " <<  y_1 << endl;

    x_0 = constrain(x_0 / 10000, 0, gridLX);
    y_0 = constrain(y_0 / 10000, 0, gridLY);
    x_1 = constrain(x_1 / 10000, 0, gridLX);
    y_1 = constrain(y_1 / 10000, 0, gridLY);

    for(unsigned int i = x_0; i < x_1; i++){
      for(unsigned int j = y_0; j < y_1; j++){
        this->grid[5][i][j] = false;
      }
    }
  }

  /**
   * Find the index of the Biases in STR vector
   */

  unsigned int biasIndex;
  for(biasIndex = 0; biasIndex < this->gdsF.STR.size(); biasIndex++){
    if(!this->gdsF.STR[biasIndex].name.compare("Biases")){
      break;
    }
  }

  /**
   * Modifying the grid
   */

  for(const auto &path: this->gdsF.STR[biasIndex].PATH){

    for(unsigned int i = 0; i < path.xCor.size() -1; i++){

      int x_0 = (path.xCor[i]) - (fillCor[0] *1000);
      int y_0 = (path.yCor[i]) - (fillCor[1] *1000);
      int x_1 = (path.xCor[i+1]) - (fillCor[0] *1000);
      int y_1 = (path.yCor[i+1]) - (fillCor[1] *1000);

      // cout << "[" << path.layer/10 << "]: " << x_0 << ", " <<  y_0 << "; "<< x_1 << ", " <<  y_1 << endl;

      x_0 /= 10000;
      y_0 /= 10000;
      x_1 /= 10000;
      y_1 /= 10000;

      if(x_0 == x_1){
        //vertical

        if((x_0 < 0) || (x_0 >= gridLX)){
          continue;
        }
        else if((y_0 < 0 && y_1 < 0) || (y_0 >= gridLY && y_1 >= gridLY)){  // if route is off grid
          continue;
        }

        y_0 = constrain(y_0, 0, gridLY-1);
        y_1 = constrain(y_1, 0, gridLY-1);

        if(y_1 > y_0){
          for(int j = y_0; j <= y_1; j++){
            this->grid[5][x_0][j] = false;
          }
        }
        else{
          for(int j = y_0; j >= y_1; j--){
            this->grid[5][x_0][j] = false;
          }
        }
      }
      else if(y_0 == y_1){
        //horizontal

        if((y_0 < 0) || (y_0 >= gridLY)){
          continue;
        }
        else if((x_0 < 0 && x_1 < 0) || (x_0 >= gridLX && x_1 >= gridLX)){  // if route is off grid
          continue;
        }

        x_0 = constrain(x_0, 0, gridLX-1);
        x_1 = constrain(x_1, 0, gridLX-1);

        if(x_1 > x_0){
          for(int j = x_0; j <= x_1; j++){
            this->grid[5][j][y_0] = false;
          }
        }
        else{
          for(int j = x_0; j >= x_1; j--){
            this->grid[5][j][y_0] = false;
          }
        }
      }
      else{
        cout << "Error: Non Manhattan routes..." << endl;
      }
    }
  }

  cout << "Filling M5, around gates and biasing tracks, done." << endl;

  /***************************************************************************
   ******************************* Plot Grid *********************************
   ***************************************************************************/

  for(unsigned int i = 0; i < grid.size(); i++){
    for(unsigned int x = 0; x < grid[i].size(); x++){
      for(unsigned int y = 0; y < grid[i][x].size(); y++){
        if(this->grid[i][x][y] == true){
          GDSfil[i].SREF.push_back(drawSREF(gdsFillName[i], (fillCor[0] * 1000) + (x*10000), (fillCor[1] * 1000) + (y*10000)));
        }
      }
    }
  }

  gdsF.setSTR(GDSfill);
  gdsF.setSTR(GDSfil);

  cout << "Placing fill, done." << endl;

  return 0;
}

/**
 * [chipSmith::placeBias - Connects/creates all the biases of the gates]
 * @return [description]
 */
int chipSmith::placeBias(){

  cout << "Routing biases." << endl;

  unsigned int compIndex;
  for(compIndex = 0; compIndex < this->gdsF.STR.size(); compIndex++){
    if(!this->gdsF.STR[compIndex].name.compare("Components")){
      break;
    }
  }

  /***************************************************************************
   ************************** Row Calculations *******************************
   ***************************************************************************/

  set<int> rowCor; // y-coordinates

  int setOffset = 5000 + (this->gateHeight);

  for(const auto &itSTR: this->gdsF.STR[compIndex].SREF){
    rowCor.insert(itSTR.yCor + setOffset);
  }

  set<int>::iterator setIt;
  set<int>::iterator setIt2;

  setIt = rowCor.begin();
  rowCor.erase(setIt);

  setIt = rowCor.end();
  setIt--;
  rowCor.erase(setIt);

  // cout << "Rows for biasing: ";
  // for(setIt = rowCor.begin(); setIt != rowCor.end(); setIt++){
  //   cout << *setIt / 1000 << ", ";
  // }
  // cout << endl;

  /***************************************************************************
   ************************** Column Calculations ****************************
   ***************************************************************************/

  int colCorMin = 10000000;
  int colCorMax = 0;

  for(const auto &itSTR: this->gdsF.STR[compIndex].SREF){
    if(!itSTR.name.compare("PAD")){
      continue;
    }

    if(colCorMin > cellSizes[itSTR.name][0] + itSTR.xCor){
      colCorMin = cellSizes[itSTR.name][0] + itSTR.xCor;
    }
    if(colCorMax < cellSizes[itSTR.name][2] + itSTR.xCor){
      colCorMax = cellSizes[itSTR.name][2] + itSTR.xCor;
    }
  }

  colCorMin -= gridSize *1000 / 2;
  colCorMax += gridSize *1000 / 2;

  // cout << "Left most coordinate: " << colCorMin / 1000 << endl;
  // cout << "Right most coordinate: " << colCorMax / 1000 << endl;

  /***************************************************************************
   *************************** Building Main Grid ****************************
   ***************************************************************************/

  gdsSTR GDSbias;

  GDSbias.name = "Biases";
  
  vector<int> corX;
  vector<int> corY;

  corX.push_back(colCorMin);
  corX.push_back(colCorMax);

  for(setIt = rowCor.begin(); setIt != rowCor.end(); setIt++){
    corY.push_back(*setIt);
    corY.push_back(*setIt);
    GDSbias.PATH.push_back(drawPath(50, this->PTLwidth, corX, corY));
    corY.clear();
  }

  setIt = rowCor.begin();
  setIt2 = rowCor.end();
  setIt2--;

  corX.clear();
  corX.push_back(colCorMin);
  corX.push_back(colCorMin);
  corY.push_back(*setIt);
  corY.push_back(*setIt2);
  GDSbias.PATH.push_back(drawPath(50, this->PTLwidth, corX, corY));

  corX.clear();
  corX.push_back(colCorMax);
  corX.push_back(colCorMax);
  GDSbias.PATH.push_back(drawPath(50, this->PTLwidth, corX, corY));

  /***************************************************************************
   *********************** Connecting Gate to Main Grid **********************
   ***************************************************************************/

  for(const auto &itSTR: this->gdsF.STR[compIndex].SREF){
    if(!itSTR.name.compare("PAD")){
      continue;
    }
    corX.clear();
    corY.clear();
    corX.push_back(itSTR.xCor + (this->GateBiasCorX[itSTR.name] * 1000));
    corX.push_back(itSTR.xCor + (this->GateBiasCorX[itSTR.name] * 1000));
    corY.push_back(itSTR.yCor + this->gateHeight + (gridSize * 500));
    corY.push_back(itSTR.yCor + this->gateHeight - (gridSize * 500));
    GDSbias.PATH.push_back(drawPath(50, this->PTLwidth, corX, corY));
  }

  this->gdsF.setSTR(GDSbias);

  cout << "Routing biases, done." << endl;

  return 0;
}

/**
 * [chipSmith::importGates - Defines the GDS structures for all the gates]
 * @return [0 - All good; 1 - Error]
 */

int chipSmith::importGates(){
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
      // gdsF.importGDSfile(itName->second);
      gdsF.import(itName->second);
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
        gdsF.setSTR(GDSlefSTR);
      }
      else{
        cout << "Missing GDS and LEF definition for gate: \"" << itList << "\"" << endl;
        return 1;
      }
    }


  }

  cout << "Importing the gates GDS files, done." << endl;

  /**
   * Calculating the size of all the gates/cells/structures
   */
  
  cout << "Calculating size of the gates." << endl;

  vector<int> foo;
  foo.resize(4);
  int bar[4];

  for(unsigned int i = 0; i < this->gdsF.STR.size(); i++){
    for(const auto &itGates: usedGates){
      if(!this->gdsF.STR[i].name.compare(this->lef2gdsNames[itGates])){
        this->gdsF.calculate_STR_bounding_box(i, bar);

        for(unsigned int j = 0; j < 4; j++){
          foo[j] = round(((float)bar[j])/10000)*10000;
        }

        this->cellSizes.insert(pair<string, vector<int>>(this->lef2gdsNames[itGates], foo));
        break;
      }
    }
  }

  cout << "Calculating size of the gates, done." << endl;

  cout << "Cell bounding boxes:" << endl;
  for(auto const& [key, value]: this->cellSizes){
    cout << "  " << key << ": " << value[0] << ", " << value[1] << "; " << value[2] << ", " << value[3] << endl;
  }

  return 0;
}


/**
 * [forgedChip::importFill - Defines the fill structures]
 * @return [0 - All good; 1 - Error]
 */

int chipSmith::importFill(){
  cout << "Defining fill structures." << endl;

  map<string, string>::iterator itLoc;

  for(itLoc = this->gdsFillFileLoc.begin(); itLoc != this->gdsFillFileLoc.end(); itLoc++){
    cout << "Importing GDS: " << itLoc->second << endl;
    // gdsF.importGDSfile(itLoc->second);
    gdsF.import(itLoc->second);
  }

  cout << "Defining fill structures, done." << endl;
  return 0;
}

/**
 * [constrain - Limits the value to the limits]
 * @param  inVal      [The value that must be constrained/limited]
 * @param  lowerLimit [The lower limit]
 * @param  upperLimit [The upper limit]
 * @return            [The final constrained value]
 */

int constrain(int inVal, int lowerLimit, int upperLimit){
  if(inVal > upperLimit){
    return upperLimit;
  }
  else if(inVal < lowerLimit){
    return lowerLimit;
  }
  else{
    return inVal;
  }
}