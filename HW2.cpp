#include <iostream>
#include <fstream>
#include <set>
#include "pin.H"
#include <cstdlib>
#include "saturation_counter.h"

/* Macro and type definitions */
#define BILLION 1000000000

/* Global variables */
std::ostream * out = &cerr;

typedef std::set<ADDRINT> FOOTPRINT;

UINT64 icount = 0; //number of dynamically executed instructions
UINT64 maxIns = 0;
UINT64 fastForwardIns = 0;
UINT64 branches_executed = 0;
UINT64 correct_prediction = 0;
/* Command line switches */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "", "specify file name for HW1 output");
KNOB<UINT64> KnobFastForward(KNOB_MODE_WRITEONCE, "pintool", "f", "0", "number of instructions to fast forward in billions");


/* Utilities */

/* Print out help message. */
INT32 Usage()
{
	cerr << "CS422 Homework 2" << endl;
	cerr << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

VOID InsCount(void)
{
	icount++;
}

ADDRINT FastForward(void)
{
	return (icount >= fastForwardIns && icount < maxIns);
}

ADDRINT Terminate(void)
{
        return (icount >= maxIns);
}


VOID StatDump(void)
{	
	*out << "===============================================" << endl;
	*out << "Number of Instructions executed " << icount << endl;
	*out << "Actual Instructions executed " << (icount - fastForwardIns) << endl;
	*out << "Number of branches executed "<< branches_executed << endl;
	*out << "Number of branches correctly predicted "<< correct_prediction << endl;
    *out << "===============================================" << endl;
	exit(0);
}

VOID BranchAnalysis(ADDRINT pc, bool branch_taken, ADDRINT target_addr)
{
	branches_executed++;
	bool prediction;
	if(target_addr < pc) prediction = TRUE;
	else prediction = FALSE;
	if(prediction == branch_taken){
		correct_prediction++;
	}
}

/* Instruction instrumentation routine */
VOID Instruction(INS ins, VOID *v)
{

	INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) Terminate, IARG_END);
        INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR) StatDump, IARG_END);
	
	if (INS_IsBranch(ins) && INS_HasFallThrough(ins)){
		INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR) BranchAnalysis,
			IARG_INST_PTR, 
			IARG_BRANCH_TAKEN,
			IARG_BRANCH_TARGET_ADDR,
			IARG_END);
	}

	/* Called for each instruction */
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) InsCount, IARG_END);
}

/* Fini routine */
VOID Fini(INT32 code, VOID * v)
{
	*out << "===============================================" << endl;
	*out << "Number of Instructions executed " << icount << endl;
	*out << "Actual Instructions executed " << (icount - fastForwardIns) << endl;
	*out << "Number of branches executed "<< branches_executed << endl;
	*out << "Number of branches correctly predicted "<< correct_prediction << endl;
    *out << "===============================================" << endl;
}

int main(int argc, char *argv[])
{
	// Initialize PIN library. Print help message if -h(elp) is specified
	// in the command line or the command line is invalid 
	if (PIN_Init(argc, argv))
		return Usage();

	/* Set number of instructions to fast forward and simulate */
	fastForwardIns = KnobFastForward.Value() * BILLION;
	maxIns = fastForwardIns + BILLION;

	string fileName = KnobOutputFile.Value();

	if (!fileName.empty())
		out = new std::ofstream(fileName.c_str());

	// Register function to be called to instrument instructions
	INS_AddInstrumentFunction(Instruction, 0);

	// Register function to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

	cerr << "===============================================" << endl;
	cerr << "This application is instrumented by HW2" << endl;
	if (!KnobOutputFile.Value().empty())
		cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
	cerr << "===============================================" << endl;

	// Start the program, never returns
	PIN_StartProgram();

	return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
