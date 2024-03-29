/**
 * Author:      Jude de Villiers
 * Origin:      E&E Engineering - Stellenbosch University
 * For:         Supertools, Coldflux Project - IARPA
 * Created:     2019-11-27
 * Modified:
 * license:
 * Description: All the ViPeR tools get executed here
 * File:        toolFlow.cpp
 */

#include "chipsmith/toolFlow.hpp"

/**
 * [forgeChip description]
 * @param  lefFile [The LEF file to be imported]
 * @param  defFile [The DEF file to be imported]
 * @param  gdsFile [The GDS file to be imported]
 * @param  conFile [The toml config file to be imported]
 * @return         [0 - All good; 1 - Error]
 */

int forgeChip(const string &lefFileName, const string &defFileName, const string &gdsFileName, const string &conFileName){

  chipSmith gdsChip;
  gdsChip.importData(lefFileName, defFileName, conFileName);
  gdsChip.toGDS(gdsFileName);

  return 0;
}