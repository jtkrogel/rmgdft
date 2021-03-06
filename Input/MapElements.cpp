#include <unordered_map>
#include <string>

#include "MapElements.h"
#include "rmg_error.h"
#include "RmgException.h"
#include <iostream>



std::unordered_map<std::string, const int> SymbolToNumber ({
{"0?", 0},
{"H", 1},
{"He", 2},
{"Li", 3},
{"Be", 4},
{"B", 5},
{"C", 6},
{"N", 7},
{"O", 8},
{"F", 9},
{"Ne", 10},
{"Na", 11},
{"Mg", 12},
{"Al", 13},
{"Si", 14},
{"P", 15},
{"S", 16},
{"Cl", 17},
{"Ar", 18},
{"K", 19},
{"Ca", 20},
{"Sc", 21},
{"Ti", 22},
{"V", 23},
{"Cr", 24},
{"Mn", 25},
{"Fe", 26},
{"Co", 27},
{"Ni", 28},
{"Cu", 29},
{"Zn", 30},
{"Ga", 31},
{"Ge", 32},
{"As", 33},
{"Se", 34},
{"Br", 35},
{"Kr", 36},
{"Rb", 37},
{"Sr", 38},
{"Y", 39},
{"Zr", 40},
{"Nb", 41},
{"Mo", 42},
{"Tc", 43},
{"Ru", 44},
{"Rh", 45},
{"Pd", 46},
{"Ag", 47},
{"Cd", 48},
{"In", 49},
{"Sn", 50},
{"Sb", 51},
{"Te", 52},
{"I", 53},
{"Xe", 54},
{"Cs", 55},
{"Ba", 56},
{"La", 57},
{"Ce", 58},
{"Pr", 59},
{"Nd", 60},
{"Pm", 61},
{"Sm", 62},
{"Eu", 63},
{"Gd", 64},
{"Tb", 65},
{"Dy", 66},
{"Ho", 67},
{"Er", 68},
{"Tm", 69},
{"Yb", 70},
{"Lu", 71},
{"Hf", 72},
{"Ta", 73},
{"W", 74},
{"Re", 75},
{"Os", 76},
{"Ir", 77},
{"Pt", 78},
{"Au", 79},
{"Hg", 80},
{"Tl", 81},
{"Pb", 82},
{"Bi", 83},
{"Po", 84},
{"At", 85},
{"Rn", 86},
{"Fr", 87},
{"Ra", 88},
{"Ac", 89},
{"Th", 90},
{"Pa", 91},
{"U", 92},
{"Np", 93},
{"Pu", 94},
{"Am", 95},
{"Cm", 96},
{"Bk", 97},
{"Cf", 98},
{"Es", 99},
{"Fm", 100},
{"Md", 101},
{"No", 102},
{"Lr", 103},
{"Rf", 104},
{"Db", 105},
{"Sg", 106},
{"Bh", 107},
{"Hs", 108},
{"Mt", 109},
{"Ds", 110}
});

std::unordered_map<std::string, const int> SymbolToMass ({
{"H",  1.0080},
{"He",  4.0026},
{"Li",  6.9400},
{"Be",  9.0122},
{"B", 10.8100},
{"C", 12.0110},
{"N", 14.0070},
{"O", 15.9990},
{"F", 18.9984},
{"Ne", 20.1797},
{"Na", 22.9898},
{"Mg", 24.3050},
{"Al", 26.9815},
{"Si", 28.0850},
{"P", 30.9738},
{"S", 32.0600},
{"Cl", 35.4500},
{"Ar", 39.9480},
{"K", 39.0983},
{"Ca", 40.0780},
{"Sc", 44.9559},
{"Ti", 47.8670},
{"V", 50.9415},
{"Cr", 51.9961},
{"Mn", 54.9380},
{"Fe", 55.8450},
{"Co", 58.9332},
{"Ni", 58.6934},
{"Cu", 63.5460},
{"Zn", 65.3800},
{"Ga", 69.7230},
{"Ge", 72.6300},
{"As", 74.9216},
{"Se", 78.9710},
{"Br", 79.9040},
{"Kr", 83.7980},
{"Rb", 85.4678},
{"Sr", 87.6200},
{"Y", 88.9058},
{"Zr", 91.2240},
{"Nb", 92.9064},
{"Mo", 95.9500},
{"Tc", 97.0000},
{"Ru", 101.0700},
{"Rh", 102.9055},
{"Pd", 106.4200},
{"Ag", 107.8682},
{"Cd", 112.4140},
{"In", 114.8180},
{"Sn", 118.7100},
{"Sb", 121.7600},
{"Te", 127.6000},
{"I", 126.9045},
{"Xe", 131.2930},
{"Cs", 132.9055},
{"Ba", 137.3270},
{"La", 138.9055},
{"Ce", 140.1160},
{"Pr", 140.9077},
{"Nd", 144.2420},
{"Pm", 145.0000},
{"Sm", 150.3600},
{"Eu", 151.9640},
{"Gd", 157.2500},
{"Tb", 158.9254},
{"Dy", 162.5000},
{"Ho", 164.9303},
{"Er", 167.2590},
{"Tm", 168.9342},
{"Yb", 173.0540},
{"Lu", 174.9668},
{"Hf", 178.4900},
{"Ta", 180.9479},
{"W", 183.8400},
{"Re", 186.2070},
{"Os", 190.2300},
{"Ir", 192.2170},
{"Pt", 195.0840},
{"Au", 196.9666},
{"Hg", 200.5920},
{"Tl", 204.3800},
{"Pb", 207.2000},
{"Bi", 208.9804},
{"Po", 209.0000},
{"At", 210.0000},
{"Rn", 222.0000},
{"Fr", 223.0000},
{"Ra", 226.0000},
{"Ac", 227.0000},
{"Th", 232.0377},
{"Pa", 231.0359},
{"U", 238.0289},
{"Np", 237.0000},
{"Pu", 244.0000},
{"Am", 243.0000},
{"Cm", 247.0000},
{"Bk", 247.0000},
{"Cf", 251.0000},
{"Es", 252.0000},
{"Fm", 257.0000},
{"Md", 258.0000},
{"No", 259.0000},
{"Lr", 262.0000},
{"Rf", 267.0000},
{"Db", 270.0000},
{"Sg", 271.0000},
{"Bh", 270.0000},
{"Hs", 277.0000},
{"Mt", 276.0000},
{"Ds", 281.0000},
{"DLO", 1000000.0000},
});


int GetAtomicNumber(std::string symbol)
{

    try {
        return SymbolToNumber[symbol];
    }
    catch (const std::out_of_range& oor) {
        throw RmgFatalException() << "Unknown atomic symbol " << symbol << " in " << __FILE__ << " at line " << __LINE__ << "\n";
    }

}

double GetAtomicMass(std::string symbol)
{

    try {
        return SymbolToMass[symbol];
    }
    catch (const std::out_of_range& oor) {
        throw RmgFatalException() << "Unknown atomic symbol " << symbol << " in " << __FILE__ << " at " << __LINE__ << "\n";
    }

}

const char * GetAtomicSymbol(int number)
{
    if((number < 1) || (number > 110))
        throw RmgFatalException() << "Error in " << __FILE__ << " at line " << __LINE__ << ". Atomic numbers must be between 1 and 110.\n";

    for ( auto it = SymbolToNumber.begin(); it != SymbolToNumber.end(); ++it ) {
        if(it->second == number) {
            return it->first.c_str();
        }
    }
    return "";
}    
