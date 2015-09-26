#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace llvm;

// ***************************************************************************
//          プリプロセッサからのコールバック処理
// ***************************************************************************

class PPCallbacksTracker : public PPCallbacks {
private:
    Preprocessor &PP;
public:
    PPCallbacksTracker(Preprocessor &pp) : PP(pp) {}
    void InclusionDirective(SourceLocation HashLoc,
                          const Token &IncludeTok,
                          llvm::StringRef FileName, 
                          bool IsAngled,
                          CharSourceRange FilenameRange,
                          const FileEntry *File,
                          llvm::StringRef SearchPath,
                          llvm::StringRef RelativePath,
                          const Module *Imported) override {
        errs() << "InclusionDirective : ";
        if (File) {
            if (IsAngled)   errs() << "<" << File->getName() << ">\n";
            else            errs() << "\"" << File->getName() << "\"\n";
        } else {
            errs() << "not found file ";
            if (IsAngled)   errs() << "<" << FileName << ">\n";
            else            errs() << "\"" << FileName << "\"\n";
        }
    }
};

class ExampleASTConsumer : public ASTConsumer {
private:
public:
	explicit ExampleASTConsumer(CompilerInstance *CI) {
		// プリプロセッサからのコールバック登録
		Preprocessor &PP = CI->getPreprocessor();
		PP.addPPCallbacks(llvm::make_unique<PPCallbacksTracker>(PP));
	}
};

class ExampleFrontendAction : public SyntaxOnlyAction /*ASTFrontendAction*/ {
public:
	virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) {
		return llvm::make_unique<ExampleASTConsumer>(&CI); // pass CI pointer to ASTConsumer
	}
};

static cl::OptionCategory MyToolCategory("My tool options");
int main(int argc, const char** argv)
{
	CommonOptionsParser op(argc, argv, MyToolCategory);
	ClangTool Tool(op.getCompilations(), op.getSourcePathList());
	return Tool.run(newFrontendActionFactory<ExampleFrontendAction>().get());
}


