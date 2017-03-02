#include <iostream>
#include <fstream>
#include <set>
#include "pin.H"
#include <cstdlib>
/* Macro and type definitions */
#define BILLION 1000000000
#define WIDTH 512
/* Global variables */
std::ostream * out = &cerr;

typedef std::set<ADDRINT> FOOTPRINT;

UINT64 icount = 0; //number of dynamically executed instructions
UINT64 maxIns = 0;
UINT64 ins_exec = 0;
UINT64 fastForwardIns = 0;
UINT64 branches_executed = 0;
UINT64 forward_branches = 0;
UINT64 backward_branches = 0;
UINT64 FNBT_correct_forward = 0;
UINT64 bimodal_correct_forward = 0;
UINT64 FNBT_correct_backward = 0;
UINT64 bimodal_correct_backward = 0;
/* Command line switches */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "", "specify file name for HW1 output");
KNOB<UINT64> KnobFastForward(KNOB_MODE_WRITEONCE, "pintool", "f", "0", "number of instructions to fast forward in billions");

enum TwoBitState {STRONGLY_NOT_TAKEN, WEAKLY_NOT_TAKEN, WEAKLY_TAKEN,
    STRONGLY_TAKEN};
class TwoBitSaturationCounter{
public:
	TwoBitSaturationCounter(){counter = STRONGLY_NOT_TAKEN;}
	void Update(bool branch_taken);
	bool prediction(void);
private:
	TwoBitState counter;
};

void TwoBitSaturationCounter::Update(bool branch_taken){
	TwoBitState temp = STRONGLY_NOT_TAKEN;
	if(counter == STRONGLY_NOT_TAKEN){
		if (branch_taken) temp = WEAKLY_NOT_TAKEN;
		else temp = STRONGLY_NOT_TAKEN;
	}
	else if(counter == WEAKLY_NOT_TAKEN){
		if (branch_taken) temp = WEAKLY_TAKEN;
		else temp = STRONGLY_NOT_TAKEN;
	}
	else if(counter == WEAKLY_TAKEN){
		if (branch_taken) temp = STRONGLY_TAKEN;
		else temp = WEAKLY_NOT_TAKEN;
	}
	else if(counter == STRONGLY_TAKEN){
		if (!branch_taken) temp = WEAKLY_TAKEN;
		else temp = STRONGLY_TAKEN;
	}
	counter = temp;
}

bool TwoBitSaturationCounter::prediction(void){
	if((counter == STRONGLY_NOT_TAKEN)||(counter == WEAKLY_NOT_TAKEN))
		return false;
	else return true;
}

class BimodalCounter{
public:
	void Update(ADDRINT pc, bool branch_taken);
	bool pred(ADDRINT pc);
private:
	TwoBitSaturationCounter PHT[WIDTH];
};

void BimodalCounter::Update(ADDRINT pc, bool branch_taken){
	int offset = (pc%WIDTH);
	PHT[offset].Update(branch_taken);
}
bool BimodalCounter::pred(ADDRINT pc){
	int offset = (pc%WIDTH);
	return PHT[offset].prediction();
}


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

void stat_temp(int fc,int bc){
	*out << (double)(branches_executed - (fc + bc))*100/branches_executed << "    " << (double)(forward_branches - fc)*100/forward_branches << "    " << (double)(backward_branches - bc)*100/backward_branches << endl;
}
void stats(void){
	ins_exec = (icount - fastForwardIns);
	*out << "===============================================" << endl;
	*out << "Number of Instructions executed " << icount << endl;
	*out << "Actual Instructions executed " << ins_exec << endl;
	*out << "Number of branches executed "<< branches_executed << endl;
	*out << "MisPrediction Table : " << endl;
	*out << "   Overall    " << "Forward   " << "Backward  " << endl;
	*out << "A. ";
	stat_temp(FNBT_correct_forward,FNBT_correct_backward);
	*out << "B. ";
	stat_temp(bimodal_correct_forward,bimodal_correct_backward);
    *out << "===============================================" << endl;
}

VOID StatDump(void)
{	
	stats();
	exit(0);
}
BimodalCounter Bimodalcounter;
void FNBT_Predictor(bool branch_taken, bool forward){
	bool prediction = !forward;
	if((prediction == branch_taken)&&(forward)) FNBT_correct_forward++;
	else if ((prediction == branch_taken)&&(!forward)) FNBT_correct_backward++;
}

void Bimodal_predictor(ADDRINT pc, bool branch_taken,bool forward){
	if((Bimodalcounter.pred(pc) == branch_taken)&&(forward)) bimodal_correct_forward++;
	else if((Bimodalcounter.pred(pc) == branch_taken)&&(!forward)) bimodal_correct_backward++;
	Bimodalcounter.Update(pc,branch_taken);
}


VOID BranchAnalysis(ADDRINT pc, bool branch_taken, ADDRINT target_addr)
{
	branches_executed++;
	bool forward = false;
	if(target_addr > pc) {
		forward = true;
		forward_branches++;
	}
	else{
		forward = false;
		backward_branches++;
	}
	FNBT_Predictor(branch_taken,forward);
	Bimodal_predictor(pc,branch_taken,forward);
	
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
	stats();
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
