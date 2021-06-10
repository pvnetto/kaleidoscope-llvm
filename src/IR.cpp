#include "IR.h"

#include <unordered_map>

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>

#include <llvm/IR/LegacyPassManager.h>
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
		std::unordered_map<std::string, llvm::Value *> ValueMap; // Maps variables declared in current scope

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
}

namespace Parser {

	llvm::Value *NumberExprAST::GenerateCode() {
		return llvm::ConstantFP::get(*IR::GetContext().LLVMContext, llvm::APFloat{m_value});
	}

	llvm::Value *VariableExprAST::GenerateCode() {
		if (IR::GetContext().ValueMap.count(m_name) > 0)
			return IR::GetContext().ValueMap[m_name];

		printf(">> ERROR: Unknown variable name\n");
		return nullptr;
	}

	llvm::Value *BinaryExprAST::GenerateCode() {
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
				// Converts float operation to boolean
				lhsValue = ctx.Builder->CreateFCmpULT(lhsValue, rhsValue, "cmptmp");
				return ctx.Builder->CreateUIToFP(lhsValue, llvm::Type::getDoubleTy(*IR::GetContext().LLVMContext), "booltmp");
			default:
				printf(">> ERROR: Unknown binary operator\n");
				return nullptr;
		}
		return nullptr;
	}

	llvm::Value *CallExprAST::GenerateCode() {
		// Checks if function is defined and arguments are valid
		if (llvm::Function *calledFunction = IR::GetContext().Module->getFunction(m_calleeName)) {
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

				return IR::GetContext().Builder->CreateCall(calledFunction, arguments, "calltmp");
			}

			printf(">> ERROR: Called function with wrong number of arguments\n");
			return nullptr;
		}

		printf(">> ERROR: %s definition not found\n", m_calleeName.c_str());
		return nullptr;
	}

	llvm::Value* ReturnStmtAST::GenerateCode() {
		// Sets function return type
		if (llvm::Value *returnValue = m_returnExpr->GenerateCode()) {
			IR::GetContext().Builder->CreateRet(returnValue);
			return returnValue;
		}

		printf(">> ERROR: Could not evaluate return statement.\n");
		return nullptr;
	}

	llvm::Value *CompoundStmtAST::GenerateCode() {
		// CompoundStmt requires at least one statement
		if (m_statements.size() == 0)
			return nullptr;

		llvm::BasicBlock *parentBlock = IR::GetContext().Builder->GetInsertBlock();

		for (auto &stmt : m_statements) stmt->GenerateCode();

		// Handles nested blocks by setting insert point to block that was active before
		IR::GetContext().Builder->SetInsertPoint(parentBlock);
		return parentBlock;
	}

	llvm::Function *PrototypeDeclAST::GenerateCode() {
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

	llvm::Function *FunctionDeclAST::GenerateCode() {
		// Looks for function prototype
		llvm::Function *function = IR::GetContext().Module->getFunction(m_prototype->GetName());
		function = function ? function : m_prototype->GenerateCode();

		if (function) {
			// Creates new block
			llvm::BasicBlock *functionBlock = llvm::BasicBlock::Create(*IR::GetContext().LLVMContext, "entry", function);
			IR::GetContext().Builder->SetInsertPoint(functionBlock);

			// Inserts variables inside block to current scope
			IR::GetContext().ValueMap.clear();
			for (auto &arg : function->args())
				IR::GetContext().ValueMap[std::string(arg.getName())] = &arg;

			if (llvm::Value *body = m_body->GenerateCode()) {
				// Adds default return value when function has no return statement
				bool shouldAddReturn = true;
				llvm::Value* lastDoubleInst = nullptr;
				for (auto &stmt : functionBlock->getInstList()) {
					if (stmt.getType()->isDoubleTy()) {
						lastDoubleInst = &stmt;
					}
					if (llvm::isa<llvm::ReturnInst>(stmt)) {
						shouldAddReturn = false;
						break;
					}
				}

				if (shouldAddReturn) {
					IR::GetContext().Builder->CreateRet(lastDoubleInst);
				}

				// Verifies correctness of function
				llvm::verifyFunction(*function);

				// Optimizes function in place, before it's even added to the module
				IR::GetContext().OptimizationPasses->run(*function);

				return function;
			}

			function->eraseFromParent();
			printf(">> ERROR: Function has no body\n");
			return nullptr;
		}

		printf(">> ERROR: Invalid function prototype\n");
		return nullptr;
	}

	void TranslationUnitDeclAST::GenerateCode() {
		for (const auto &proto : m_prototypes)
			proto->GenerateCode();

		for (const auto &func : m_functions)
			func->GenerateCode();
	}
}