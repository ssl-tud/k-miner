BIN = main
SRC = $(wildcard ./*.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
DEP = $(patsubst %.c, %.d, $(SRC))
BIT = $(patsubst %.c, %.bc, $(SRC))
ASM = $(patsubst %.c, %.ll, $(SRC)) 
FINAL_BIT = out.bc 
OPT = out.opt
DOT = $(wildcard ./*.dot)
PNG = $(patsubst %.dot, %.png, $(DOT))

CC = clang 
LN = llvm-link
OP = opt

all: $(BIN) $(FINAL_BIT) $(OPT) $(ASM)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -g -m32 -o $(BIN)

%.o: %.c
	$(CC) -g -MMD -MP -m32 -c $< -o $@ 

-include $(DEP)

$(FINAL_BIT): $(BIT)
	$(LN) $(BIT) -o $(FINAL_BIT)

$(OPT): $(FINAL_BIT)
	$(OP) -mem2reg $< -o $@

%.bc: %.c
	$(CC) -emit-llvm -c $< 
		
%.ll: %.c
	$(CC) -emit-llvm -S $<

# -----------------------------------------------
# Dump graphs
GRAPH_TYPE = fspta

png: $(DOT) $(PNG)

dot: $(FINAL_BIT)
	wpa -$(GRAPH_TYPE) -dump-callgraph -dump-mssa -dump-pag -dump-svfg $(FINAL_BIT)

dot_opt: $(OPT)
	wpa -$(GRAPH_TYPE) -dump-callgraph -dump-mssa -dump-pag -dump-svfg $(OPT)

%.png: %.dot
	dot -Tpng $< -o $@	

# -----------------------------------------------

clean:
	rm -rf $(DEP) $(OBJ) $(BIN) $(BIT) $(FINAL_BIT) $(ASM) $(DOT) $(PNG) $(OPT) ./*.wpa ./*saber ./*.data ./*ssca

.PHONY: clean
