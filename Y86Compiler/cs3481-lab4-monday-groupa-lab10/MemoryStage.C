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
#include "MemoryStage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"

//class to perform the combinational logic of
//the Memory stage
bool MemoryStage::doClockLow(PipeReg ** pregs, Stage ** stages) {

    // Pointers to the W & M registers
    W * wreg = (W *) pregs[WREG];
    M * mreg = (M *) pregs[MREG];

    // Set valE & valM to 0
    valM = 0;
    stat = SAOK;

    // Pushes icode, dstE & dstM along from reg E to reg M
    uint64_t icode = mreg->geticode()->getOutput();
    uint64_t dstE = mreg->getdstE()->getOutput();
    uint64_t dstM = mreg->getdstM()->getOutput();
    uint64_t valE = mreg->getvalE()->getOutput();

    // Sets the memory address to the proper value
    uint64_t mem_addr = setAddr(icode, mreg);

    // Gets an instance of memory
    Memory * mem  = Memory::getInstance();

    // Declares a bool for use in getLong and putLong
    bool mem_error = false;

    // Gets a long from the instance of memory if reading
    if (memoryRead(icode)) valM = mem->getLong(mem_addr, mem_error);

    // Puts a long into memory if writing
    if (memoryWrite(icode)) mem->putLong(mreg->getvalA()->getOutput(), mem_addr, mem_error);

    stat = m_stat(mem_error, mreg);

    // Set the values of the w register
    setWInput(wreg, stat, icode, valE, valM, dstE, dstM);

    return false;
}

// Normalizes the values in the W register
void MemoryStage::doClockHigh(PipeReg ** pregs) {
    W * wreg = (W *) pregs[WREG];

    // Sets the state of the register to the input
    wreg -> getstat() -> normal();
    wreg -> geticode() -> normal();
    wreg -> getvalE() -> normal();
    wreg -> getvalM() -> normal();
    wreg -> getdstE() -> normal();
    wreg -> getdstM() -> normal();
}

// Sets the input to the W register
void MemoryStage::setWInput(W * wreg, uint64_t stat, uint64_t icode,
        uint64_t valE, uint64_t valM, uint64_t dstE, uint64_t dstM) {

    // Sets the values of w register
    wreg -> getstat() -> setInput(stat);
    wreg -> geticode() -> setInput(icode);
    wreg -> getvalE() -> setInput(valE);
    wreg -> getvalM() -> setInput(valM);
    wreg -> getdstE() -> setInput(dstE);
    wreg -> getdstM() -> setInput(dstM);
}

// Takes icode and sets the destination address for the specified instruction
uint64_t MemoryStage::setAddr(uint64_t icode, M * mreg) {

    if (icode == IRMMOVQ || icode == IPUSHQ || icode == ICALL || icode == IMRMOVQ) return mreg->getvalE()->getOutput();
    else if (icode == IPOPQ || icode == IRET ) return mreg->getvalA()->getOutput();
    else return 0;
}

// Takes an icode and determines if we are reading from memory
bool MemoryStage::memoryRead(uint64_t icode) {

    if (icode == IMRMOVQ || icode == IPOPQ || icode == IRET) return true;
    else return false;
}

// Takes an icode and determines if we are writing to memory
bool MemoryStage::memoryWrite(uint64_t icode) {

    if (icode == IRMMOVQ || icode == IPUSHQ || icode == ICALL) return true;
    else return false;
}

// Getter for valM
uint64_t MemoryStage::getM_valM() {
    return valM;
}

// Getter for stat
uint64_t MemoryStage::getM_Stat() {
    return stat;
}

// Sets m_stat correctly if there is an error in memory
uint64_t MemoryStage::m_stat(bool mem_error, M * mreg) {
    if (mem_error) return SADR;
    else return mreg->getstat()->getOutput(); 
}
