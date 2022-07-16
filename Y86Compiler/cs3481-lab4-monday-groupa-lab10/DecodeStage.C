#include <string>
#include <cstdint>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "E.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "DecodeStage.h"
#include "Status.h"
#include "Debug.h"
#include "ExecuteStage.h"
#include "Instructions.h"
#include "MemoryStage.h"

bool DecodeStage::doClockLow(PipeReg ** pregs, Stage ** stages) {

    // Pointers to the registers
    E * ereg = (E *) pregs[EREG];
    D * dreg = (D *) pregs[DREG];
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];

    // Clearing valA & valB 
    uint64_t valA = 0, valB = 0;
    uint64_t dstE = 0Xf, dstM = 0xf;

    // stat, icode, ifun, valC COME FROM D REGISTER
    uint64_t stat = dreg->getstat()->getOutput();
    uint64_t icode = dreg->geticode()->getOutput();
    uint64_t ifun = dreg->getifun()->getOutput();
    uint64_t valC = dreg->getvalC()->getOutput();

    // Setting values according to the decode stage
    bool imem = false;
    RegisterFile * regFile = RegisterFile::getInstance();

    // Get the values to use in decode
    uint64_t rA = dreg->getrA()->getOutput();
    uint64_t rB = dreg->getrB()->getOutput();

    // Multiplexes the values
    d_srcA = decodeA(icode, rA);
    d_srcB = decodeB(icode, rB);
    dstE = decodeDSTE(icode, rB);
    dstM = decodeDSTM(icode, rA);
    uint64_t rvalA = regFile->readRegister(d_srcA, imem);
    uint64_t rvalB = regFile->readRegister(d_srcB, imem);

    valA = forwardValA(dreg, mreg, wreg, rvalA, d_srcA, stages);
    valB = forwardValB(mreg, wreg, rvalB, d_srcB, stages);

    // Call to calculateControlSignals to set E_bubble
    E_bubble = calculateControlSignals(ereg, stages);

    // Called at the end here
    setEInput(ereg, stat, icode, ifun, valC, valA, valB, dstE, dstM, d_srcA, d_srcB);

    return false;
}

void DecodeStage::doClockHigh(PipeReg ** pregs) {
    // Pointers to the E registers
    E * ereg = (E *) pregs[EREG];

    // Sets the state of the register based on the E_bubble boolean
    if (E_bubble) bubbleE(ereg);
    else normalE(ereg);
}

void DecodeStage::setEInput(E * ereg, uint64_t stat, uint64_t icode, uint64_t ifun, uint64_t valC,
        uint64_t valA, uint64_t valB, uint64_t dstE, 
        uint64_t dstM, uint64_t srcA, uint64_t srcB) {

    // Sets the fields in the E register
    ereg->getstat()->setInput(stat);
    ereg->geticode()->setInput(icode);
    ereg->getifun()->setInput(ifun);
    ereg->getvalC()->setInput(valC);
    ereg->getvalA()->setInput(valA);
    ereg->getvalB()->setInput(valB);
    ereg->getdstE()->setInput(dstE);
    ereg->getdstM()->setInput(dstM);
    ereg->getsrcA()->setInput(srcA);
    ereg->getsrcB()->setInput(srcB);
}

/* Decodes Instruction & sets srcA for use in doClockLow
   Instruction Code Key
   2 = register to register move
   4 = register to memory move
   6 = Operation
   8 = call
   9 = return
   10 = Push
   11 = pop
   */
uint64_t DecodeStage::decodeA(uint64_t icode, uint64_t rA) {

    if(icode == IRRMOVQ || icode == IRMMOVQ || icode == IOPQ || icode == IPUSHQ) return rA;
    else if (icode == IPOPQ || icode == IRET) return RSP;
    else return RNONE;
}

/* Decodes Instruction & sets srcB for use in doClockLow
   Instruction Code Key
   2 = register to register move
   4 = register to memory move
   5 = memory to register move
   6 = Operation
   8 = call
   9 = return
   10 = Push
   11 = pop
   */
uint64_t DecodeStage::decodeB(uint64_t icode, uint64_t rB) {

    if(icode == IOPQ || icode == IRMMOVQ || icode == IMRMOVQ) return rB;
    else if (icode == IPUSHQ || icode == IPOPQ || icode == ICALL || icode == IRET) return RSP;
    else return RNONE;
}

/* Decodes Instruction & sets dstE for use in doClockLow
   Instruction Code Key
   2 = register to register move
   4 = register to memory move
   5 = memory to register move
   6 = Operation
   8 = call
   9 = return
   10 = Push
   11 = pop
   */
uint64_t DecodeStage::decodeDSTE(uint64_t icode, uint64_t rB) {

    if(icode == IRRMOVQ || icode == IIRMOVQ || icode == IOPQ) return rB;
    else if (icode == IPUSHQ || icode == IPOPQ || icode == ICALL || icode == IRET) return RSP;
    else return RNONE;
}

/* Decode instruction & set dstM for use in doClockLow
   Decodes Instruction & sets dstE for use in doClockLow
   Instruction Code Key
   2 = register to register move
   4 = register to memory move
   5 = memory to register move
   6 = Operation
   8 = call
   9 = return
   10 = Push
   11 = pop
   */
uint64_t DecodeStage::decodeDSTM(uint64_t icode, uint64_t rA) {

    if(icode == IMRMOVQ || icode == IPOPQ) return rA;
    else return RNONE;
}

// Forwards valA from Decode to Execute
// Decides what valA needs to be set to
uint64_t DecodeStage::forwardValA(D * dreg, M * mreg, W * wreg, uint64_t rvalA, uint64_t srcA, Stage ** stages) {

    // Prevents the this method from selecting a valE that id doesn't actually use
    if (dreg->geticode()->getOutput() == ICALL || dreg->geticode()->getOutput() == IJXX) return dreg->getvalP()->getOutput();

    if (srcA == RNONE) return 0;

    ExecuteStage * ES = (ExecuteStage *) stages[ESTAGE];
    MemoryStage * MS = (MemoryStage *) stages[MSTAGE];

    if (srcA == ES->getE_dstE()) return ES->getE_valE();
    else if (srcA == mreg->getdstM()->getOutput()) return MS->getM_valM();
    else if (srcA == mreg->getdstE()->getOutput()) return mreg->getvalE()->getOutput();
    else if (srcA == wreg->getdstM()->getOutput()) return wreg->getvalM()->getOutput();
    else if (srcA == wreg->getdstE()->getOutput()) return wreg->getvalE()->getOutput();
    else return rvalA;
}

// Forwards valB from Decode to Execute
// Decides what valB needs to be set to
uint64_t DecodeStage::forwardValB(M * mreg, W * wreg, uint64_t rvalB, uint64_t srcB, Stage ** stages) {

    // Prevents the this method from selecting a valE that id doesn't actually use
    if (srcB == RNONE) return 0;

    ExecuteStage * ES = (ExecuteStage *) stages[ESTAGE];
    MemoryStage * MS = (MemoryStage *) stages[MSTAGE];

    if (srcB == ES->getE_dstE()) return ES->getE_valE();
    else if (srcB == mreg->getdstM()->getOutput()) return MS->getM_valM();
    else if (srcB == mreg->getdstE()->getOutput()) return mreg->getvalE()->getOutput();
    else if (srcB == wreg->getdstM()->getOutput()) return wreg->getvalM()->getOutput();
    else if (srcB == wreg->getdstE()->getOutput()) return wreg->getvalE()->getOutput();
    else return rvalB;
}

// Getter for d_srcA
uint64_t DecodeStage::getd_srcA() {
    return d_srcA;
}

// Getter for d_srcB
uint64_t DecodeStage::getd_srcB() {
    return d_srcB;
}

// Sets the E_bubble boolean based on the state of the D and E registers
bool DecodeStage::calculateControlSignals(E * ereg, Stage ** stages) {
    ExecuteStage * ES = (ExecuteStage *) stages[ESTAGE];

    if ((ereg->geticode()->getOutput() == IJXX && (!ES->getE_Cnd())) ||       
            ((ereg->geticode()->getOutput() == IMRMOVQ || ereg->geticode()->getOutput() == IPOPQ) &&
             (ereg->getdstM()->getOutput() == d_srcA || ereg->getdstM()->getOutput() == d_srcB))) return true;
    return false;
}

// Simulates passing the correct values to the ExecuteStage
void DecodeStage::normalE(E * ereg) {
    ereg->getstat()->normal();
    ereg->geticode()->normal();
    ereg->getifun()->normal();
    ereg->getvalC()->normal();
    ereg->getvalA()->normal();
    ereg->getvalB()->normal();
    ereg->getdstE()->normal();
    ereg->getdstM()->normal();
    ereg->getsrcA()->normal();
    ereg->getsrcB()->normal();
}

// Simulates passing a bubble into the ExecuteStage
void DecodeStage::bubbleE(E * ereg) {
    ereg->getstat()->bubble(SAOK);
    ereg->geticode()->bubble(INOP);
    ereg->getifun()->bubble();
    ereg->getvalC()->bubble();
    ereg->getvalA()->bubble();
    ereg->getvalB()->bubble();
    ereg->getdstE()->bubble(RNONE);
    ereg->getdstM()->bubble(RNONE);
    ereg->getsrcA()->bubble(RNONE);
    ereg->getsrcB()->bubble(RNONE);
}

