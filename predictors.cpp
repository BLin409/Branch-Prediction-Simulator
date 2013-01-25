/**
 * CS 320 - Computer System III
 * Project 1 - Branch Instruction Simulator
 *
 * @author Cheng Lin
 */

#include <fstream>
#include <iostream>

using namespace std;

int main (int argc, char *argv[])
{
    bool predictor1bit(int * table, int index, char taken);
    bool predictor2bit(int * table, int index, char taken);
    bool tournamentPred(int &entry, int bPred, int gPred, char taken);

    if (argc != 3) {
        cerr << "ERROR: Please provide a file names for both input and output files and nothing else\n";
        return 0;
    }

    ifstream fileIn(argv[1]);
    if (!fileIn.is_open()) {
        cerr << "ERROR: The input file does not exist!\n";
        return 1;
    }

    ofstream fileOut(argv[2]);
    if (!fileOut.is_open()) {
        cerr << "ERROR: The output file does not exist!\n";
        return 2;
    }

	// Prediction table for 1 bit and 2 bit Bimodal Predictor
	// Third least significant bit is for 1 bit Bimodal Predictor
	// First and second least significant bit for 2 bit Bimodal Predictor
    int *table8 = new int[8];
    int *table16 = new int[16];
    int *table32 = new int[32];
    int *table128 = new int[128];
    int *table256 = new int[256];
    int *table512 = new int[512];
    int *table1024 = new int[1024];
	
	// Gshare Predictor
	// Use third and fourth least significant bits of gtable[8] for tournament predictor
    int globalReg = 0;
    int **gtable = new int*[9];		// Prediction table for 2 to 10 bit global register
    for (int i = 0; i < 9; i++) {
        gtable[i] = new int[1024];
    }  

	// Initialize prediction table
	// 1 bit Bimodal Prediction Table set to Taken (1)
	// 2 bit Bimodal Prediction Table set to Strongly Taken (11)
	// Gshare Prediction Table also set to Strongly Taken (11)
	// Tournament Prediction Table set to Prefer Gshare (00)
    for (int i = 0; i < 8; i++) {
        table8[i] = table16[i] = table32[i] = table128[i] = table256[i] = table512[i] = table1024[i] = 7;
        gtable[0][i] = gtable[1][i] = gtable[2][i] = gtable[3][i] = gtable[4][i] = 
        gtable[5][i] = gtable[6][i] = gtable[7][i] = gtable[8][i] = 3;
    }
    for (int i = 8; i < 16; i++) {
        table16[i] = table32[i] = table128[i] = table256[i] = table512[i] = table1024[i] = 7;
        gtable[0][i] = gtable[1][i] = gtable[2][i] = gtable[3][i] = gtable[4][i] = 
        gtable[5][i] = gtable[6][i] = gtable[7][i] = gtable[8][i] = 3;
    }
    for (int i = 16; i < 32; i++) {
        table32[i] = table128[i] = table256[i] = table512[i] = table1024[i] = 7;
        gtable[0][i] = gtable[1][i] = gtable[2][i] = gtable[3][i] = gtable[4][i] = 
        gtable[5][i] = gtable[6][i] = gtable[7][i] = gtable[8][i] = 3;
    }
    for (int i = 32; i < 128; i++) {
        table128[i] = table256[i] = table512[i] = table1024[i] = 7;
        gtable[0][i] = gtable[1][i] = gtable[2][i] = gtable[3][i] = gtable[4][i] = 
        gtable[5][i] = gtable[6][i] = gtable[7][i] = gtable[8][i] = 3;
    }
    for (int i = 128; i < 256; i++) {
        table256[i] = table512[i] = table1024[i] = 7;
        gtable[0][i] = gtable[1][i] = gtable[2][i] = gtable[3][i] = gtable[4][i] = 
        gtable[5][i] = gtable[6][i] = gtable[7][i] = gtable[8][i] = 3;
    }
    for (int i = 256; i < 512; i++) {
        table512[i] = table1024[i] = 7;
        gtable[0][i] = gtable[1][i] = gtable[2][i] = gtable[3][i] = gtable[4][i] = 
        gtable[5][i] = gtable[6][i] = gtable[7][i] = gtable[8][i] = 3;
   }
    for (int i = 512; i < 1024; i++) {
        table1024[i] = 7;
        gtable[0][i] = gtable[1][i] = gtable[2][i] = gtable[3][i] = gtable[4][i] = 
        gtable[5][i] = gtable[6][i] = gtable[7][i] = gtable[8][i] = 3;
    }

	// Create Counters
    int alwaysT = 0;
    int alwaysNT = 0;
    int bimodal1bit8 = 0, bimodal1bit16 = 0, bimodal1bit32 = 0, bimodal1bit128 = 0, 
        bimodal1bit256 = 0, bimodal1bit512 = 0, bimodal1bit1024 = 0;
    int bimodal2bit8 = 0, bimodal2bit16 = 0, bimodal2bit32 = 0, bimodal2bit128 = 0, 
        bimodal2bit256 = 0, bimodal2bit512 = 0, bimodal2bit1024 = 0;
    int gCounter[9] = {0};
    int tCounter = 0;
    int iCount = 0;

    while (!fileIn.eof()) 
	{
		// Reading Trace File
		// format: address(hex)\ T/NT(actual branch direction)
        long long address;
        fileIn >> hex >> address;
        if (fileIn.fail()) break;

        char branchTaken;
        fileIn >> branchTaken;
        if (fileIn.fail()) break;

        fileIn.ignore(256, '\n');

        int index = address % 1024;
		// Tournament Predictor
        tCounter += tournamentPred(gtable[8][index], table1024[index] % 4, gtable[8][index ^ globalReg & 1023] % 4, branchTaken);

		// Gshare Predictor (2 - 10 bits global register)
        for (int i = 2; i <= 10; i++) {
            int cutOff = 1 << i; //Use to obtain bits from global register
            gCounter[i - 2] += predictor2bit(gtable[i - 2], (index) ^ (globalReg % cutOff), branchTaken);
        }

        globalReg = globalReg << 1;
        if (branchTaken == 'T') {
            alwaysT++;
            globalReg++;
        } else {
            alwaysNT++;
        }
        globalReg &= 1023; //Keep global register to 10 bits

		//1 bit Bimodal Predictor with various table size
        bimodal1bit8 += predictor1bit(table8, address % 8, branchTaken);
        bimodal1bit16 += predictor1bit(table16, address % 16, branchTaken);
        bimodal1bit32 += predictor1bit(table32, address % 32, branchTaken);
        bimodal1bit128 += predictor1bit(table128, address % 128, branchTaken);
        bimodal1bit256 += predictor1bit(table256, address % 256, branchTaken);
        bimodal1bit512 += predictor1bit(table512, address % 512, branchTaken);
        bimodal1bit1024 += predictor1bit(table1024, address % 1024, branchTaken);

		//2 bit Bimodal Predictor with various table size
        bimodal2bit8 += predictor2bit(table8, address % 8, branchTaken);
        bimodal2bit16 += predictor2bit(table16, address % 16, branchTaken);
        bimodal2bit32 += predictor2bit(table32, address % 32, branchTaken);
        bimodal2bit128 += predictor2bit(table128, address % 128, branchTaken);
        bimodal2bit256 += predictor2bit(table256, address % 256, branchTaken);
        bimodal2bit512 += predictor2bit(table512, address % 512, branchTaken);
        bimodal2bit1024 += predictor2bit(table1024, address % 1024, branchTaken);

        iCount++;	//Instruction Count
    }

	//Calculate the accuracy rate of each predictor
    int accurateT = (int)((float)alwaysT * 100 / iCount + 0.5f);
    int accurateNT = (int)((float)alwaysNT * 100 / iCount + 0.5f);
    int accurate1bit8 = (int)((float)bimodal1bit8 * 100 / iCount + 0.5f);
    int accurate1bit16 = (int)((float)bimodal1bit16 * 100 / iCount + 0.5f);
    int accurate1bit32 = (int)((float)bimodal1bit32 * 100 / iCount + 0.5f);
    int accurate1bit128 = (int)((float)bimodal1bit128 * 100 / iCount + 0.5f);
    int accurate1bit256 = (int)((float)bimodal1bit256 * 100 / iCount + 0.5f);
    int accurate1bit512 = (int)((float)bimodal1bit512 * 100 / iCount + 0.5f);
    int accurate1bit1024 = (int)((float)bimodal1bit1024 * 100 / iCount + 0.5f);
    int accurate2bit8 = (int)((float)bimodal2bit8 * 100 / iCount + 0.5f);
    int accurate2bit16 = (int)((float)bimodal2bit16 * 100 / iCount + 0.5f);
    int accurate2bit32 = (int)((float)bimodal2bit32 * 100 / iCount + 0.5f);
    int accurate2bit128 = (int)((float)bimodal2bit128 * 100 / iCount + 0.5f);
    int accurate2bit256 = (int)((float)bimodal2bit256 * 100 / iCount + 0.5f);
    int accurate2bit512 = (int)((float)bimodal2bit512 * 100 / iCount + 0.5f);
    int accurate2bit1024 = (int)((float)bimodal2bit1024 * 100 / iCount + 0.5f);
    int accurateG[9] = {0};
    int accurateTour = (int)((float)tCounter * 100 / iCount + 0.5f);

    fileOut << accurateT << endl;
    fileOut << accurateNT << endl;
    fileOut << accurate1bit8 << " " << accurate1bit16 << " " << accurate1bit32 << " " << accurate1bit128 
         << " " << accurate1bit256 << " " << accurate1bit512 << " " << accurate1bit1024 << endl;
    fileOut << accurate2bit8 << " " << accurate2bit16 << " " << accurate2bit32 << " " << accurate2bit128 
         << " " << accurate2bit256 << " " << accurate2bit512 << " " << accurate2bit1024 << endl;
    for (int i = 2; i <= 9; i++) {
        accurateG[i - 2] = (int)((float)gCounter[i - 2] * 100 / iCount + 0.5f);
        fileOut << accurateG[i - 2] << " ";
    }
    accurateG[10 - 2] = (int)((float)gCounter[10 - 2] * 100 / iCount + 0.5f);
    fileOut << accurateG[10 - 2] << endl;
    fileOut << accurateTour << endl;

    fileIn.close();
    fileOut.close();
	
	delete [] table8;
	delete [] table16;
	delete [] table32;
	delete [] table128;
	delete [] table256;
	delete [] table512;
	delete [] table1024;
    for (int i = 0; i < 9; i++) {
        delete [] gtable[i];
    }  
	delete [] gtable;
}

/**
 * Predictor using 1 bit saturating counters
 * Update the entry in table base on the actual branch direction
 * (1 - Taken, 0 - Non-Taken)
 *
 * @param table prediction table
 * @param index locate the entry in prediction table
 * @param taken actual branch direction
 * @return whether entry in the table matches actual branch direction
 */
bool predictor1bit(int * table, int index, char taken) {
    bool correct = 0;
    if (taken == 'T') {
        if (table[index] >= 4) {
            correct = 1;
        } else {
            table[index] += 4;
        }
    } else {
        if (table[index] < 4) {
            correct = 1;
        } else {
            table[index] -= 4;
        }
    }
    return correct;
}

/**
 * Predictor using 2 bit saturating counters
 * Update the entry in table base on the actual branch direction
 * (11 - Strongly Taken, 10 - Weakly Taken, 01 - Weakly Non-Taken, 00 - Strongly Non-Taken)
 *
 * @param table prediction table
 * @param index locate the entry in prediction table
 * @param taken actual branch direction
 * @return whether entry in the table matches actual branch direction
 */
bool predictor2bit(int * table, int index, char taken) {
    bool correct = 0;
    int state = table[index] % 4;
    if (taken == 'T') {
        switch (state) {
            case 0:
            case 1:
                table[index]++;
                break;
            case 2:
                table[index]++;
            case 3:
                correct = 1;
                break;
        }
    } else {
        switch (state) {
            case 0:
                correct = 1;
                break;
            case 1:
                correct = 1;
            case 2:
            case 3:
                table[index]--;
                break;
        }
    }
    return correct;
}

/**
 * Tournament Predictor
 * Update the entry in selector base on the actual branch direction
 * (11 - Strongly Prefer 2 bits Bimodal, 10 - Weakly Prefer 2 bits Bimodal, 
 *  01 - Weakly Prefer Gshare, 00 - Strongly Prefer Gshare)
 *
 * @param entry entry of the selector
 * @param bPred prediction made by 2 bits Bimodal
 * @param gPred prediction made by Gshare
 * @return whether entry in the prefered table matches actual branch direction
 */
bool tournamentPred(int &entry, int bPred, int gPred, char taken) {
    bool correct = 0;
    int state = entry & 12;
    if (taken == 'T') {
        switch (state) {
            case 0:
            case 4:
                if (gPred >= 2) {
                    if (bPred <= 1) {
                        entry = entry % 4;
                    }
                    correct = 1;
                } else if (bPred > 1) {
                    entry += 4;
                }
                break;
            case 8:
            case 12:
                if (bPred >= 2) {
                    if (gPred <= 1) {
                        entry = 12 + entry % 4;
                    }
                    correct = 1;
                } else if (gPred >= 2) {
                    entry -= 4;
                }
                break;
        } 
    } else {
        switch (state) {
            case 0:
            case 4:
                if (gPred <= 1) {
                    if (bPred >= 2) {
                        entry = entry % 4;
                    }
                    correct = 1;
                } else if (bPred <= 1) {
                    entry += 4;
                }
                break;
            case 8:
            case 12:
                if (bPred <= 1) {
                    if (gPred >= 2) {
                        entry = 12 + entry % 4;
                    }
                    correct = 1;
                } else if (bPred > 1 && gPred <= 1) {
                    entry -= 4;
                }
                break;
        }
    }
    return correct;
}
