﻿
#include "LibLsp/lsp/general/exit.h"
#include "LibLsp/lsp/textDocument/declaration_definition.h"
#include "LibLsp/lsp/textDocument/signature_help.h"
#include "LibLsp/lsp/general/initialize.h"
#include "LibLsp/lsp/ProtocolJsonHandler.h"
#include "LibLsp/lsp/textDocument/typeHierarchy.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "LibLsp/lsp/textDocument/resolveCompletionItem.h"
#include <network/uri.hpp>
#include "LibLsp/JsonRpc/Condition.h"
#include "LibLsp/lsp/textDocument/did_change.h"
#include "LibLsp/lsp/textDocument/did_save.h"	
#include "LibLsp/JsonRpc/Endpoint.h"
#include "LibLsp/JsonRpc/stream.h"
#include "LibLsp/JsonRpc/TcpServer.h"
#include "LibLsp/lsp/textDocument/document_symbol.h"
#include "LibLsp/lsp/workspace/execute_command.h"
#include "LibLsp/lsp/textDocument/declaration_definition.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "LibLsp/lsp/textDocument/document_symbol.h"
#include "LibLsp/lsp/workspace/didChangeWorkspaceFolders.h"
#include "WorkSpaceManager.h"
#include "timer.h"
#include "utils.h"
#include "working_files.h"
#include "IcuUtil.h"
#include "message/MessageHandler.h"
using namespace boost::asio::ip;
using namespace std;
class DummyLog :public lsp::Log
{
public:

	void log(Level level, std::wstring&& msg)
	{
		std::wcout << msg << std::endl;
	};
	void log(Level level, const std::wstring& msg)
	{
		std::wcout << msg << std::endl;
	};
	void log(Level level, std::string&& msg)
	{
		std::cout << msg << std::endl;
	};
	void log(Level level, const std::string& msg)
	{
		std::cout << msg << std::endl;
	};
};

std::string _address = "127.0.0.1";
std::string _port = "9333";
bool ShouldIgnoreFileForIndexing(const std::string& path) {
	return StartsWith(path, "git:");
}

class Server
{
public:
	WorkingFiles working_files;
	WorkSpaceManager   project;
	Server() :server(_address, _port, protocol_json_handler, endpoint, _log), project(working_files, _log)
	{
		server.remote_end_point_.registerRequestHandler([&](const td_initialize::request& req)
			->lsp::ResponseOrError< td_initialize::response > {
				td_initialize::response rsp;
				CodeLensOptions code_lens_options;
				code_lens_options.resolveProvider = true;
				rsp.result.capabilities.codeLensProvider = code_lens_options;

				return rsp;
			});
		server.remote_end_point_.registerRequestHandler([&](const td_symbol::request& req)
			->lsp::ResponseOrError< td_symbol::response > {
				auto unit = project.find(req.params.textDocument.uri.GetAbsolutePath());
			    if(!unit)
			    {
					Rsp_Error error;
					return  error;
			    }
			    td_symbol::response rsp;
				process_symbol(unit, rsp.result);
				return rsp;
			});
		server.remote_end_point_.registerRequestHandler([&](const td_definition::request& req)
			->lsp::ResponseOrError< td_definition::response > {
				td_definition::response rsp;
				
				return rsp;
			});
		

		server.remote_end_point_.registerNotifyHandler([&](Notify_TextDocumentDidOpen::notify& notify)
			{
				
				const auto& params = notify.params;
				AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
				if (ShouldIgnoreFileForIndexing(path))
					return;
				if(auto work_file = working_files.OnOpen(params.textDocument))
			    {
					std::wstring context;
					working_files.GetFileBufferContent(work_file, context);
					project.OnOpen(work_file, std::move(context));
			    }
					
				// 解析 
			});
		
		server.remote_end_point_.registerNotifyHandler([&]( Notify_TextDocumentDidChange::notify& notify)
		{
				Timer time;
				const auto& params = notify.params;
				AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
				if (ShouldIgnoreFileForIndexing(path))
					return;
				if(auto work_file= working_files.OnChange(notify.params))
				{
					std::wstring context;
					working_files.GetFileBufferContent(work_file, context);
					project.OnChange(work_file, std::move(context));
				}
			
		});
		server.remote_end_point_.registerNotifyHandler([&](Notify_TextDocumentDidClose::notify& notify)
		{
				Timer time;
				const auto& params = notify.params;
				AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
				if (ShouldIgnoreFileForIndexing(path))
					return;
				working_files.OnClose(params.textDocument);
				project.OnClose(notify.params.textDocument.uri);
			    // 通知消失了
		});
		
		server.remote_end_point_.registerNotifyHandler([&](Notify_TextDocumentDidSave::notify& notify)
			{
				Timer time;
				const auto& params = notify.params;
				AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
				if (ShouldIgnoreFileForIndexing(path))
					return;
				working_files.OnClose(params.textDocument);
				project.OnSave(params.textDocument.uri);
				// 通知消失了
			});
		server.remote_end_point_.registerNotifyHandler([&](Notify_WorkspaceDidChangeWorkspaceFolders::notify& notify)
		{
				std::vector<Directory> remove;
			    for(auto& it :notify.params.event.removed)
			    {
					remove.push_back(Directory(it.uri.GetAbsolutePath()));
			    }
				working_files.CloseFilesInDirectory(remove);
				project.OnDidChangeWorkspaceFolders(notify.params);
		});

		server.remote_end_point_.registerNotifyHandler([&](Notify_Exit::notify& notify)
		{
				server.stop();
				esc_event.notify(std::make_unique<bool>(true));
		});

		std::thread([&]()
			{
				server.run();
			}).detach();
	}
	~Server()
	{
		server.stop();
	}
	std::shared_ptr < lsp::ProtocolJsonHandler >  protocol_json_handler = std::make_shared < lsp::ProtocolJsonHandler >();
	DummyLog  _log;

	std::shared_ptr < GenericEndpoint >  endpoint = std::make_shared<GenericEndpoint>(_log);
	lsp::TcpServer server;
	Condition<bool> esc_event;
};


int main(int argc, char* argv[])
{

	using namespace  boost::program_options;
	options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message");



	variables_map vm;
	try {
		store(parse_command_line(argc, argv, desc), vm);
	}
	catch (std::exception& e) {
		std::cout << "Undefined input.Reason:" << e.what() << std::endl;
		return 0;
	}
	notify(vm);


	if (vm.count("help"))
	{
		cout << desc << endl;
		return 1;
	}
	Server server;
	server.esc_event.wait();
	return 0;
}

