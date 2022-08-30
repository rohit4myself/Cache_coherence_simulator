#include <stdio.h>
#include "pin.H"
#include <iostream>

FILE * trace;
PIN_LOCK pinLock;
static unsigned long long machine_access = 0, count = 0;

void countMemoryAccess(int memSize, int tid, unsigned long long memoryAddress,int j){
    int quotient_8, quotient_4, quotient_2, quotient_1;
    quotient_8= quotient_4= quotient_2= quotient_1 =0;
			
	quotient_8 = memSize / 8;
	
	//printing memory requests into the trace file for stride of 8
	for(int i =0; i< quotient_8; i++){
		PIN_GetLock(&pinLock, tid+1);
	    	fprintf(trace,"%d %d %llu %llu \n ", tid,j,count,memoryAddress);
	    	count++;
		fflush(trace);
		PIN_ReleaseLock(&pinLock);	
		//Incrementing memory address by 8
		memoryAddress += 8;
	}

	if(memSize % 8){
		memSize = memSize - quotient_8*8;
		quotient_4 = memSize/4;
		
		//printing memory requests into the trace file for stride of 4
		for(int i =0; i< quotient_4; i++){
			PIN_GetLock(&pinLock, tid+1);
		    	fprintf(trace,"%d %d %llu %llu\n", tid,j,count,memoryAddress);
		    	count++;
			fflush(trace);
			PIN_ReleaseLock(&pinLock);	
			//Incrementing memory address by 4
			memoryAddress += 4;
		}
		
		if(memSize %4){
			memSize = memSize - quotient_4*4;
			quotient_2 = memSize/2;
			
			//printing memory requests into the trace file for stride of 2
			for(int i =0; i< quotient_2; i++){
				PIN_GetLock(&pinLock, tid+1);
			    	fprintf(trace,"%d %d %llu %llu\n", tid,j,count,memoryAddress);
			    	count++;
				fflush(trace);
				PIN_ReleaseLock(&pinLock);	
				//Incrementing memory address by 2
				memoryAddress += 2;
			}
			
			if(memSize %2){
				quotient_1=1;
				PIN_GetLock(&pinLock, tid+1);
			    	fprintf(trace,"%d %d %llu %llu\n", tid,j,count,memoryAddress);
			    	count++;
				fflush(trace);
				PIN_ReleaseLock(&pinLock);
				memoryAddress += 1;
			}
		}
	}
	UINT64 total_split_accesses = quotient_1 + quotient_2 + quotient_4 + quotient_8;
    
	PIN_GetLock(&pinLock, tid+1);
	machine_access+= total_split_accesses;
	total_split_accesses =0;
	PIN_ReleaseLock(&pinLock);
}

// Print a memory read record
VOID RecordMachineAccess(VOID * ip, THREADID tid, int memSize, void * addr, int j)
{    
    // Printing the which thread requests which memory address -- commented as it consumes time.	
    //fprintf(trace,"threadId: %d  MemoryAddress: %p\n", tid, addr);
    
    //debugging
    //fprintf(stdout, "addr: %llu\n",*addr);
    unsigned long long memoryAddress = (unsigned long long)addr;
    unsigned block_number = memoryAddress/64;
    int block_end = (block_number * 64 + 64) - memoryAddress;
    
    if (block_end >= memSize){
        countMemoryAccess(memSize,tid,memoryAddress,j);
    }
    else{
        countMemoryAccess(block_end,tid,memoryAddress,j);
        countMemoryAccess(memSize - block_end,tid,memoryAddress + block_end,j);
    }
   
}

// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PIN_GetLock(&pinLock, tid+1);
    fprintf(stdout, "thread begin %d\n",tid);
    fflush(stdout);
    PIN_ReleaseLock(&pinLock);
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    PIN_GetLock(&pinLock, tid+1);
    fprintf(stdout, "thread end %d code %d\n",tid, code);
    fflush(stdout);
    PIN_ReleaseLock(&pinLock);
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
	//To get number of memory operands present in the instruction
	UINT32 memOperands = INS_MemoryOperandCount(ins);
	// Variable to store the size of data requested by the instruction 
	UINT32 memSize;

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
    	memSize = INS_MemoryOperandSize(ins, memOp);
	 
	//Calling RecordMachine Access routine (analysis routine) if instruction perfroms memory read.
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMachineAccess, 
                IARG_INST_PTR,
                IARG_THREAD_ID, 
                IARG_UINT32, memSize,//passed the memory size to the analysis routine
                IARG_MEMORYOP_EA, memOp, //passed the memory address referenced 
                IARG_UINT32,0,
                IARG_END);
        }
       
        //Calling RecordMachine Access routine (analysis routine) if instruction perfroms memory write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMachineAccess,
                IARG_INST_PTR,
                IARG_THREAD_ID,
                IARG_UINT32, memSize,
                IARG_MEMORYOP_EA, memOp,
                IARG_UINT32,1,
                IARG_END);
        }
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
	//Printing out the result on the command line
	fprintf(stdout, "Total Machine access  %llu\n",machine_access);
    fprintf(trace, "#eof\n");
    fclose(trace);
    fprintf(stdout, "Output trace file generated - addrtrace.txt\n");
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints the IPs of every instruction executed\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
//	char inputname[100];
//	sprintf(inputname, "input/thread_memory_trace_%s.txt",argv[1]);
	trace = fopen("addrtrace.txt", "w");
    // trace = fopen("thread_memory_trace.txt", "w");
    
    PIN_InitLock(&pinLock);

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
