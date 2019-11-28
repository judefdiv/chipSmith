/**
 * Author:      Jude de Villiers
 * Origin:      E&E Engineering - Stellenbosch University
 * For:         Supertools, Coldflux Project - IARPA
 * Created:     2019-11-08
 * Modified:
 * license:
 * Description: Class for storing .lef
 * File:        ParserLef.cpp
 */

#include "chipsmith/ParserLef.hpp"

/**
 * [lef_file::importFile - Imports a LEF file]
 * @param  fileName [The lef file to be imported]
 * @return          [0 - All good; 1 - Error]
 */

int lef_file::importFile(const string &fileName){
  cout << "Importing LEF file \""  << fileName << "\"" << endl;
  vector<vector<string> > strBlock;
  string keyword;
  vector<string> lineVec;
  string tempDis;
  string blockName;

  ifstream lefFile;

  lefFile.open(fileName);

  if(!lefFile.is_open()){
    cout << "LEF file FAILED to be properly opened" << endl;
    return 0;
  }

  while(1){
    lineVec = splitFileLine(lefFile);
    keyword = lineVec[0];
    if(blkEndWithName.find(keyword) != blkEndWithName.end()){

      //Creating a sub block
      strBlock.clear();
      strBlock.push_back(lineVec);
      string blockName = lineVec[1];
      for(int i = 0; i < 254; i++){          // can use while loop, but for loop is used for safety
        lineVec = splitFileLine(lefFile);
        strBlock.push_back(lineVec);
        if(lineVec[0] == "END" && lineVec[1] == blockName)
          break;
      }

      disVectorBlk(strBlock);

      if(keyword == "MACRO"){
        lef_macro cl_macro_Temp;
        cl_macro_Temp.autoCreate(strBlock);
        this->macros.push_back(cl_macro_Temp);
      }

    }
    if(blkEndWithType.find(keyword) != blkEndWithType.end()){

      //Creating a sub block
      strBlock.clear();
      strBlock.push_back(lineVec);
      for(int i = 0; i < 64; i++){
        lineVec = splitFileLine(lefFile);
        strBlock.push_back(lineVec);
        if(lineVec[0] == "END" && lineVec[1] == keyword)
          break;
      }

    }
    else if(validOthWords.find(keyword) != validOthWords.end()){
      // other keyword found
      if(keyword == "NAMESCASESENSITIVE"){
        // assign some stuffs
        // cout << "Stuffs is cases sensitive." << endl;
      }
      else if(keyword == "END"){
        if(lineVec[1] == "LIBRARY"){
          cout << "Importing LEF file done." << endl;
          break;
        }
        else
          cout << "Found an alone END, miss match..." << endl;
      }

    }
    else {
      // word not in the library, out error;

      cout << "Error with line ->" << endl;
      disVector(lineVec);
      cout << "<--------------------" << endl;

      cout << "String not found in lef dictionary" << endl;
      cout << "Totsiens!" << endl;
      return 0;
    }


  }
  lefFile.close();
  return 0;
}

/**
 * [lef_file::importGDF description]
 * @param  fileName [The file name of the .toml Gate Definition File]
 * @return          [1 - All good; 0 - Error]
 */

int lef_file::importGDF(const string &fileName){
  cout << "Importing \"" << fileName << "\"" << endl;

  vector<string> gateList;
  // double ptlWidth = 0;
  double portSizeX = 0;
  double portSizeY = 0;
  double biasSizeX = 0;
  double biasSizeY = 0;
  double viaSizeX = 0;
  double viaSizeY = 0;

  const auto GDF = toml::parse(fileName);
  const auto &configPara    = toml::find(GDF, "ALL_GATES");

  // -----------------------------------------------------------------

  auto element = toml::find(configPara, "list");
  gateList = toml::get<vector<string>>(element);

  vector<float> fltVec;
  vector<string> strVec;

  element   = toml::find(configPara, "port_size");
  fltVec    = toml::get<vector<float>>(element);
  portSizeX = fltVec[0] * unitScale;
  portSizeY = fltVec[1] * unitScale;

  element   = toml::find(configPara, "via_size");
  fltVec    = toml::get<vector<float>>(element);
  viaSizeX  = fltVec[0] * unitScale;
  viaSizeY  = fltVec[1] * unitScale;

  element   = toml::find(configPara, "bias_p_size");
  fltVec    = toml::get<vector<float>>(element);
  biasSizeX = fltVec[0] * unitScale;
  biasSizeY = fltVec[1] * unitScale;

  this->macros.resize(gateList.size());

  for(unsigned int i = 0; i < this->macros.size(); i++){
    this->macros[i].name = gateList[i];
  }

  vector<int> intVec;
  for(auto &mac: this->macros){
    auto gateBlk = toml::find(GDF, mac.name);

    cout << "Importing: " << mac.name << endl;

    element = toml::find(gateBlk, "size");
    intVec = toml::get<vector<int>>(element);
    mac.sizeX = intVec[0] * unitScale;
    mac.sizeY = intVec[1] * unitScale;

    element = toml::find(gateBlk, "origin");
    intVec = toml::get<vector<int>>(element);
    mac.originX = intVec[0] * unitScale;
    mac.originY = intVec[1] * unitScale;

    element = toml::find(gateBlk, "pillar");
    intVec = toml::get<vector<int>>(element);
    if(intVec[0] != -1){
      mac.obs.resize(1);
      mac.obs[0].ptsX.push_back((intVec[0] * unitScale) - (biasSizeX/2));
      mac.obs[0].ptsY.push_back((intVec[1] * unitScale) - (biasSizeY/2));
      mac.obs[0].ptsX.push_back((intVec[0] * unitScale) + (biasSizeX/2));
      mac.obs[0].ptsY.push_back((intVec[1] * unitScale) + (biasSizeY/2));
    }

    // ------------------------------------------------------------------------

    string pinType;
    string pinName;       // the name of the pin in the .toml file, P1, P2, ..., Pn
    unsigned int pinCnt, pinInCnt, pinOutCnt, pinInOutCnt;

    pinCnt = 0;
    pinInCnt = 1;
    pinOutCnt = 1;
    pinInOutCnt = 1;

    element = toml::find(gateBlk, "pins");
    vector<string> strVec = toml::get<vector<string>>(element);
    mac.pins.resize(strVec.size());

    for(auto &foo:mac.pins){
      pinType = strVec[pinCnt];
      pinName = "P" + to_string(++pinCnt);

      element = toml::find(gateBlk, pinName);
      intVec = toml::get<vector<int>>(element);

      foo.ports.resize(1);
      foo.ports[0].layer = "unknown";

      foo.ports[0].ptsX.push_back(((double)intVec[0] * unitScale) - (viaSizeX/2));
      foo.ports[0].ptsY.push_back(((double)intVec[1] * unitScale) - (viaSizeY/2));
      foo.ports[0].ptsX.push_back(((double)intVec[0] * unitScale) + (viaSizeX/2));
      foo.ports[0].ptsY.push_back(((double)intVec[1] * unitScale) + (viaSizeY/2));

      if(!pinType.compare("in")){
        foo.name = "IN_" + to_string(pinInCnt++);
        foo.direction = "IN";
      }
      else if(!pinType.compare("out")){
        foo.name = "OUT_" + to_string(pinOutCnt++);
        foo.direction = "OUT";
      }
      else if(!pinType.compare("clk")){
        foo.name = "CLK";
        foo.direction = "IN";
      }
      else if(!pinType.compare("in_out")){
        foo.name = "INOUT_" + to_string(pinInOutCnt++);
        foo.direction = "INOUT";
      }
      else{
        cout << "Pin naming error with \"" << mac.name << endl;
        return 0;
      }

    }
  }

  // ----------------------- Layers ---------------------------

  element = toml::find(configPara, "layer_names");
  strVec  = toml::get<vector<string>>(element);

  this->layers.resize(strVec.size());

  unsigned int index = 0;

  for(auto &foo: this->layers){
    foo.name      = strVec[index++];
    foo.type      = "ROUTING";
    foo.direction = "HORIZONTAL"; // HORIZONTAL; VERTICAL
    foo.pitch     = 10;
    foo.width     = 5;
    foo.spacing   = 5;
  }

  // ------------------------- VIA -----------------------------

  this->vias.resize(1);

  for(auto &foo: this->vias){
    foo.name = "via1";

    for(auto &bar: strVec){
      foo.layers.push_back(bar);
    }

    for(unsigned int i = 0; i < foo.layers.size(); i++){
      foo.ptsX.push_back(-viaSizeX/2);
      foo.ptsY.push_back(-viaSizeY/2);
      foo.ptsX.push_back(+viaSizeX/2);
      foo.ptsY.push_back(+viaSizeY/2);
    }
  }

  return 1;
}

/**
 * [lef_file::exportLef - creating a lef file]
 * @param  fileName [The file name of the to be created /lef file]
 * @return          [1 - All good; 0 - Error]
 */

int lef_file::exportLef(const string &fileName){
  cout << "Creating lef file: \"" << fileName << "\"" << endl;
  ofstream lefFile;
  // unsigned int index;

  lefFile.open(fileName);

  if(!lefFile){
    cout << "Could not create \"" << fileName << "\"" << endl;
    return 0;
  }

  // ----------------------------- Header -----------------------------

  time_t now = time(0);
  char* currentTime = ctime(&now);

  lefFile << "# Lef file generated with ViPeR " << endl;
  lefFile << "# Jude de Villiers, Stellenbosch University" << endl;
  lefFile << "# " << currentTime << endl;

  // ----------------------------- Overheads -----------------------------

  lefFile << "NAMESCASESENSITIVE ON ;" << endl;
  lefFile << "UNITS" << endl;
  lefFile << "DATABASE MICRONS 100 ;" << endl;
  lefFile << "END UNITS" << endl << endl;

  // ----------------------------- Layers -----------------------------

  for(auto &foo: this->layers){
    lefFile << "LAYER " << foo.name << endl;
    lefFile << "  TYPE " << foo.type << " ;" << endl;
    lefFile << "  PITCH " << foo.pitch << " ;" << endl;
    lefFile << "  WIDTH " << foo.width << " ;" << endl;
    lefFile << "  SPACING " << foo.spacing << " ;" << endl;
    lefFile << "  DIRECTION " << foo.direction << " ;" << endl;
    lefFile << "END " << foo.name << endl;
  }

  // ----------------------------- Vias -----------------------------

  for(auto &foo: this->vias){
    lefFile << "VIA " << foo.name << " DEFAULT" << endl;

    for(unsigned int i = 0; i < foo.layers.size(); i++){
      lefFile << "  LAYER " << foo.layers[i] << " ;" << endl;
      lefFile << "    RECT " << foo.ptsX[0] << " "
                              << foo.ptsY[0] << " "
                              << foo.ptsX[1] << " "
                              << foo.ptsY[1] << " ;" << endl;
    }

    // not sure if this needs to be there!!!
    lefFile << "  LAYER " << foo.name << " ;" << endl;
    lefFile << "    RECT " << foo.ptsX[0] << " "
                        << foo.ptsY[0] << " "
                        << foo.ptsX[1] << " "
                        << foo.ptsY[1] << " ;" << endl;


    lefFile << "END " << foo.name << endl;
  }

  // ----------------------------- Spacing -----------------------------


  // ----------------------------- Site -----------------------------


  // ----------------------------- Macro -----------------------------

  for(auto &foo: this->macros){
    lefFile << "MACRO " << foo.name << endl;
    lefFile << "  TYPE CORE ;" << endl;
    lefFile << "  SIZE " << foo.sizeX << " by " << foo.sizeY << " ;" << endl;
    lefFile << "  ORIGIN " << foo.originX << " " << foo.originY << " ;" << endl;
    lefFile << "  SYMMETRY X ;" << endl;
    lefFile << "  SITE core 0 0 N DO 1 BY 1 STEP 0 0 ;" << endl;

    // ------------ pins ------------
    for(auto &bar: foo.pins){
      lefFile << "  PIN " << bar.name << endl;
      lefFile << "    DIRECTION " << bar.direction << " ;" << endl;
      lefFile << "    USE SIGNAL ;" << endl;
      lefFile << "      PORT" << endl;

      for(auto &spam: bar.ports){
        lefFile << "        LAYER " << spam.layer << " ;" << endl;
        lefFile << "          RECT " << spam.ptsX[0] << " "
                                      << spam.ptsY[0] << " "
                                      << spam.ptsX[1] << " "
                                      << spam.ptsY[1] << " ;" << endl;
      }
      lefFile << "      END" << endl;
      lefFile << "  END " << bar.name << endl;
    }

    // ------------ obs ------------
    if(foo.obs.size() > 0)
      lefFile << "  OBS" << endl;

    for(auto &bar: foo.obs){
      lefFile << "    LAYER " << bar.layer << " ;" << endl;
      lefFile << "      RECT " << bar.ptsX[0] << " "
                                << bar.ptsY[0] << " "
                                << bar.ptsX[1] << " "
                                << bar.ptsY[1] << " ;" << endl;
    }
    lefFile << "  END " << endl;

    lefFile << "END " << foo.name << endl;
  }

  lefFile << "END LIBRARY" << endl;

  lefFile.close();
  cout << "Creating lef file: \"" << fileName << "\" done." << endl;

  return 1;
}

/**
 * [lef_macro::autoCreate - Creates/imports the macro class]
 * @param  inBlock [Input block vector]
 * @return         [0 - EXIT_SUCCESSFUL; 1 - EXIT_ERROR]
 */

int lef_macro::autoCreate(vector<vector<string> > &inBlock){
  int i = 0;

  while(i < inBlock.size() -1){
    if(!inBlock[i][0].compare("MACRO")){
      this->name = inBlock[i][1];
    }
    else if(!inBlock[i][0].compare("SIZE")){
      this->sizeX = stof(inBlock[i][1]);
      this->sizeY = stof(inBlock[i][3]);
    }
    else if(!inBlock[i][0].compare("ORIGIN")){
      this->originX = stof(inBlock[i][1]);
      this->originY = stof(inBlock[i][2]);
    }
    else if(!inBlock[i][0].compare("PIN")){
      // creating a sub block
      vector<vector<string> > subBlock;
      subBlock.clear();
      subBlock.push_back(inBlock[i]);

      string blockName = inBlock[i][1];
      while(i++){
        subBlock.push_back(inBlock[i]);
        if(inBlock[i][0] == "END" && inBlock[i][1] == blockName)
          break;
      }

      disVectorBlk(subBlock);

      int j = 0;
      while(j < subBlock.size()){
        macro_pin st_pin_Temp;
        if(subBlock[j][0] == "PIN"){
          this->pins.resize(this->pins.size()+1);
          this->pins.back().name = subBlock[j][1];
        }
        else if(subBlock[j][0] == "DIRECTION"){
          this->pins.back().direction = subBlock[j][1];
        }
        else if(subBlock[j][0] == "USE" && subBlock[j][1] == "SIGNAL"){
          if(subBlock[++j][0] == "PORT"){
            while(subBlock[++j][0] != "END"){
              if(subBlock[j][0] == "LAYER"){
                macro_port st_port_Temp;
                st_port_Temp.layer = subBlock[j++][1];
                st_port_Temp.ptsX.push_back(stof(subBlock[j][1]));
                st_port_Temp.ptsY.push_back(stof(subBlock[j][2]));
                st_port_Temp.ptsX.push_back(stof(subBlock[j][3]));
                st_port_Temp.ptsY.push_back(stof(subBlock[j][4]));

                this->pins.back().ports.push_back(st_port_Temp);
              }
              else{
                cout << "macro pin error" << endl;
                return 1;
              }
            }
          }
          else{
            cout << "macro pin error" << endl;
            return 1;
          }
        }
        else if(subBlock[j][0] == "END" && subBlock[j][1] ==  this->pins.back().name){
          break;
        }
        j++;
      }

    }
    else if(!inBlock[i][0].compare("OBS")){
      while(inBlock[i][0] != "END"){
        if(inBlock[i][0] == "LAYER"){
          macro_port st_port_Temp;
          st_port_Temp.layer = inBlock[i++][1];

          st_port_Temp.ptsX.push_back(stof(inBlock[i][1]));
          st_port_Temp.ptsY.push_back(stof(inBlock[i][2]));
          st_port_Temp.ptsX.push_back(stof(inBlock[i][3]));
          st_port_Temp.ptsY.push_back(stof(inBlock[i][4]));

          this->obs.push_back(st_port_Temp);
        }
        i++;
      }
    }
    else if(!inBlock[i][0].compare("CLASS")){

    }
    else if(!inBlock[i][0].compare("SYMMETRY")){

    }
    else if(!inBlock[i][0].compare("SITE")){

    }
    else{
      cout << "error in macro" << endl;
      return 1;
    }
    i++;
  }
  return 0;
}

void lef_file::to_str(){
  for(auto foo: this->macros){
    foo.to_str();
  }

  for(auto foo: this->layers){
    foo.to_str();
  }

  for(auto foo: this->vias){
    foo.to_str();
  }
}

void lef_macro::to_str(){
  cout << "Macro:" << endl;
  cout << "  name: " << this->name << endl;
  cout << "  sizeX: " << this->sizeX << endl;
  cout << "  sizeY: " << this->sizeY << endl;
  cout << "  originX: " << this->originX << endl;
  cout << "  originY: " << this->originY << endl;

  cout << "  Pins:" << endl;
  for(auto foo: this->pins){
    cout << "    name: " << foo.name << endl;
    cout << "    direction: " << foo.direction << endl;

    for(auto boo:foo.ports){
      cout << "      layer: " << boo.layer << endl;
      for(unsigned int i = 0; i < boo.ptsX.size(); i++){
        cout << "      ptsX[" << i << "]: " << boo.ptsX[i] << endl;
        cout << "      ptsY[" << i << "]: " << boo.ptsY[i] << endl;
      }
    }
  }
  cout << endl;
}

void lef_layer::to_str(){
  cout << "layer:" << endl;
  cout << "  name: " << this->name << endl;
  cout << "  type: " << this->type << endl;
  cout << "  direction: " << this->direction << endl;
  cout << "  pitch: " << this->pitch << endl;
  cout << "  width: " << this->width << endl;
  cout << "  spacing: " << this->spacing << endl;
}

void lef_via::to_str(){
  cout << "layer:" << endl;
  cout << "  name: " << this->name << endl;

  cout << "  Layer:  X:     Y:" << endl;
  cout << setprecision(2);
  for(unsigned int i = 0; i < this->layers.size(); i++){
    cout << i << ": " << this->layers[i] << "  " << this->ptsX[i] << "  " << this->ptsY[i] << endl;
    // cout << "  layers[" << i << "]: " << this->layers[i] << endl;
    // cout << "  ptsX[" << i << "]: " << this->ptsX[i] << endl;
    // cout << "  ptsY[" << i << "]: " << this->ptsY[i] << endl;
  }
}