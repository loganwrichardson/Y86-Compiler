//the Fetch stage
class FetchStage: public Stage
{
    private:
        bool F_stall = false;
        bool D_stall = false;
        bool D_bubble = false;
        
        void setDInput(D * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
                uint64_t rA, uint64_t rB,
                uint64_t valC, uint64_t valP);
        uint64_t selectPC(F * freg, M * mreg, W * wreg);
        bool needRegIds(uint64_t icode);
        bool needValC(uint64_t icode);
        uint64_t predictPC(uint64_t icode, uint64_t valC, uint64_t valP);
        uint64_t PCincrement(uint64_t pc, bool regIds, bool needValC);
        uint8_t getRegIds(bool need_regId, uint64_t pc);
        uint64_t buildValC(bool need_valC, uint64_t pc, bool regIds);
        bool hasValidInstruction(uint64_t icode);
        uint64_t f_stat(bool mem_error, bool instrValid, uint64_t icode);
        uint64_t setIcode(bool mem_error, uint8_t newByte);
        uint64_t setIfun(bool mem_error, uint8_t newByte);
        bool setF_stall(D * dreg, E * ereg, M * mreg, Stage ** stages);
        bool setD_stall(E * ereg, Stage ** stages);
        void calculateControlSignals(D * dreg, E * ereg, M * mreg, Stage ** stages);
        bool setD_bubble(D * dreg, E * ereg, M * mreg, Stage ** stages);
        void bubbleD(D * dreg);
        void normalD(D * dreg);

    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);
};
