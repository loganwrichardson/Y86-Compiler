class DecodeStage: public Stage {

    private:
        uint64_t d_srcA = 0;
        uint64_t d_srcB = 0;
        bool E_bubble = false;

        void setEInput(E * ereg, uint64_t stat, uint64_t icode, uint64_t ifun, uint64_t valC,
                uint64_t valA, uint64_t valB, uint64_t dstE, 
                uint64_t dstM, uint64_t srcA, uint64_t srcB);
        uint64_t decodeA(uint64_t icode, uint64_t rA);
        uint64_t decodeB(uint64_t icode, uint64_t rB);
        uint64_t decodeDSTE(uint64_t icode, uint64_t rB);
        uint64_t decodeDSTM(uint64_t icode, uint64_t rA);
        uint64_t forwardValA(D * dreg, M * mreg, W * wreg, uint64_t rvalA, uint64_t srcA, Stage ** stages);
        uint64_t forwardValB(M * mreg, W * wreg, uint64_t rvalB, uint64_t srcB, Stage ** stages);
        bool calculateControlSignals(E * ereg, Stage ** stages);
        void normalE(E * ereg);
        void bubbleE(E * ereg);

    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);
        uint64_t getd_srcA();
        uint64_t getd_srcB();
};
