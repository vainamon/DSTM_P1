/*
 * PthreadCreateReplacePass.cpp
 *
 *  Created on: 22.06.2011
 *      Author: igor
 */

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CallSite.h"

#include "PthreadCreateReplacePass.h"

using namespace llvm;

namespace {
	struct PthreadCreateReplacePass : FunctionPass {
		static char ID;

		PthreadCreateReplacePass() : FunctionPass(ID){}

		virtual bool runOnFunction(Function &F)
		{
			for(Function::iterator BB = F.begin(), BBE = F.end();BB != BBE; ++BB){
				for(BasicBlock::iterator II = BB->begin(), IE = BB->end();II != IE;++II){
					CallSite CS = CallSite::get(II);
					Instruction *call = CS.getInstruction();
					if (call){
						Function *callingF = CS.getCalledFunction();
						if(callingF->getName()=="pthread_create"){
							errs()<<CS.getArgument(2)->getName()<<"\n";
						}else
							errs()<< "Hello: "<<callingF->getName() <<"\n";
					}
				}
			}
			return false;
		}
	};

	char PthreadCreateReplacePass::ID=0;

	static RegisterPass<PthreadCreateReplacePass> X("pcrpass","pthread_create calls replacement",false,false);
}

FunctionPass *llvm::createPthreadCreateReplacePass()
{
	return new PthreadCreateReplacePass();
}

