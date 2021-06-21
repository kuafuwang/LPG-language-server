#pragma once
#include <LibLsp/lsp/symbol.h>
#include <vector>
#include <LibLsp/lsp/lsp_completion.h>
#include <LibLsp/lsp/lsWorkspaceEdit.h>
#include <LibLsp/lsp/textDocument/foldingRange.h>
#include <LibLsp/lsp/textDocument/documentColor.h>
#include <LibLsp/lsp/textDocument/rename.h>
#include "LibLsp/JsonRpc/RequestInMessage.h"
#include "LibLsp/JsonRpc/lsResponseMessage.h"

#include "LibLsp/lsp/lsWorkspaceEdit.h"
#include "LibLsp/lsp/lsTextDocumentIdentifier.h"
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

struct ReferencesHandler
{

	ReferencesHandler(std::shared_ptr<CompilationUnit>&, const lsPosition& , std::vector<lsLocation>&, Monitor* );

	


	struct Data;
	std::shared_ptr<Data>  d_ptr;

};

struct RenameHandler
{

	
	RenameHandler(std::shared_ptr<CompilationUnit>&, const TextDocumentRename::Params&,
		std::vector< lsWorkspaceEdit::Either >&, Monitor*);

	
	struct Data;

	std::shared_ptr<Data>  d_ptr;

};



namespace InlineNonTerminal {
	struct Params {
		// The document to format.
		lsTextDocumentIdentifier textDocument;

		// The position at which this request was sent.
		lsPosition position;

		bool	fInlineAll =false;
		MAKE_SWAP_METHOD(Params,
			textDocument,
			position,
			fInlineAll)
	};
};
MAKE_REFLECT_STRUCT(InlineNonTerminal::Params,
	textDocument,
	position,
	fInlineAll);




/**
 * The rename request is sent from the client to the server to do a
 * workspace wide rename of a symbol.
 *
 * Registration Options: TextDocumentRegistrationOptions
 */
DEFINE_REQUEST_RESPONSE_TYPE(lpg_inlineNonTerminal, InlineNonTerminal::Params, lsWorkspaceEdit, "lpg/inlineNonTerminal");


struct InlineNonTerminalHandler
{

	InlineNonTerminalHandler(std::shared_ptr<CompilationUnit>&, const InlineNonTerminal::Params&,
		std::vector< lsWorkspaceEdit::Either >&, Monitor*);

	struct Data;

	std::shared_ptr<Data>  d_ptr;

};



namespace MakeEmptyNonTerminal {
	struct Params {
		// The document to format.
		lsTextDocumentIdentifier textDocument;

		// The position at which this request was sent.
		lsPosition position;

		MAKE_SWAP_METHOD(Params,
			textDocument,
			position)
	};
};
MAKE_REFLECT_STRUCT(MakeEmptyNonTerminal::Params,
	textDocument,
	position);




/**
 * The rename request is sent from the client to the server to do a
 * workspace wide rename of a symbol.
 *
 * Registration Options: TextDocumentRegistrationOptions
 */
DEFINE_REQUEST_RESPONSE_TYPE(lpg_makeEmpty, MakeEmptyNonTerminal::Params, lsWorkspaceEdit, "lpg/makeEmpty");


struct MakeEmptyNonTerminalHandler
{

	MakeEmptyNonTerminalHandler(std::shared_ptr<CompilationUnit>&, const MakeEmptyNonTerminal::Params&,
		std::vector< lsWorkspaceEdit::Either >&, Monitor*);

	struct Data;

	std::shared_ptr<Data>  d_ptr;

};



namespace MakeNonEmptyNonTerminal {
	struct Params {
		// The document to format.
		lsTextDocumentIdentifier textDocument;

		// The position at which this request was sent.
		lsPosition position;

		MAKE_SWAP_METHOD(Params,
			textDocument,
			position)
	};
};
MAKE_REFLECT_STRUCT(MakeNonEmptyNonTerminal::Params,
	textDocument,
	position);




/**
 * The rename request is sent from the client to the server to do a
 * workspace wide rename of a symbol.
 *
 * Registration Options: TextDocumentRegistrationOptions
 */
DEFINE_REQUEST_RESPONSE_TYPE(lpg_makeNonEmpty, MakeNonEmptyNonTerminal::Params, lsWorkspaceEdit, "lpg/MakeNonEmpty");


struct MakeNonEmptyNonTerminalHandler
{

	MakeNonEmptyNonTerminalHandler(std::shared_ptr<CompilationUnit>&, const MakeNonEmptyNonTerminal::Params&,
		std::vector< lsWorkspaceEdit::Either >&, Monitor*);

	struct Data;

	std::shared_ptr<Data>  d_ptr;

};




namespace MakeLeftRecursive {
	struct Params {
		// The document to format.
		lsTextDocumentIdentifier textDocument;

		// The position at which this request was sent.
		lsPosition position;

		MAKE_SWAP_METHOD(Params,
			textDocument,
			position)
	};
};
MAKE_REFLECT_STRUCT(MakeLeftRecursive::Params,
	textDocument,
	position);




/**
 * The rename request is sent from the client to the server to do a
 * workspace wide rename of a symbol.
 *
 * Registration Options: TextDocumentRegistrationOptions
 */
DEFINE_REQUEST_RESPONSE_TYPE(lpg_MakeLeftRecursive, MakeLeftRecursive::Params, lsWorkspaceEdit, "lpg/MakeLeftRecursive");


struct MakeLeftRecursiveHandler
{

	MakeLeftRecursiveHandler(std::shared_ptr<CompilationUnit>&, const MakeLeftRecursive::Params&,
		std::vector< lsWorkspaceEdit::Either >&, Monitor*);

	struct Data;

	std::shared_ptr<Data>  d_ptr;

};