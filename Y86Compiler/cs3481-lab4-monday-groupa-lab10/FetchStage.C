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
#include "FetchStage.h"
#include "Status.h"
#include "Debug.h"
#include "Memory.h"
#include "Tools.h"
#include <iostream>
#include <array>
#include "Status.h"
#include "Instructions.h"
#include "DecodeStage.h"
#include "ExecuteStage.h"

/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    // Declaring pointers to specific registers.
    // These registers are labeled F, D, E, M, W in the hardware diagram.
    F * freg = (F *) pregs[FREG];
    D * dreg = (D *) pregs[DREG];
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];

    // Declaring all the different pieces of the instruction code
    uint64_t f_pc = 0, icode = 0, ifun = 0, valC = 0, valP = 0;
    uint64_t rA = 0xf, rB = 0xf, stat = SAOK;

    // Declaring a memory instance and setting a bool value for use with the get
    Memory * mem = Memory::getInstance();
    bool imem = false;

    // Selecting the new Program Counter & then getting the memory byte
    // The Memory byte gives us the icode and ifunction code
    f_pc = selectPC(freg, mreg, wreg);
    uint8_t newByte = mem->getByte(f_pc, imem);

    // Set Icode & Ifun based on presence of errors when getting icode from memory
    icode = setIcode(imem, newByte);
    ifun = setIfun(imem, newByte);    

    // Checks to see if the icode is a valid instruction.
    bool instrValid = hasValidInstruction(icode); 

    // Gives the status of the pipline given a set a parameters
    stat = f_stat(imem, instrValid, icode);

    // Determine if we need to use the registers
    // If we need them, set rA & rB with bit shifting
    bool regIds = needRegIds(icode);
    uint8_t registerByte = getRegIds(regIds, f_pc);
    //Shift byte to get rA and rB
    if(regIds) {
        rA = Tools::getBits(registerByte, 4, 7);
        rB = Tools::getBits(registerByte, 0, 3);
    }

    // Determine if we need ValC
    // Set ValC
    bool nValC = needValC(icode);
    valC = buildValC(nValC, f_pc, regIds);

    // Increment the program counter
    valP = PCincrement(f_pc, regIds, nValC);

    // The value passed to setInput below will need to be changed
    freg->getpredPC()->setInput(predictPC(icode, valC, valP));

    // Call to calculateControlSignals to set the stall signals
    calculateControlSignals(dreg, ereg, mreg, stages);

    // provide the input values for the D register
    setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);

    return false;
}

/* doClockHigh
 * applies the appropriate control signal to the F and D register intances.
 * If you need to bubble call bubbleD() else if you don't need to bubble and
 * you don't need to stall call normalD()
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
    // Declare the pointers the F & D register
    F * freg = (F *) pregs[FREG];
    D * dreg = (D *) pregs[DREG];

    // Normalizes the predPC if F_stall is not true
    if (F_stall == false)
        freg->getpredPC()->normal();

    // Normalizes the D register fields if D_stall is not true
    if (D_bubble) bubbleD(dreg);
    else if ((!D_bubble && !D_stall)) normalD(dreg);
}

/* setDInput
 * provides the input to potentially be stored in the D register
 * during doClockHigh
 *
 * @param: dreg - pointer to the D register instance
 * @param: stat - value to be stored in the stat pipeline register within D
 * @param: icode - value to be stored in the icode pipeline register within D
 * @param: ifun - value to be stored in the ifun pipeline register within D
 * @param: rA - value to be stored in the rA pipeline register within D
 * @param: rB - value to be stored in the rB pipeline register within D
 * @param: valC - value to be stored in the valC pipeline register within D
 * @param: valP - value to be stored in the valP pipeline register within D
 */
void FetchStage::setDInput(D * dreg, uint64_t stat, uint64_t icode, 
        uint64_t ifun, uint64_t rA, uint64_t rB,
        uint64_t valC, uint64_t valP)
{
    // Setting the values for the D register for use in Decode stage
    dreg->getstat()->setInput(stat);
    dreg->geticode()->setInput(icode);
    dreg->getifun()->setInput(ifun);
    dreg->getrA()->setInput(rA);
    dreg->getrB()->setInput(rB);
    dreg->getvalC()->setInput(valC);
    dreg->getvalP()->setInput(valP);
}

// Multiplexes to select the program counter
uint64_t FetchStage::selectPC(F * freg, M * mreg, W * wreg) {

    if (mreg->geticode()->getOutput() == IJXX && !mreg->getCnd()->getOutput()) return mreg->getvalA()->getOutput();
    else if (wreg->geticode()->getOutput() == IRET) return wreg->getvalM()->getOutput();
    else return freg->getpredPC()->getOutput();
}

// Determine if we need Register IDs based off of the instruction code
bool FetchStage::needRegIds(uint64_t icode) {

    if (icode == IRRMOVQ || icode == IOPQ || icode == IPUSHQ ||
            icode == IPOPQ || icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ) return true;
    return false;
}

// Determine if we need valC based off of the instruction code
bool FetchStage::needValC(uint64_t icode) {

    if (icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ || icode == IJXX || icode == ICALL) return true;
    return false;
}

// Predict the program counter value
uint64_t FetchStage::predictPC(uint64_t icode, uint64_t valC, uint64_t valP) {

    if (icode == IJXX || icode == ICALL) return valC;
    return valP;
}

// Increment the Program Counter
uint64_t FetchStage::PCincrement(uint64_t pc, bool regIds, bool needValC) {

    if (regIds && needValC) return pc + 10;
    else if (!regIds && needValC) return pc + 9;
    else if (regIds && !needValC) return pc + 2;
    else return pc + 1;
}

// If RegIDs are needed, get the byte you need from memory 
// This is then used to get the register values
uint8_t FetchStage::getRegIds(bool need_regId, uint64_t pc) {
    if (!need_regId) return 0;

    bool imem = false;

    // Get an instance of memory
    Memory * mem = Memory::getInstance();

    // Grab a register byte from the instruction in memory
    uint8_t newByte = mem->getByte((pc+1), imem);
    return newByte;
}

// If valC is needed, this is used to build valC 
uint64_t FetchStage::buildValC(bool need_valC, uint64_t pc, bool regIds) {
    if (!need_valC) return 0;

    // Declare a byteArray of size 8
    uint8_t byteArray[8];

    // Declare a bool imem for use with getByte()
    bool imem = false;

    // Get an instance of memory
    Memory * mem = Memory::getInstance();

    uint64_t newLong = 0;

    // Use a for loop to get the byte to use with buildLong
    if (regIds) {
        for (int i = 0; i < 8; i++) {
            byteArray[i] = mem->getByte((pc + i + 2), imem);
        }
        newLong = Tools::buildLong(byteArray);
    } else {
        for (int i = 0; i < 8; i++) {
            byteArray[i] = mem->getByte((pc + i + 1), imem);
        }
        newLong = Tools::buildLong(byteArray);
    }

    return newLong;
}

// Checks to see if the icode is valid instruction
bool FetchStage::hasValidInstruction(uint64_t icode) {  
    if (icode == INOP || icode == IHALT || icode == IRRMOVQ || icode == IIRMOVQ || icode == IRMMOVQ || icode == IMRMOVQ ||
            icode == IOPQ || icode == IJXX || icode == ICALL || icode == IRET || icode == IPUSHQ || icode == IPOPQ) return true;
    return false;
}

// Returns the status of the pipeline given a set of inputs
uint64_t FetchStage::f_stat(bool mem_error, bool instrValid, uint64_t icode) {
    if (mem_error) return SADR; 
    else if (!instrValid) return SINS;
    else if (icode == IHALT) return SHLT;
    else return SAOK;
}

// Sets the icode based on the byte grabbed from memory
uint64_t FetchStage::setIcode(bool mem_error, uint8_t newByte) {
    if (mem_error) return INOP;
    else return Tools::getBits(newByte, 4, 7);
}

// Sets the ifun based on the byte grabbed form memory
uint64_t FetchStage::setIfun(bool mem_error, uint8_t newByte) {
    if (mem_error) return FNONE;
    else return Tools::getBits(newByte, 0, 3);
}

// Determines the F_stall boolean based on if your executing memory to register
// or popping AND E_dstM is equivalent to d_srcA or d_srcB OR if the current instruction
// is a return instruction
bool FetchStage::setF_stall(D * dreg, E * ereg, M * mreg, Stage ** stages) {
    DecodeStage * DS = (DecodeStage *) stages[DSTAGE];

    if (((ereg->geticode()->getOutput() == IMRMOVQ || ereg->geticode()->getOutput() == IPOPQ) &&
                (ereg->getdstM()->getOutput() == DS->getd_srcA() || ereg->getdstM()->getOutput() == DS->getd_srcB())) ||
            (IRET == dreg->geticode()->getOutput() || IRET == ereg->geticode()->getOutput() ||
             IRET == mreg->geticode()->getOutput())) return true;
    return false;
}

// Determines the D_stall boolean based on the state of the E and D registers
bool FetchStage::setD_stall(E * ereg, Stage ** stages) {
    DecodeStage * DS = (DecodeStage *) stages[DSTAGE];

    if ((ereg->geticode()->getOutput() == IMRMOVQ || ereg->geticode()->getOutput() == IPOPQ) &&
            (ereg->getdstM()->getOutput() == DS->getd_srcA() ||
             ereg->getdstM()->getOutput() == DS->getd_srcB())) return true;
    return false;
}

// Calculates the value of D_bubble based on if there is a unconditional jmp instruction OR
// if there is a return instruction in the pipeline but only if there isn't a load/use hazard
bool FetchStage::setD_bubble(D * dreg, E * ereg, M * mreg, Stage ** stages) {
    DecodeStage * DS = (DecodeStage *) stages[DSTAGE];
    ExecuteStage * ES = (ExecuteStage *) stages[ESTAGE];

    if ((ereg->geticode()->getOutput() == IJXX && (!ES->getE_Cnd())) ||
            ((!((ereg->geticode()->getOutput() == IMRMOVQ || ereg->geticode()->getOutput() == IPOPQ) &&
                (ereg->getdstM()->getOutput() == DS->getd_srcA() ||
                 ereg->getdstM()->getOutput() == DS->getd_srcB()))) &&
             (IRET == dreg->geticode()->getOutput() || IRET == ereg->geticode()->getOutput() ||
              IRET == mreg->geticode()->getOutput()))) return true;
    return false;
}

// Calculates the control signals F and D stall by calling the methods
void FetchStage::calculateControlSignals(D * dreg, E * ereg, M * mreg, Stage ** stages) {
    F_stall = setF_stall(dreg, ereg, mreg, stages);
    D_stall = setD_stall(ereg, stages);
    D_bubble = setD_bubble(dreg, ereg, mreg, stages);
}

// Calls bubble on each of the fields of the D register
void FetchStage::bubbleD(D * dreg) {

    dreg->getstat()->bubble(SAOK);
    dreg->geticode()->bubble(INOP);
    dreg->getifun()->bubble();
    dreg->getrA()->bubble(RNONE);
    dreg->getrB()->bubble(RNONE);
    dreg->getvalC()->bubble();
    dreg->getvalP()->bubble();
}

// Calls normal on each of the fields of the D register
void FetchStage::normalD(D * dreg) {

    dreg->getstat()->normal();
    dreg->geticode()->normal();
    dreg->getifun()->normal();
    dreg->getrA()->normal();
    dreg->getrB()->normal();
    dreg->getvalC()->normal();
    dreg->getvalP()->normal();
}

