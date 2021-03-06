#pragma once


#include <LibLsp/lsp/symbol.h>
#include <LibLsp/lsp/working_files.h>

#include "parser/JiksPGControl.h"
#include "parser/LPGLexer.h"
#include "parser/LPGParser.h"

namespace LPGParser_top_level_ast {
	struct ASTNode;
	struct LPG;
}

namespace BuildInMacroName
{
	inline const char* entry_declarations_string = "entry_declarations";
	inline const char* rule_number_string = "rule_number";
	inline const char* rule_text_string = "rule_text";
	inline const char* rule_size_string = "rule_size";
	inline const char* input_file_string = "input_file";
	inline const char* current_line_string = "current_line";
	inline const char* next_line_string = "next_line";
	inline const char* identifier_string = "#identifier";
	inline const char* symbol_declarations_string = "symbol_declarations";
	inline const char* num_rules_string = "num_rules";
	inline const char* num_terminals_string = "num_terminals";
	inline const char* num_nonterminals_string = "num_nonterminals";
	inline const char* num_non_terminals_string = "num_non_terminals";
	inline const char* num_symbols_string = "num_symbols";
	inline const char* template_string = "template";
	inline const char* file_prefix_string = "file_prefix";
	inline const char* package_string = "package";
	inline const char* ast_package_string = "ast_package";
	inline const char* ast_type_string = "ast_type";
	inline const char* exp_type_string = "exp_type";
	inline const char* prs_type_string = "prs_type";
	inline const char* sym_type_string = "sym_type";
	inline const char* action_type_string = "action_type";
	inline const char* visitor_type_string = "visitor_type";
	inline const char* prefix_string = "prefix";
	inline const char* suffix_string = "suffix";

	inline const char* entry_name_string = "entry_name";
	inline const char* entry_marker_string = "entry_marker";

};
struct WorkingFile;
struct WorkSpaceManager;
struct SearchPolicy;
struct CompilationUnit : Object,std::enable_shared_from_this<CompilationUnit>
{
	std::shared_ptr<CompilationUnit> getptr();
	std::atomic<long long> counter = 0;
	bool NeedToCompile() const;
	string getName();
	CompilationUnit(std::shared_ptr<WorkingFile>& file, WorkSpaceManager&);
	void insert_local_macro(const char* name);
	void parser(Monitor* monitor);
	/**
 * Get the target for a given source node in the AST represented by a given
 * Parse Controller.
 */
	std::vector<Object*> getLinkTarget(SearchPolicy& policy, Object* node, Monitor* monitor);

	struct FindMacroInBlockResult
	{
		std::vector<Object*> def_set;
		std::wstring macro_name;
	};

	int FindExportMacro(const std::wstring& name, std::vector<Object*>&);
	std::unique_ptr<FindMacroInBlockResult> FindMacroInBlock(
		Object* , const lsPosition&, Monitor* monitor);

	bool is_macro_name_symbol(LPGParser_top_level_ast::ASTNodeToken* node);

	void FindIn_noTerm(const std::wstring& name, std::vector<Object*>& candidates);

	void FindIn_Term(const std::wstring& name, std::vector<Object*>& candidates);
	
	void FindIn_Export(const std::wstring& name, std::vector<Object*>& candidates);
	
	void FindIn_define(const std::wstring& name, std::vector<Object*>& candidates);
	
	std::vector<Object*> FindDefine(const SearchPolicy& policy, const std::wstring& name);
	std::vector<Object*>   FindDefineIn_Term_and_noTerms(const std::wstring& name);
	
	//friend struct LPGBindingVisitor;
	std::shared_ptr<JikesPG2> GetBinding();


	std::unordered_multimap<std::wstring, LPGParser_top_level_ast::ASTNode*>  export_term;
	std::unordered_multimap<std::wstring, LPGParser_top_level_ast::ASTNode*>  import_term;
	std::set<std::string> local_macro_name_table;
	std::unordered_multimap<std::wstring, Object*> export_macro_table;

	
	std::shared_ptr<WorkingFile> working_file;
	
	WorkSpaceManager& parent;

	std::shared_ptr<RunTimeUnit> runtime_unit = std::make_shared<RunTimeUnit>();
	
	std::vector< lsDocumentSymbol > document_symbols;

	void removeDependency();
	DependenceInfo dependence_info;
	std::string fileName;
	void  ResetBinding();
private:
	std::shared_ptr<JikesPG2> data;
};
