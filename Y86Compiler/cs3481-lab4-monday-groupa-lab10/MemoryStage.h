//class to perform the combinational logic of
//the Memory stage
class MemoryStage: public Stage
{
    private:
        uint64_t valM = 0;
        uint64_t stat = 1;

        void setWInput(W * wreg, uint64_t stat, uint64_t icode,
                uint64_t valE, uint64_t valM, uint64_t dstE, uint64_t dstM);
        uint64_t setAddr(uint64_t icode, M * mreg);
        bool memoryRead(uint64_t icode);
        bool memoryWrite(uint64_t icode);
        uint64_t m_stat(bool mem_error, M * mreg);
    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);
        uint64_t getM_valM();
        uint64_t getM_Stat();
};
