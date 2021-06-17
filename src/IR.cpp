#include "IR.h"

#include <unordered_map>

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include "KailedoscopeJIT.h"

namespace IR {

	static llvm::ExitOnError ExitOnErr;

	struct Context {
		std::unique_ptr<llvm::LLVMContext> LLVMContext;
		std::unique_ptr<llvm::Module> Module;
		std::unique_ptr<llvm::IRBuilder<>> Builder;
		std::unordered_map<std::string, llvm::AllocaInst *> ValueMap; // Maps variables declared in current scope

		// Defines optimization passes for IR
		std::unique_ptr<llvm::legacy::FunctionPassManager> OptimizationPasses;

		// Vanilla JIT compiler
		std::unique_ptr<llvm::orc::KaleidoscopeJIT> JIT;

		void Init() {
			JIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create());
			ResetModule();
		}

		void ResetModule() {
			LLVMContext = std::make_unique<llvm::LLVMContext>();
			Module = std::make_unique<llvm::Module>("KaleidoscopeDefaultModule", *LLVMContext);
			Module->setDataLayout(JIT->getDataLayout()); // this doesn't bind the module to the JIT

			Builder = std::make_unique<llvm::IRBuilder<>>(*LLVMContext);

			OptimizationPasses = std::make_unique<llvm::legacy::FunctionPassManager>(Module.get());
			OptimizationPasses->add(llvm::createPromoteMemoryToRegisterPass());	 // mem2reg pass, promotes allocas to registers
			OptimizationPasses->add(llvm::createInstructionCombiningPass());
			OptimizationPasses->add(llvm::createReassociatePass());
			OptimizationPasses->add(llvm::createGVNPass());
			OptimizationPasses->add(llvm::createCFGSimplificationPass());
			OptimizationPasses->doInitialization();
		}

		void Dump() {
			Module->print(llvm::errs(), nullptr);
		}
	};

	static Context s_ir;

	Context &GetContext() { return s_ir; }

	void GenerateCode(Parser::TranslationUnitASTPtr unit) {
		s_ir.Init();
		unit->GenerateCode();
		s_ir.Dump();
	}

	// BEWARE: JIT compilation invalidates the module, so you need to reset it everytime you compile something
	void JITCompile() {
		// IR builder uses target architecture's data layout to allocate memory with proper
		// allignment, guaranteeing allocations are optimized for the platform.
		llvm::orc::ResourceTrackerSP resourceTracker = s_ir.JIT->getMainJITDylib().createResourceTracker();

		// Transfers module ownership to JIT compiler, so it's discarded after compilation
		llvm::orc::ThreadSafeModule safeModule{std::move(s_ir.Module), std::move(s_ir.LLVMContext)};
		ExitOnErr(s_ir.JIT->addModule(std::move(safeModule), resourceTracker));

		if (auto exprSymbol = ExitOnErr(s_ir.JIT->lookup(ANON_EXPR_NAME))) {
			double (*funcPointer)() = (double (*)())(intptr_t)exprSymbol.getAddress();
			fprintf(stderr, "Evaluated to %f\n", funcPointer());
		}

		ExitOnErr(resourceTracker->remove());
	}

	// Creates stack allocation for a variable. Allocas must ALWAYS
	// be declared in the function's entry point, so that mem2reg optimization
	// is able to optimize local variables into phi nodes.
	llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* function, const std::string& varName) {
		// Creates temporary builder on start of entry block
		llvm::BasicBlock *entryBlock = &function->getEntryBlock();
		llvm::IRBuilder<> tempBuilder { entryBlock, entryBlock->begin() };
		
		return tempBuilder.CreateAlloca(llvm::Type::getDoubleTy(*s_ir.LLVMContext), 0, varName.c_str());
	}
}

namespace Parser {

	llvm::Value *NumberExpr::GenerateCode() {
		return llvm::ConstantFP::get(*IR::GetContext().LLVMContext, llvm::APFloat{m_value});
	}

	llvm::Value *VariableExpr::GenerateCode() {
		auto &ctx = IR::GetContext();
		auto &varAlloca = ctx.ValueMap[m_name];
		if (varAlloca)
			return ctx.Builder->CreateLoad(varAlloca, m_name);

		printf(">> ERROR: Unknown variable name\n");
		return nullptr;
	}

	llvm::Value *BinaryExpr::GenerateCode() {
		llvm::Value *lhsValue = m_lhs->GenerateCode(), *rhsValue = m_rhs->GenerateCode();

		if (!lhsValue || !rhsValue) {
			return nullptr;
		}

		auto &ctx = IR::GetContext();
		switch (m_op) {
			case '+':
				return ctx.Builder->CreateFAdd(lhsValue, rhsValue, "addtmp"); // name parameter is optional but helps reading
			case '-':
				return ctx.Builder->CreateFSub(lhsValue, rhsValue, "subtmp");
			case '*':
				return ctx.Builder->CreateFMul(lhsValue, rhsValue, "multmp");
			case '/':
				return ctx.Builder->CreateFDiv(lhsValue, rhsValue, "divtmp");
			case '<':
				// Converts float operation to integer boolean
				lhsValue = ctx.Builder->CreateFCmpULT(lhsValue, rhsValue, "lttmp"); // ULT = unordered less than
				return ctx.Builder->CreateFPToUI(lhsValue, llvm::Type::getInt1Ty(*IR::GetContext().LLVMContext), "booltmp");
			case '>':
				lhsValue = ctx.Builder->CreateFCmpUGT(lhsValue, rhsValue, "gttmp"); // UGT = unordered greater than
				return ctx.Builder->CreateFPToUI(lhsValue, llvm::Type::getInt1Ty(*IR::GetContext().LLVMContext), "booltmp");
			default:
				printf(">> ERROR: Unknown binary operator\n");
				return nullptr;
		}
		return nullptr;
	}

	llvm::Value *CallExpr::GenerateCode() {
		// Checks if function is defined and arguments are valid
		auto &ctx = IR::GetContext();
		if (llvm::Function *calledFunction = ctx.Module->getFunction(m_calleeName)) {
			if (m_args.size() == calledFunction->arg_size()) {
				std::vector<llvm::Value *> arguments;
				for (const auto &arg : m_args) {
					if (llvm::Value *argValue = arg->GenerateCode()) {
						arguments.push_back(argValue);
					} else {
						printf(">> ERROR: Invalid function argument\n");
						return nullptr;
					}
				}

				return ctx.Builder->CreateCall(calledFunction, arguments, "calltmp");
			}

			printf(">> ERROR: Called function with wrong number of arguments\n");
			return nullptr;
		}

		printf(">> ERROR: %s definition not found\n", m_calleeName.c_str());
		return nullptr;
	}

	llvm::Value *ReturnStmt::GenerateCode() {
		// Sets function return type
		if (llvm::Value *returnValue = m_returnExpr->GenerateCode()) {
			IR::GetContext().Builder->CreateRet(returnValue);
			return returnValue;
		}

		printf(">> ERROR: Could not evaluate return statement.\n");
		return nullptr;
	}

	llvm::Value *IfStmt::GenerateCode() {
		auto &ctx = IR::GetContext();
		auto &builder = ctx.Builder;

		llvm::Function *function = builder->GetInsertBlock()->getParent();

		llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(*ctx.LLVMContext, "ifend");

		llvm::Value *value = GenerateCodeSequence(exitBlock);

		function->getBasicBlockList().push_back(exitBlock);
		builder->SetInsertPoint(exitBlock);

		return value;
	}

	llvm::Value *IfStmt::GenerateCodeSequence(llvm::BasicBlock *exit) {
		auto &ctx = IR::GetContext();
		auto &builder = ctx.Builder;

		if (llvm::Value *conditionValue = m_condition->GenerateCode()) {
			llvm::BasicBlock *parentBlock = builder->GetInsertBlock();
			llvm::Function *function = parentBlock->getParent();

			// Creates block for if statement
			llvm::BasicBlock *ifBlock = llvm::BasicBlock::Create(*ctx.LLVMContext, "ifbb", function);
			builder->SetInsertPoint(ifBlock);
			m_body->GenerateCode();

			// Creates block for else/else-if statement (optional)
			if (m_else) {
				llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(*ctx.LLVMContext, "elsebb", function);
				builder->SetInsertPoint(parentBlock);
				builder->CreateCondBr(conditionValue, ifBlock, elseBlock); // branches from parent to if/else
				builder->SetInsertPoint(elseBlock);

				if (IfStmt *elseIfStmt = dynamic_cast<IfStmt *>(m_else.get())) {
					elseIfStmt->GenerateCodeSequence(exit);
				} else {
					// Generates else code
					m_else->GenerateCode();
				}
			} else {
				builder->SetInsertPoint(parentBlock);
				builder->CreateCondBr(conditionValue, ifBlock, exit); // branches from parent to if or exit
			}

			return ifBlock;
		}
		return nullptr;
	}

	llvm::Value *ForStmt::GenerateCode() {
		auto &ctx = IR::GetContext();
		auto &builder = ctx.Builder;

		llvm::BasicBlock *entryBlock = builder->GetInsertBlock();
		llvm::Function *function = entryBlock->getParent();

		// Creates alloca for loop induction var. This eliminates the need for a Phi instruction.
		auto &loopVarAlloca = ctx.ValueMap[m_loopVarName];
		loopVarAlloca = IR::CreateEntryBlockAlloca(function, m_loopVarName);
		llvm::Value *startVal = m_value->GenerateCode();
		builder->CreateStore(startVal, loopVarAlloca);

		// Generates loop block
		llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(*ctx.LLVMContext, "loop", function);
		builder->CreateBr(loopBlock);			// Branches from entry to loop
		builder->SetInsertPoint(loopBlock);

		// Generates loop body
		m_body->GenerateCode();

		// Increments loop variable by step
		llvm::Value *step = m_step->GenerateCode();
		llvm::Value *currentLoopValue = builder->CreateLoad(loopVarAlloca, m_loopVarName);
		llvm::Value *newLoopValue = builder->CreateFAdd(currentLoopValue, step, m_loopVarName);
		builder->CreateStore(newLoopValue, loopVarAlloca);

		// Generates loop exit
		llvm::BasicBlock *loopEndBlock = llvm::BasicBlock::Create(*ctx.LLVMContext, "loopend", function);
		llvm::Value *endCondition = m_condition->GenerateCode();
		builder->CreateCondBr(endCondition, loopBlock, loopEndBlock);

		builder->SetInsertPoint(loopEndBlock);
		ctx.ValueMap.erase(m_loopVarName);		// Removes loop induction var

		return loopEndBlock;
	}

	llvm::Value *CompoundStmt::GenerateCode() {
		// CompoundStmt requires at least one statement
		if (m_statements.size() == 0)
			return nullptr;

		llvm::BasicBlock *parentBlock = IR::GetContext().Builder->GetInsertBlock();

		for (auto &stmt : m_statements)
			stmt->GenerateCode();

		return parentBlock;
	}

	llvm::Value* AssignStmt::GenerateCode() {
		return nullptr;
	}

	llvm::Function *PrototypeDecl::GenerateCode() {
		// Creates vector of parameter types. Currently, all parameters are of type 'double'
		std::vector<llvm::Type *> parameters{m_params.size(), llvm::Type::getDoubleTy(*IR::GetContext().LLVMContext)};

		// Creates function type
		llvm::FunctionType *functionType = llvm::FunctionType::get(
		    llvm::Type::getDoubleTy(*IR::GetContext().LLVMContext), parameters, false);

		// Creates function prototype and adds it to the module
		llvm::Function *function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, m_name, IR::GetContext().Module.get());

		// Sets names of all function parameters
		unsigned int idx = 0;
		for (auto &arg : function->args())
			arg.setName(m_params[idx++]);

		return function;
	}

	// Adds default return value when block has no control flow instruction
	void AddDefaultReturn(llvm::Function *function) {
		for (auto &block : function->getBasicBlockList()) {
			llvm::Value *lastDoubleInst = nullptr;
			bool noControlFlow = true;
			for (auto &stmt : block.getInstList()) {
				if (stmt.getType()->isDoubleTy()) {
					lastDoubleInst = &stmt;
				}
				if (llvm::isa<llvm::ReturnInst>(stmt) || llvm::isa<llvm::BranchInst>(stmt)) {
					noControlFlow = false;
					break;
				}
			}

			if (noControlFlow) {
				IR::GetContext().Builder->SetInsertPoint(&block);

				if (lastDoubleInst) {
					IR::GetContext().Builder->CreateRet(lastDoubleInst);
				} else {
					IR::GetContext().Builder->CreateRetVoid();
				}
			}
		}
	}

	llvm::Function *FunctionDecl::GenerateCode() {
		// Looks for function prototype
		llvm::Function *function = IR::GetContext().Module->getFunction(m_prototype->GetName());
		function = function ? function : m_prototype->GenerateCode();

		auto &ctx = IR::GetContext();
		auto &builder = ctx.Builder;
		if (function) {
			// Creates new block
			llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(*IR::GetContext().LLVMContext, "entry", function);
			builder->SetInsertPoint(entryBlock);

			// Inserts variables inside block to current scope
			ctx.ValueMap.clear();
			for (auto &arg : function->args()) {
				auto &varAlloca = ctx.ValueMap[arg.getName().str()];
				varAlloca = IR::CreateEntryBlockAlloca(function, arg.getName().str());
				builder->CreateStore(&arg, varAlloca);
			}

			if (llvm::Value *body = m_body->GenerateCode()) {
				// BEWARE: This changes insert point to block with no control flow
				AddDefaultReturn(function);

				// Verifies correctness of function
				llvm::verifyFunction(*function);

				// Optimizes function in place, before compiling the rest of the module
				ctx.OptimizationPasses->run(*function);

				return function;
			}

			function->eraseFromParent();
			printf(">> ERROR: Function has no body\n");
			return nullptr;
		}

		printf(">> ERROR: Invalid function prototype\n");
		return nullptr;
	}

	void TranslationUnitDecl::GenerateCode() {
		for (const auto &proto : m_prototypes)
			proto->GenerateCode();

		for (const auto &func : m_functions)
			func->GenerateCode();
	}
}