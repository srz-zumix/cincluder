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

#include <functional>

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace llvm;

// ***************************************************************************
//          プリプロセッサからのコールバック処理
// ***************************************************************************

class cincluder : public PPCallbacks {
private:
    Preprocessor &PP;

	typedef ::std::hash< ::std::string > hash;
	typedef ::std::list< hash::result_type > hash_list;

	struct header
	{
		header() : angled(false) {}
		header(const ::std::string& n, bool angled_) : name(n), angled(angled_) {}
		::std::string name;
		bool angled;
		hash_list include;
	};
	typedef ::std::map< hash::result_type, header > Map;
	typedef ::std::map< hash::result_type, hash_list > Depend;
	Map m_includes;
	Depend m_depends;
	hash::result_type m_root;
public:
	cincluder(Preprocessor &pp) : PP(pp) {}

	::std::string getFilePath(hash::result_type h)
	{
		return m_includes[h].name;
	}
	::std::string getFileName(hash::result_type h)
	{
		::std::string path = getFilePath(h);
		size_t dpos1 = path.rfind('\\');
		size_t dpos2 = path.rfind('/');
		if( dpos1 == ::std::string::npos ) dpos1 = 0;
		if( dpos2 == ::std::string::npos ) dpos2 = 0;
		const size_t dpos = (::std::max)(dpos1, dpos2) + 1;

		return path.substr( dpos );
	}

	void printRoot(hash::result_type h, int indent, bool expand)
	{
		if( indent > 2 ) return;
		for( int i = 0; i < indent; ++i ) errs() << "  ";
		if( expand ) errs() << "+ ";
		else errs() << "  ";
		errs() << getFileName(h) << "\n";

		if( m_depends.find(h) != m_depends.end() )
		{
			auto depend = m_depends[h];
			if( depend.size() > 1 ) ++indent;
			for( auto d : depend )
			{
				printRoot(d, indent, (depend.size() > 1));
			}
		}
	}

	void report()
	{
		for( auto h : m_depends )
		{
			if( h.second.size() > 1 )
			{
				errs() << getFileName(h.first) << " is already include by \n";
				for( auto d : h.second )
				{
					printRoot(d, 1, true);
				}
			}
		}
	}

	void dot()
	{
	}

	void EndOfMainFile() override
	{
		report();
	}

    void InclusionDirective(SourceLocation HashLoc,
                          const Token &IncludeTok,
                          llvm::StringRef FileName, 
                          bool IsAngled,
                          CharSourceRange FilenameRange,
                          const FileEntry *File,
                          llvm::StringRef SearchPath,
                          llvm::StringRef RelativePath,
                          const Module *Imported) override
	{
		if( File == nullptr ) return;

		SourceManager& SM = PP.getSourceManager();
		const FileEntry* pFromFile = SM.getFileEntryForID(SM.getFileID(SM.getExpansionLoc(HashLoc)));
		if( pFromFile == nullptr ) return;

		const auto h = hash()(File->getName());
		const auto p = hash()(pFromFile->getName());
		{
			if( m_includes.find(h) == m_includes.end() )
			{
				m_includes.insert(::std::make_pair(h, header(File->getName(), IsAngled)));
			}
			//else if( !IsAngled )
			//{
			//	auto it = m_depends.find(h);
			//	if( it != m_depends.end() )
			//	{
			//		errs() << FileName << " is aleady include by \n";
			//		errs() << "\t" << pFromFile->getName() << "\n";
			//		for( auto f : it->second )
			//		{
			//			auto m = m_includes.find(f);
			//			if( m != m_includes.end() )
			//			{
			//				errs() << "\t" << m->second.name << "\n";
			//			}
			//		}
			//	}
			//}

			auto it = m_includes.find(p);
			if( it == m_includes.end() )
			{
				m_root = p;
				m_includes.insert(::std::make_pair(p, header(pFromFile->getName(), false)));
			}
			if( it != m_includes.end() )
			{
				it->second.include.push_back(h);
			}
		}
		if( !IsAngled )
		{
			auto it = m_depends.find(h);
			if( it != m_depends.end() )
			{
				it->second.push_back(p);
			}
			else
			{
				hash_list a;
				a.push_back(p);
				m_depends.insert(::std::make_pair(h, a));
			}
		}


#if 0
        errs() << "InclusionDirective : ";
        if (File) {
            if (IsAngled)   errs() << "<" << File->getName() << ">\n";
            else            errs() << "\"" << File->getName() << "\"\n";
        } else {
            errs() << "not found file ";
            if (IsAngled)   errs() << "<" << FileName << ">\n";
            else            errs() << "\"" << FileName << "\"\n";
        }
#endif
    }
};

class ExampleASTConsumer : public ASTConsumer {
private:
public:
	explicit ExampleASTConsumer(CompilerInstance *CI) {
		// プリプロセッサからのコールバック登録
		Preprocessor &PP = CI->getPreprocessor();
		PP.addPPCallbacks(llvm::make_unique<cincluder>(PP));
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


