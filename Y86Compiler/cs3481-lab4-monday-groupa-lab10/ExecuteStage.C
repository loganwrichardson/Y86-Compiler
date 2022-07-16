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
#include "ExecuteStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "ConditionCodes.h"
#include "MemoryStage.h"
#include <math.h>

//class to perform the combinational logic of the Execute
bool ExecuteStage::doClockLow(PipeReg ** pregs, Stage ** stages) {

    // Pointers to E, M and W registers
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];

    // Getting the codes out of the E register to put in the M register
    uint64_t valA = ereg->getvalA()->getOutput();
    uint64_t stat = ereg->getstat()->getOutput();
    uint64_t icode = ereg->geticode()->getOutput();
    uint64_t dstM = ereg->getdstM()->getOutput();
    bool set_cc = setConditionCodes(icode, wreg, stages);
    int64_t aluA = setaluA(icode, ereg);
    int64_t aluB = setaluB(icode, ereg);
    uint64_t aluFun = setaluFun(icode, ereg);

    //Cnd needs to be set to either 0 or 1, note for lab9
    Cnd = cond(icode, ereg->getifun()->getOutput());

    // If instructions sets condition codes call ALU method, else forward
    if (set_cc) valE = logicCircuitALU(aluFun, aluA, aluB, ereg);
    else valE = aluA;

    // Initialize dstE
    dstE = setedstE(icode, ereg, Cnd);

    // Sets the bool depending on if a bubble is needed
    m_bubble = calculateControlSignals(wreg, stages);

    // Sets the M registers values
    setMInput(mreg, stat, icode, Cnd, valE, valA, dstE, dstM);

    return false;
}

// what happens when the clock pulses high
void ExecuteStage::doClockHigh(PipeReg ** pregs) {
    // Pointer to the M register
    M * mreg = (M *) pregs[MREG];

    // Sets the state of the registers based on m_bubble
    if (!m_bubble) normalM(mreg);
    else bubbleM(mreg);
}

void ExecuteStage::setMInput(M * mreg, uint64_t stat, uint64_t icode, uint64_t Cnd, 
        uint64_t valE, uint64_t valA, uint64_t dstE, uint64_t dstM) {

    // Sets the values of the M register
    mreg->getstat()->setInput(stat); 
    mreg->geticode()->setInput(icode); 
    mreg->getCnd()->setInput(Cnd); 
    mreg->getvalE()->setInput(valE); 
    mreg->getvalA()->setInput(valA); 
    mreg->getdstE()->setInput(dstE); 
    mreg->getdstM()->setInput(dstM); 
}

// Multiplexes A for use in ALU based off which instruction we are executing
int64_t ExecuteStage::setaluA(uint64_t icode, E * ereg) {

    if (icode == IRRMOVQ || icode == IOPQ) return ereg->getvalA()->getOutput();
    else if (icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ) return ereg->getvalC()->getOutput() + ereg->getvalB()->getOutput();
    else if (icode == ICALL || icode == IPUSHQ) return -8 + ereg->getvalB()->getOutput();
    else if (icode == IRET || icode == IPOPQ) return 8 + ereg->getvalB()->getOutput();
    else return 0;
}

// Multiplexes B for use in ALU based off which instruction we are executing
int64_t ExecuteStage::setaluB(uint64_t icode, E * ereg) {

    if (icode == IRMMOVQ || icode == IMRMOVQ || icode == IOPQ ||
            icode == ICALL || icode == IPUSHQ || icode == IRET ||
            icode == IPOPQ) return ereg->getvalB()->getOutput();
    else if (icode == IRRMOVQ || icode == IIRMOVQ) return 0;
    else return 0;
}

// Multiplexes the correct OPQ instruction to use
uint64_t ExecuteStage::setaluFun(uint64_t icode, E * ereg) {

    if (icode == IOPQ) return ereg->getifun()->getOutput();
    else return ADDQ;
}

// Multiplexes conditions codes for use in CC logic unit
bool ExecuteStage::setConditionCodes(uint64_t icode, W * wreg, Stage ** stages) {
    MemoryStage * MS = (MemoryStage *) stages[MSTAGE];

    if(icode == IOPQ && MS->getM_Stat() != SADR && MS->getM_Stat() != SINS &&
            MS->getM_Stat() != SHLT && wreg->getstat()->getOutput() != SADR &&
            wreg->getstat()->getOutput() != SINS && wreg->getstat()->getOutput() != SHLT) return true;
    return false;
}

// Multiplxes dstE to pass the correct destination on to the memory stage
uint64_t ExecuteStage::setedstE(uint64_t icode, E * ereg, uint64_t e_Cnd) {

    if (icode == IRRMOVQ && !e_Cnd) return RNONE;
    else return ereg->getdstE()->getOutput();
}

// Calculates valE from the ALUA, ALUB & Fun multiplexers. Outputs valE & condition Codes
uint64_t ExecuteStage::logicCircuitALU(uint64_t aluFun, int64_t aluA, int64_t aluB, E * ereg) {

    int64_t result = 0;
    if (aluFun == ADDQ) {
        result = aluA + aluB;
        ExecuteStage::calculateConditionCodes(aluFun, aluA, aluB, result);
        return result;
    } else if (aluFun == SUBQ) {
        result = aluB - aluA;
        ExecuteStage::calculateConditionCodes(aluFun, aluA, aluB, result);
        return result;
    } else if (aluFun == ANDQ) {
        result = aluA & aluB;
        ExecuteStage::calculateConditionCodes(aluFun, aluA, aluB, result);
        return result;
    } else if (aluFun == XORQ) {
        result = aluA ^ aluB;
        ExecuteStage::calculateConditionCodes(aluFun, aluA, aluB, result);
        return result;
    } else
        return 0;
}

// Calculates the condition codes and set their flags for the pipeline
void ExecuteStage::calculateConditionCodes(uint64_t aluFun, int64_t aluA, int64_t aluB, int64_t result) {

    bool cc_error = false;

    // Get and isntance of Condition Codes
    ConditionCodes * cc = ConditionCodes::getInstance();

    // Rough Code
    switch(aluFun) {

        case ADDQ:
            // ZF = 0, SF = 0
            // If we're adding two positives together & get a negative
            if (result < 0) {                    
                // Set Zero Flag
                cc->setConditionCode(false, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(true, SF, cc_error);
            }
            // ZF = 0, SF = 0
            // If we're adding two negatives together & get a positive
            else if (result > 0) {
                // Set Zero Flag
                cc->setConditionCode(false, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(false, SF, cc_error);
            }
            // ZF = 1, SF = 0
            // If you add two numbers together and get a zero
            else { // Result == 0
                // Set Zero Flag
                cc->setConditionCode(true, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(false, SF, cc_error);
            }

            // Conditional to handle overflows
            if ((signbit(aluA) == signbit(aluB)) && signbit(aluA+aluB) != signbit(aluA)) {
                // If there is overflow
                cc->setConditionCode(true, OF, cc_error);
            }else
                // If there is no overflow
                cc->setConditionCode(false, OF, cc_error);

            break;


        case SUBQ:
            if (result < 0) {
                // Set Zero Flag
                cc->setConditionCode(false, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(true, SF, cc_error);
            }
            // ZF = 0, SF = 0
            else if (result > 0) {
                // Set Zero Flag
                cc->setConditionCode(false, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(false, SF, cc_error);
            }
            // ZF = 1, SF = 0
            else  { // Result == 0
                // Set Zero Flag
                cc->setConditionCode(true, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(false, SF, cc_error);
            }

            // If else conditional to handle overflows
            if ((signbit(aluA) != signbit(aluB)) && signbit(aluA-aluB) != signbit(aluA)) {
                // If there is overflow
                cc->setConditionCode(true, OF, cc_error);
            }else
                // If there is no overflow
                cc->setConditionCode(false, OF, cc_error);
            break;

        case ANDQ: 
            // ZF = 0, SF = 1, OF = 0
            if (result < 0) {
                // Set Zero Flag
                cc->setConditionCode(false, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(true, SF, cc_error);
                // Set Overflow Flag
                cc->setConditionCode(false, OF, cc_error);
            }
            // ZF = 0, SF = 0, OF = 0
            else if (result > 0) {
                // Set Zero Flag
                cc->setConditionCode(false, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(false, SF, cc_error);
                // Set Overflow Flag
                cc->setConditionCode(false, OF, cc_error);
            }
            // ZF = 1, SF = 0, OF = 0
            else  { // Result == 0
                // Set Zero Flag
                cc->setConditionCode(true, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(false, SF, cc_error);
                // Set Overflow Flag
                cc->setConditionCode(false, OF, cc_error);
            }
            break;

        case XORQ:
            // ZF = 0, SF = 1, OF = 0
            if (result < 0) {
                // Set Zero Flag
                cc->setConditionCode(false, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(true, SF, cc_error);
                // Set Overflow Flag
                cc->setConditionCode(false, OF, cc_error);
            }
            // ZF = 0, SF = 0, OF = 0
            else if (result > 0) {
                // Set Zero Flag
                cc->setConditionCode(false, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(false, SF, cc_error);
                // Set Overflow Flag
                cc->setConditionCode(false, OF, cc_error);
            }
            // ZF = 1, SF = 0, OF = 0
            else  { // Result == 0
                // Set Zero Flag
                cc->setConditionCode(true, ZF, cc_error);
                // Set Sign Flag
                cc->setConditionCode(false, SF, cc_error);
                // Set Overflow Flag
                cc->setConditionCode(false, OF, cc_error);
            }
            break;
    }
}

// Accessor for dstE
uint64_t ExecuteStage::getE_dstE() {

    return dstE;
}

// Accessor for valE
uint64_t ExecuteStage::getE_valE() {

    return valE;
}

// Cond method. Takes icode and ifun and outputs the value of e_Cnd
uint64_t ExecuteStage::cond(uint64_t icode, uint64_t ifun) {    

    uint64_t value = 0;

    // Get an instance of Condition Codes
    ConditionCodes * cc = ConditionCodes::getInstance();

    // Declare a bool for use in getConditonCode
    bool error = false;

    if (icode != IJXX && icode != ICMOVXX) return 0;

    bool sf = cc->getConditionCode(SF, error);
    bool of = cc->getConditionCode(OF, error);
    bool zf = cc->getConditionCode(ZF, error);

    switch (ifun) {
        // Unconditional Move (always jump)
        case UNCOND:
            value = 1;
            break;

        case LESSEQ:
            // (SF^OF) | ZF
            if ((sf^of) | zf) {
                value = 1;
            } else value = 0;
            break;

        case LESS:
            // (SF^OF)
            if (sf^of) {
                value = 1; 
            } else value = 0;
            break;
        case EQUAL:
            // ZF
            if (zf) {
                value = 1;
            } else value = 0;
            break;
        case NOTEQUAL:
            // !ZF
            if (!zf) {
                value = 1;
            } else value = 0;
            break;

        case GREATEREQ:
            //!(SF^OF)
            if (!(sf^of)) {
                value = 1;
            } else value = 0;
            break;
        case GREATER:
            //!SF^OF) & !ZF
            if (!(sf^of)&(!zf)) {
                value = 1;
            } else value = 0;
            break;
    }
    return value;
}

// Sets the m_bubble bool based on if a bubble is needed or not
bool ExecuteStage::calculateControlSignals(W * wreg, Stage ** stages) {
    MemoryStage * MS = (MemoryStage *) stages[MSTAGE];

    if (MS->getM_Stat() == SADR || MS->getM_Stat() == SINS || MS->getM_Stat() == SHLT ||
            wreg->getstat()->getOutput() == SADR || wreg->getstat()->getOutput() == SINS ||
            wreg->getstat()->getOutput() == SHLT) return true;
    return false;
}

// Simulates passing the correct values to the MemoryStage
void ExecuteStage::normalM(M * mreg) { 
    mreg->getstat()->normal();
    mreg->geticode()->normal();
    mreg->getCnd()->normal();
    mreg->getvalE()->normal();
    mreg->getvalA()->normal();
    mreg->getdstE()->normal();
    mreg->getdstM()->normal();
}

// Simulates passing a bubble into the MemoryStage
void ExecuteStage::bubbleM(M * mreg) {
    mreg->getstat()->bubble(SAOK);
    mreg->geticode()->bubble(INOP);
    mreg->getCnd()->bubble();
    mreg->getvalE()->bubble();
    mreg->getvalA()->bubble();
    mreg->getdstE()->bubble(RNONE);
    mreg->getdstM()->bubble(RNONE);
}

// Getter for E_Cnd
uint64_t ExecuteStage::getE_Cnd() {

    return Cnd;
}

