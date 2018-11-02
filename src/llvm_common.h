/*
 * llvm_common.h
 *
 *  Created on: 27.02.2012
 *      Author: igor
 */

#ifndef LLVM_COMMON_H_
#define LLVM_COMMON_H_

#include <llvm/LLVMContext.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/JIT.h>
//#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/PassManager.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Linker.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/CommandLine.h>

#endif /* LLVM_COMMON_H_ */
