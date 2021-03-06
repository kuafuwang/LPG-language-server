#pragma once
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <boost/optional/optional.hpp>
#include <LibLsp/lsp/lsAny.h>


struct lsTextDocumentIdentifier;
struct lsTextDocumentDidChangeParams;
struct lsTextDocumentItem;
struct ProblemHandler;
struct JikesPG2;
struct SearchPolicy;
struct ILexStream;
struct Monitor;
class RemoteEndPoint;
struct Directory;

namespace LPGParser_top_level_ast {
	struct option;
	struct ASTNodeToken;
}

struct Object;
struct AbsolutePath;
struct CompilationUnit;
struct WorkingFile;
struct DidChangeWorkspaceFoldersParams;
struct WorkingFiles;
struct lsDocumentUri;


struct GenerationOptions {
	
	

	// Search template  path for the LPG tool.
	boost::optional< std::string> template_search_directory ;

	// Search inlcude  path for the LPG tool.
	boost::optional< std::string > include_search_directory ;


	// Package or namespace name for generated files (default: none).
	boost::optional< std::string >package ;

	boost::optional< std::string >language;

	boost::optional< std::string >visitor ;

	boost::optional< std::string > trace;

	boost::optional< bool > quiet;

	boost::optional< bool > verbose;

	// Any additional parameter you want to send to LPG for generation (e.g. "-lalr=3").
	boost::optional< std::string >additionalParameters ;
	
};

namespace lsp {
	class Log;
}

struct WorkSpaceManagerData;


struct WorkSpaceManager {

	void OnOpen(lsTextDocumentItem&, Monitor* monitor);
	void OnChange(const lsTextDocumentDidChangeParams& params, Monitor* monitor);
	void OnClose(const lsTextDocumentIdentifier& close);
	void OnSave(const lsTextDocumentIdentifier& _save);
	void OnDidChangeWorkspaceFolders(const DidChangeWorkspaceFoldersParams&);
	
	
	std::shared_ptr<CompilationUnit> CreateUnit(const AbsolutePath& path, Monitor* monitor);

	std::shared_ptr<CompilationUnit> lookupImportedFile(Directory& directory, const std::string& fileName, Monitor* monitor);
	Object* findAndParseSourceFile(Directory& directory, const std::string& fileName, Monitor* monitor);
	std::vector<Object*> findDefOf(const SearchPolicy& , std::wstring id, const std::shared_ptr<CompilationUnit>& unit, Monitor* monitor);
	std::vector<Object*> findDefOf(const SearchPolicy&, LPGParser_top_level_ast::ASTNodeToken* s, const std::shared_ptr<CompilationUnit>& unit, Monitor* monitor);
	std::shared_ptr<CompilationUnit> find(const AbsolutePath& path);
	std::shared_ptr<CompilationUnit> find_or_open(const AbsolutePath& path, Monitor* monitor);

	
	WorkSpaceManager(RemoteEndPoint& , lsp::Log&);
	~WorkSpaceManager();
	



	std::shared_ptr<CompilationUnit> Build(std::shared_ptr<WorkingFile>&, Monitor* monitor);
	std::vector<Directory> GetIncludeDirs()const;
	RemoteEndPoint& GetEndPoint()const;
	std::shared_ptr<CompilationUnit> FindFile(ILexStream*);


	void UpdateSetting(const GenerationOptions&) const;
	GenerationOptions& GetSetting() const;

	void AddAsReferenceTo(const AbsolutePath& from, const AbsolutePath& reference);

	void RemoveDependency(const AbsolutePath& from);

	std::set<AbsolutePath> GetReference(const AbsolutePath& from);
	std::vector<AbsolutePath> GetAffectedReferences(const AbsolutePath& from);

	void collect_options(std::stack<LPGParser_top_level_ast::option*>&,
		const std::shared_ptr<CompilationUnit>& refUnit,
		Monitor* monitor);

private:
	WorkSpaceManagerData* d_ptr = nullptr;
    std::vector< Object*>	 findDefOf_internal(const SearchPolicy&, std::wstring _word, const std::shared_ptr<CompilationUnit>& unit, Monitor* monitor);
	void collect_def(std::set<AbsolutePath>& includedFiles, std::vector<Object*>& result, const SearchPolicy& policy,
	                 std::wstring id, const std::shared_ptr<CompilationUnit>& refUnit, Monitor* monitor);
	std::shared_ptr<CompilationUnit> ProcessFileContent(std::shared_ptr<WorkingFile>& _change, std::wstring&&, Monitor* monitor);
	bool IsFileRecursiveInclude(const std::shared_ptr<CompilationUnit>& , std::vector<AbsolutePath>& footprint, Monitor* );
	void ProcessCheckFileRecursiveInclude(std::set<AbsolutePath>& includedFiles, std::vector<AbsolutePath>& footprint,
	                                      const std::shared_ptr<CompilationUnit>& refUnit, Monitor* monitor);


	

};

