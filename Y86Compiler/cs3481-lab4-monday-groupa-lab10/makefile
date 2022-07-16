CC = g++
CFLAGS = -g -c -Wall -std=c++11 -O0
OBJ = yess.o PipeReg.o Simulate.o PipeRegField.o F.o D.o E.o M.o W.o ExecuteStage.o MemoryStage.o DecodeStage.o\
	FetchStage.o WritebackStage.o Loader.o ConditionCodes.o Memory.o RegisterFile.o Tools.o

.C.o:
	$(CC) $(CFLAGS) $< -o $@

yess: $(OBJ)

yess.o: Debug.h Memory.h Loader.h RegisterFile.h ConditionCodes.h PipeReg.h Stage.h Simulate.h
PipeReg.o: PipeReg.h
Simulate.o: PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h ExecuteStage.h MemoryStage.h DecodeStage.h\
	FetchStage.h WritebackStage.h Simulate.h Memory.h RegisterFile.h ConditionCodes.h
PipeRegField.o: PipeRegField.h
F.o: PipeRegField.h PipeReg.h F.h
D.o: Instructions.h RegisterFile.h PipeReg.h PipeRegField.h D.h Status.h
E.o: RegisterFile.h Instructions.h PipeRegField.h PipeReg.h E.h Status.h
M.o: RegisterFile.h Instructions.h PipeRegField.h PipeReg.h M.h Status.h
W.o: RegisterFile.h Instructions.h PipeRegField.h PipeReg.h W.h Status.h
ExecuteStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h ExecuteStage.h Status.h\
	Debug.h Instructions.h ConditionCodes.h MemoryStage.h
MemoryStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h MemoryStage.h Status.h\
Debug.h Instructions.h Memory.h
DecodeStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h DecodeStage.h Status.h Debug.h ExecuteStage.h Instructions.h MemoryStage.h
FetchStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h FetchStage.h Status.h Debug.h Memory.h Tools.h Instructions.h DecodeStage.h ExecuteStage.h
WritebackStage.o: RegisterFile.h PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h WritebackStage.h Status.h\
	Debug.h
Simulate.o: PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h ExecuteStage.h MemoryStage.h DecodeStage.h\
	FetchStage.h WritebackStage.h Simulate.h Memory.h RegisterFile.h ConditionCodes.h
Loader.o: Loader.h Memory.h
ConditionCodes.o: ConditionCodes.h Tools.h
Memory.o: Memory.h Tools.h
RegisterFile.o: RegisterFile.h Tools.h
Tools.o: Tools.h

clean:
	rm $(OBJ) yess

run:
	make clean
	make yess
	./run.sh

