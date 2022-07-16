//class to perform the combinational logic of
//the Fetch stage
class ExecuteStage: public Stage
{
    private:
        uint64_t valE = 0;
        uint64_t dstE = 0;
        bool m_bubble = false;
        uint64_t Cnd = 0;

        void setMInput(M * mreg, uint64_t stat, uint64_t icode, uint64_t Cnd, 
                uint64_t valE, uint64_t valA, uint64_t dstE, uint64_t dstM); 
        int64_t setaluA(uint64_t icode, E * ereg);
        int64_t setaluB(uint64_t icode, E * ereg);
        uint64_t setaluFun(uint64_t icode, E * ereg);
        bool setConditionCodes(uint64_t icode, W * wreg, Stage ** stages);
        uint64_t setedstE(uint64_t icode, E * ereg, uint64_t e_Cnd);
        uint64_t logicCircuitALU(uint64_t aluFun, int64_t aluA, int64_t aluB, E * ereg);
        void calculateConditionCodes(uint64_t aluFun, int64_t aluA, int64_t aluB, int64_t result);
        uint64_t cond(uint64_t icode, uint64_t ifun);
        bool calculateControlSignals(W * wreg, Stage ** stages);
        void normalM(M * mreg);
        void bubbleM(M * mreg);

    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);
        uint64_t getE_dstE();
        uint64_t getE_valE();
        uint64_t getE_Cnd();
};
