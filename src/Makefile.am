SUBDIRS = APs

bin_PROGRAMS = apl

datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

apl_SOURCES = \
					buildtag.hh		\
					../config.h		\
					APL_types.hh		\
Archive.cc				Archive.hh		\
ArrayIterator.cc			ArrayIterator.hh	\
Assert.cc				Assert.hh		\
Avec.cc		    Avec.def		Avec.hh			\
Backtrace.cc				Backtrace.hh		\
Bif_OPER1_COMMUTE.cc			Bif_OPER1_COMMUTE.hh	\
Bif_OPER1_EACH.cc			Bif_OPER1_EACH.hh	\
Bif_OPER2_PRODUCT.cc			Bif_OPER2_PRODUCT.hh	\
Bif_OPER1_RANK.cc			Bif_OPER1_RANK.hh	\
Bif_OPER1_REDUCE.cc			Bif_OPER1_REDUCE.hh	\
Bif_OPER1_SCAN.cc			Bif_OPER1_SCAN.hh	\
CDR.cc					CDR.hh			\
Cell.cc					Cell.hh			\
CharCell.cc				CharCell.hh		\
CollatingCache.cc			CollatingCache.hh	\
Command.cc	    Command.def		Command.hh		\
Common.cc				Common.hh		\
ComplexCell.cc				ComplexCell.hh		\
DiffOut.cc				DiffOut.hh		\
DynamicObject.cc			DynamicObject.hh	\
					EOC_handler_args.hh	\
Error.cc	    Error.def		Error.hh		\
					ErrorCode.hh		\
Executable.cc				Executable.hh		\
FloatCell.cc				FloatCell.hh		\
Function.cc				Function.hh		\
DerivedFunction.cc			DerivedFunction.hh	\
Id.cc		    Id.def		Id.hh			\
IndexExpr.cc				IndexExpr.hh		\
IndexIterator.cc			IndexIterator.hh	\
Input.cc				Input.hh		\
IntCell.cc				IntCell.hh		\
LApack.cc							\
Logging.cc	    Logging.def		Logging.hh		\
LvalCell.cc				LvalCell.hh		\
main.cc					main.hh			\
Nabla.cc				Nabla.hh		\
NamedObject.cc				NamedObject.hh		\
NumericCell.cc				NumericCell.hh		\
Output.cc				Output.hh		\
Parser.cc				Parser.hh		\
Prefix.cc	    Prefix.def		Prefix.hh		\
PointerCell.cc				PointerCell.hh		\
PrimitiveFunction.cc			PrimitiveFunction.hh	\
PrimitiveOperator.cc			PrimitiveOperator.hh	\
PrintBuffer.cc				PrintBuffer.hh		\
PrintContext.cc				PrintContext.hh		\
					PrintOperator.hh	\
QuadFunction.cc				QuadFunction.hh		\
ProcessorID.cc				ProcessorID.hh		\
Quad_RL.cc				Quad_RL.hh		\
Quad_SVx.cc				Quad_SVx.hh		\
Quad_TF.cc				Quad_TF.hh		\
RealCell.cc				RealCell.hh		\
Shape.cc				Shape.hh		\
					Simple_string.hh	\
SkalarFunction.cc			SkalarFunction.hh	\
					Source.hh		\
StateIndicator.cc			StateIndicator.hh	\
		    StaticValues.def				\
Svar_DB.cc				Svar_DB.hh		\
		    Svar_signals.def	Svar_signals.hh		\
Symbol.cc				Symbol.hh		\
SymbolTable.cc				SymbolTable.hh		\
					SystemLimits.hh		\
SystemVariable.cc   SystemVariable.def	SystemVariable.hh	\
TestFiles.cc				TestFiles.hh		\
Token.cc				Token.hh		\
					TokenEnums.hh		\
Tokenizer.cc				Tokenizer.hh		\
UCS_string.cc				UCS_string.hh		\
UdpSocket.cc				UdpSocket.hh		\
					Unicode.hh		\
UserFunction.cc				UserFunction.hh		\
UTF8_string.cc				UTF8_string.hh		\
Value.cc				Value.hh		\
Workspace.cc				Workspace.hh

.precious:	Svar_signals.hh
Svar_signals.hh:	Svar_signals.def udp_signal.m4
	m4 -D protocol=Svar_signals udp_signal.m4 > $@

.precious:  buildtag.hh
buildtag.hh:	buildtag
	source buildtag

apl_CXXFLAGS = -rdynamic $(GPROF_WANTED)

apl_LDFLAGS  = $(GPROF_WANTED) $(LIBS)

apl.lines: apl
	objdump -j .text -l -d $< > $@

test: all
	./apl --id 1010 -T testcases/*tc

test1: all
	./apl --id 1010 --TM 3 -T testcases/*\.tc

perf: apl
	./apl --id 1010 --TM 3 -T testcases/Performance.pt


AM_MAKEFLAGS = -j 2


