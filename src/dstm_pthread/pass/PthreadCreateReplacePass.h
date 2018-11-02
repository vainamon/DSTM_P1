/*
 * PthreadCreateReplacePass.h
 *
 *  Created on: 24.06.2011
 *      Author: igor
 */

#ifndef PTHREADCREATEREPLACEPASS_H_
#define PTHREADCREATEREPLACEPASS_H_

namespace llvm {
	class FunctionPass;

	FunctionPass* createPthreadCreateReplacePass();

}

#endif /* PTHREADCREATEREPLACEPASS_H_ */
