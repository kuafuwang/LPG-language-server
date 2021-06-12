#pragma once
#include <LibLsp/lsp/symbol.h>
#include <vector>
#include <LibLsp/lsp/lsp_completion.h>
#include <LibLsp/lsp/textDocument/foldingRange.h>
#include <LibLsp/lsp/textDocument/documentColor.h>

struct Monitor;
struct lsFormattingOptions;
struct FoldingRangeRequestParams;
struct lsCompletionParams;
struct CompletionList;
struct CompilationUnit;
struct WorkSpaceManager;

struct SemanticTokens;

namespace TextDocumentHover
{
	struct Result;

}
void process_symbol(std::shared_ptr<CompilationUnit>&, std::vector< lsDocumentSymbol >&);

void process_definition(std::shared_ptr<CompilationUnit>&,const  lsPosition& position,
	std::vector<lsLocation>&, Monitor* monitor);

void process_hover(std::shared_ptr<CompilationUnit>&, const lsPosition& , 
	TextDocumentHover::Result&, Monitor* monitor);

struct CompletionHandler
{
	
	CompletionHandler(std::shared_ptr<CompilationUnit>&, CompletionList& out, const lsCompletionParams&, Monitor* monitor);
private:

	struct Region
	{
		/** The region offset */
		int fOffset;
		/** The region length */
		int fLength;
		/**
	* Create a new region.
	*
	* @param offset the offset of the region
	* @param length the length of the region
	*/
		Region(int _offset, int _length) {
			fOffset = _offset;
			fLength = _length;
		}
		
		int getLength() const {
			return fLength;
		}

	
		 int getOffset() const{
			return fOffset;
		}
	};
	void  MakeCompletionItem(const std::string& proposal, const std::string& newText,
	                         const std::string& prefix,
	                         const Region& region, int cursorLoc, const std::string& addlInfo);
	void  MakeCompletionItem(const std::string& proposal, const std::string& newText,
	                         const std::string& prefix,
	                         const Region& region, const std::string& addlInfo);
	void MakeCompletionItem(const std::string& newText, const std::string& prefix, int offset);
	std::shared_ptr<CompilationUnit>& unit;
	CompletionList& out;
	int  offset = 0;
	void computeSegmentCompletions(const std::string& prefix);
	void computeOptionKeyProposals(const std::string& prefix);
	void computeMacroCompletions(const std::string& prefix);
	void computeNonTerminalCompletions(const std::string& prefix);
	void computeTerminalCompletions(const std::string& prefix);
};
struct FoldingRangeHandler
{

	FoldingRangeHandler(std::shared_ptr<CompilationUnit>&, std::vector<FoldingRange>& , const FoldingRangeRequestParams&);


	std::shared_ptr<CompilationUnit>& unit;
	std::vector<FoldingRange>& out;
};
struct DocumentFormatHandler
{

	DocumentFormatHandler(std::shared_ptr<CompilationUnit>&, std::vector<lsTextEdit>&, const lsFormattingOptions&);


	std::shared_ptr<CompilationUnit>& unit;
	std::vector<lsTextEdit>& out;
};
struct DocumentColorHandlerData;
struct DocumentColorHandler
{

	DocumentColorHandler(std::shared_ptr<CompilationUnit>&, std::vector<ColorInformation>&);
	~DocumentColorHandler();
	DocumentColorHandlerData* d_ptr;

};
struct SemanticTokensHandler
{
	struct Data;

	SemanticTokensHandler(std::shared_ptr<CompilationUnit>&, SemanticTokens&);
	~SemanticTokensHandler();
	Data* d_ptr;

};
struct ReferencesHandlerData;
struct ReferencesHandler
{

	ReferencesHandler(std::shared_ptr<CompilationUnit>&, const lsPosition& position, std::vector<lsLocation>&, Monitor* monitor);
	~ReferencesHandler();
	ReferencesHandlerData* d_ptr;

};
