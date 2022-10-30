/**
 * Author:  		Jude de Villiers
 * Origin:  		E&E Engineering - Stellenbosch University
 * For:					Supertools, Coldflux Project - IARPA
 * Created: 		2019-11-27
 * Modified:
 * license:
 * Description: Primary file for the program.
 * File:				main.cpp
 */

#include <iostream>				//stream
#include <string>					//string goodies
#include <set>
#include <map>

// #include "chipsmith/genFunc.hpp"
#include "chipsmith/toolFlow.hpp"
#include "toml/toml.hpp"

#define versionNo 0.1
#define configFile "config.toml"

using namespace std;

/**
 * Declaring functions
 */

void welcomeScreen();
void helpScreen();
int RunTool(int argCount, char** argValues);
int RunToolFromConfig(string fileName);

/**
 * Main loop
 */

int main(int argc, char* argv[]){
	// welcomeScreen();
	if(!RunTool(argc, argv)) return 0;

	return 0;
}

/**
 * [RunTool - Runs the appropriate tool]
 * @param  argCount  [Size of the input argument array]
 * @param  argValues [The input argument array]
 * @return           [0 - all good; 1 - error]
 */

int RunTool(int argCount, char** argValues){
	welcomeScreen();

	if(argCount <= 1){
		return 1;
	}

	set<string> validCommands = {"-v", "-h", "-c"};

	string tomlFName = "\0";			// .toml
	string command  = "\0";			// The command to be executed

	string foo;

	// search for command
	for(int i = 0; i < argCount; i++){
		foo = string(argValues[i]);
		if(validCommands.find(foo) != validCommands.end()){
			command = foo;
		}
	}
	if(!command.compare("\0")){
		cout << "Invalid." << endl;
		return 1;
	}

	// search for .toml
	for(int i = 0; i < argCount; i++){
		foo = string(argValues[i]);
	  if(foo.find(".toml")!=string::npos){
	  	tomlFName = foo;
	  }
	}

	if(!command.compare("-c")){
		if(tomlFName.compare("\0")){
			return RunToolFromConfig(tomlFName);
		}
		else {
			cout << "Input argument error." << endl;
			return 1;
		}
	}
	else if(!command.compare("-v")){
		if(argCount == 1 + 1){
			cout << setprecision(2);
			cout << "Version: " << versionNo << endl;
			return 0;
		}
		else{
			cout << "Input argument error." << endl;
			return 1;
		}
	}
	else if(!command.compare("-h")){
		helpScreen();
		return 0;
	}
	else{
		cout << "Quickly catch the smoke before it escapes." << endl;
		return 1;
	}

	cout << "I am smelling smoke." << endl;
	return 1;
}

/**
 * [RunToolFromConfig - Runs the tools using parameters from config.toml]
 * @param  fileName [The config toml file]
 * @return          [1 - all good; 0 - error]
 */

int RunToolFromConfig(string fileName){
	cout << "Importing execution parameters from config.toml" << endl;

	const auto mainConfig  = toml::parse(fileName);
	map<string, string> run_para = toml::get<map<string, string>>(mainConfig.at("File_Location"));

	map<string, string>::iterator it_run_para;

	string command  = "g";
	string gdsFName = "\0";
	string defFName = "\0";
	string lefFName = "\0";
	string workDir  = "\0";

	// it_run_para = run_para.find("Command");
	// if(it_run_para != run_para.end()){
	// 	command = it_run_para->second;
	// }
	// else{
	// 	cout << "Invalid parameters." << endl;
	// 	return 0;
	// }

	// it_run_para = run_para.find("work_dir");
	// if(it_run_para != run_para.end()){
	// 	workDir = it_run_para->second;
	// }

	it_run_para = run_para.find("GDSfile");
	if(it_run_para != run_para.end()){
		gdsFName = it_run_para->second;
		// gdsFName = gdsFName.insert(0, workDir);
	}

	it_run_para = run_para.find("LEFfile");
	if(it_run_para != run_para.end()){
		lefFName = it_run_para->second;
		// lefFName = lefFName.insert(0, workDir);
	}

		it_run_para = run_para.find("DEFfile");
	if(it_run_para != run_para.end()){
		defFName = it_run_para->second;
		// lefFName = lefFName.insert(0, workDir);
	}

	if(!command.compare("g")){
		if(defFName.compare("\0") && gdsFName.compare("\0") && lefFName.compare("\0")){
			forgeChip(lefFName, defFName, gdsFName, configFile);
			return 0;
		}
		else{
			cout << "Input argument error." << endl;
			return 1;
		}
	}
	else{
		cout << "Invalid command." << endl;
		return 1;
	}

	return 1;
}


void helpScreen(){
	cout << "=====================================================================" << endl;
	cout << "Usage: chipSmith [ OPTION ] [ filenames ]" << endl;
	cout << "-c(onfig)     Runs the tools using the parameters in the toml file." << endl;
	cout << "                [.toml file]" << endl;
	cout << "-v(ersion)    Displays the version number." << endl;
	cout << "-h(elp)       Help screen." << endl;
	cout << "=====================================================================" << endl;
}

/**
 * Welcoming screen
 */

void welcomeScreen(){
	cout << "===============================================" << endl;
	cout << "                chipSmith" << endl;
	cout << "             Author: JF de Villiers" << endl;
	cout << "            Stellenbosch University" << endl;
	cout << "          For IARPA, ColdFlux project" << endl;
	cout << "===============================================" << endl;
}