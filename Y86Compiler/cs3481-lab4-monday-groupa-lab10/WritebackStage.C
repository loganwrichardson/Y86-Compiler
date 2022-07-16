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
#include "WritebackStage.h"
#include "Status.h"
#include "Debug.h"

//class to perform the combinational logic of the write back stage
bool WritebackStage::doClockLow(PipeReg ** pregs, Stage ** stages) {

    // Pointer to the W register
    W * wreg = (W *) pregs[WREG];

    wreg->getdstM()->setInput(wreg->getvalM()->getOutput());

    // grabs the icode value out of W register
    // If the icode is equal to HALT return true, else false
    if(wreg->getstat()->getOutput() != SAOK) return true;

    return false;
}

// Writing to registers if we need to do so
void WritebackStage::doClockHigh(PipeReg ** pregs) {

    // Declare an instance of a regFile
    // Put valE in the register at destination E
    RegisterFile * regFile = RegisterFile::getInstance();
    bool reg = false;

    // Pointer to the W register
    W * wreg = (W *) pregs[WREG];

    // Writing to the register
    regFile->writeRegister(wreg->getvalE()->getOutput(), wreg->getdstE()->getOutput(), reg);
    regFile->writeRegister(wreg->getvalM()->getOutput(), wreg->getdstM()->getOutput(), reg);
}

