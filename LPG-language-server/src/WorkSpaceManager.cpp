#include  "WorkSpaceManager.h"
#include <functional>
#include <Monitor.h>
#include <mutex>
#include <string>
#include <vector>
#include <LibLsp/lsp/AbsolutePath.h>
#include <boost/thread/win32/shared_mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <LibLsp/lsp/textDocument/did_change.h>
#include <LibLsp/lsp/workspace/didChangeWorkspaceFolders.h>
#include <LibLsp/lsp/workspace/workspaceFolders.h>

#include "CompilationUnit.h"
#include "LibLsp/lsp/utils.h"
#include "LibLsp/lsp/working_files.h"
#include "LibLsp/lsp/SimpleTimer.h"
#include "parser/LPGParser_top_level_ast.h"
#include <boost/filesystem.hpp>
#include <LibLsp/JsonRpc/RemoteEndPoint.h>

#include "IMessageHandler.h"
#include "message/MessageHandler.h"
#include <LibLsp/lsp/textDocument/publishDiagnostics.h>
using namespace LPGParser_top_level_ast;
using namespace lsp;
struct WorkSpaceManagerData
{
	WorkSpaceManagerData(WorkingFiles& _w, RemoteEndPoint& _end_point, lsp::Log& _log): working_files(_w),
		end_point(_end_point),
		logger(_log)
	{
	}

	WorkingFiles& working_files;
	RemoteEndPoint& end_point;
	lsp::Log& logger;

	typedef boost::shared_lock<boost::shared_mutex> readLock;
	typedef boost::unique_lock<boost::shared_mutex> writeLock;
	boost::shared_mutex _rw_mutex;

	std::map<std::string, std::shared_ptr<CompilationUnit>> units;
	std::vector<Directory>includeDirs;
	std::shared_ptr<CompilationUnit> FindFile(ILexStream* lex)
	{
		readLock a(_rw_mutex);
		for(auto& it : units)
		{
			if (lex == it.second->_lexer.getILexStream())
				return  it.second;
		}

		return {};
	}
	void update_unit(const std::string& path, std::shared_ptr<CompilationUnit>& unit)
	{

		{
		//	writeLock b(_rw_mutex);
			units[path] = unit;
		}
	}
	
	void remove(const std::string& path)
	{
		{
			writeLock a(_rw_mutex);
			units.erase(path);
		}
	
	}
	std::shared_ptr<CompilationUnit> find(const std::string& path)
	{
		readLock a(_rw_mutex);
		auto findIt = units.find(path);
		if (findIt != units.end())
			return findIt->second;
		return {};
	}
	void remove_in_directory(std::vector<Directory>& directories)
	{
		if (directories.empty()) return;
		
		std::vector<string> files_to_be_delete;
		{
			readLock a(_rw_mutex);
			for(auto& it : units)
			{
				for(auto& dir : directories)
				{
					if (it.second->working_file->directory == dir)
					{
						files_to_be_delete.emplace_back(it.first);
						break;
					}
				}	
			}
		}
		if(files_to_be_delete.empty())return;
		
		{
			writeLock a(_rw_mutex);
			for(auto& it : files_to_be_delete)
			{
				units.erase(it);
			}

		}
		
		
	}
};
std::wstring stripName(const std::wstring& rawId) {
	int idx = rawId.find('$');

	return (idx >= 0) ? rawId.substr(0, idx) : rawId;
}
std::string stripName(const std::string& rawId) {
	int idx = rawId.find('$');

	return (idx >= 0) ? rawId.substr(0, idx) : rawId;
}
std::vector<Object*>  WorkSpaceManager::findDefOf_internal(std::wstring _word,
                                              const std::shared_ptr<CompilationUnit>& refUnit,  Monitor* monitor)
{

	 auto     id = stripName(_word);
	 std::set<std::string> includedFiles;
	 includedFiles.insert(refUnit->working_file->filename);
	collectIncludedFiles(includedFiles,refUnit, monitor);
	includedFiles.erase(refUnit->working_file->filename);
	 for (auto& fileName : includedFiles) 
	 {
		 auto include_unit= lookupImportedFile(refUnit->working_file->directory,fileName,monitor);
		 if(!include_unit)continue;
		 if (monitor && monitor->isCancelled())
			 return {};
		 if (LPG * includedRoot = include_unit->root; includedRoot != nullptr) {
			 std::vector<Object*> candidates = include_unit->FindDefine(id);
			 if (!candidates.empty())
				 return candidates;
		 }
	 }
	 return {};
	
}



std::shared_ptr<CompilationUnit> WorkSpaceManager::CreateUnit(const AbsolutePath& fileName, Monitor* monitor)
{
	auto  unit = find(AbsolutePath(fileName, false));
	if (unit) return unit;
	boost::system::error_code ec;

	if (!boost::filesystem::exists(fileName.path, ec))
	{
		return  {};
	}
	std::wstring content;
	if(!IcuUtil::getFileUnicodeContent(IcuUtil::s2ws(fileName.path).c_str(), content))
	{
		return {};
	}
	lsTextDocumentItem item;
	item.uri= fileName;
	item.text = IcuUtil::ws2s(content);
	std::shared_ptr<WorkingFile> _open=d_ptr->working_files.OnOpen(item);
	if (!_open)
		return {};
	return OnChange(_open,std::move(content), monitor);
}

void WorkSpaceManager::collectIncludedFiles(std::set<std::string>& result, const std::shared_ptr<CompilationUnit>& refUnit,  Monitor* monitor)
{
	if(!refUnit){
		return;
	}
	auto root = refUnit->root;
	if (!root){
		return;
	}
	if(monitor && monitor->isCancelled())
		return;
	
	auto optSeg = static_cast<option_specList*>(root->getoptions_segment());
	for (int i = 0; i < optSeg->size(); i++) {
		auto optSpec = optSeg->getoption_specAt(i);
		auto optList = optSpec->getoption_list();
		for (int o = 0; o < optList->size(); o++) {
			auto opt =optList->getoptionAt(o);
			auto sym = opt->getSYMBOL();
			auto  optName = sym->toString();

			if (optName==(L"import_terminals")
				|| optName==(L"template")
				|| optName==(L"filter")) 
			{
				auto  optValue = opt->getoption_value();
				if ( dynamic_cast<option_value0*>(optValue)) {
					string fileName;
					fileName = static_cast<option_value0*>(optValue)->getSYMBOL()->to_utf8_string();
					if(result.find(fileName) != result.end()) 
						continue;
					result.insert(fileName);
					if (optName == (L"import_terminals")) {
						// pick up defs from the filter
						if (monitor && monitor->isCancelled())
							return;
						auto filterUnit = lookupImportedFile(refUnit->working_file->directory, fileName,monitor);
						if (monitor && monitor->isCancelled())
							return;
						 collectIncludedFiles(result, filterUnit,monitor);
					}
				}
			}
		}
	}
}



std::shared_ptr<CompilationUnit> WorkSpaceManager::lookupImportedFile(Directory& directory, const std::string& fileName, Monitor* monitor)
{
	boost::system::error_code ec;
	
	auto  unit = find(AbsolutePath(fileName,false ));
	if (unit) return unit;
	
	if(boost::filesystem::exists(fileName, ec))
	{
		return  CreateUnit(AbsolutePath(fileName, false),monitor);
	}
	
	{
		
		string refPath(directory.path);
		refPath.append(fileName);
		unit = find(AbsolutePath(refPath, false));
		if (unit) return unit;
		
		if (boost::filesystem::exists(refPath, ec))
		{
			return  CreateUnit(AbsolutePath(refPath, false),monitor);
		}
	}


	for(auto & it :  d_ptr-> includeDirs)
	{
		string refPath(it.path);
		refPath.append(fileName);
		unit = find(AbsolutePath(refPath, false));
		if (unit) return unit;
		if (!boost::filesystem::exists(refPath, ec))
		{
			return  CreateUnit(AbsolutePath(refPath, false),monitor);
		}
	}
	return {};
}
Object* WorkSpaceManager::findAndParseSourceFile(Directory& directory, const std::string& fileName, Monitor* monitor)
{
	auto unit = lookupImportedFile(directory, fileName,monitor);
	if(unit && unit->root)
	{
		return  unit->root;
	}
	return nullptr;
}

std::vector<Object*> WorkSpaceManager::findDefOf(std::wstring id, const std::shared_ptr<CompilationUnit>& unit, Monitor* monitor)
{
	std::vector<Object*> candidates = unit->FindDefine(id);;
	if (candidates.empty()) {
		// try a little harder
		auto def_set = findDefOf_internal(id, unit,monitor);
		if (!def_set.empty())
			return def_set;
	}
	return  candidates;
}

std::vector<Object*> WorkSpaceManager::findDefOf(ASTNodeToken* s, const std::shared_ptr<CompilationUnit>& unit, Monitor* monitor)
{
	auto     id = stripName(s->toString());
	std::vector<Object*> candidates = unit->FindDefine(id);
	if (candidates.empty()) {
		// try a little harder
		auto def_set =  findDefOf_internal(s->toString(), unit, monitor);
		if (!def_set.empty())
			return def_set;
	}
	else
	{
		std::vector<Object*> temp = candidates;
		for(auto& it : candidates)
		{
			if (s->parent != it)
				continue;
			// just found the same spot;
			auto def_set = findDefOf_internal(s->toString(), unit, monitor);
			for(auto& it : def_set)
			{
				temp.push_back(it);
			}
		}
		candidates.swap(temp);
	}


	if (!candidates.empty())
		return  candidates;
	
	auto node = s;
	auto parent = static_cast<ASTNode*>(node->getParent());
	if (!parent)return {};
	
	auto grandParent = static_cast<ASTNode*>(parent->getParent());

	if ( dynamic_cast<option*>(grandParent)) {
		auto opt = static_cast<option*>(grandParent);
		auto optName = opt->getSYMBOL()->toString();

		if (optName==(L"import_terminals")
			|| optName==(L"template")
			|| optName ==(L"filter")) 
		{
			auto temp_file =lookupImportedFile(unit->working_file->directory, IcuUtil::ws2s(id), monitor).get();
			if(temp_file)
			{
				return { temp_file };
			}
			else
			{
				return {};
			}
		}
	}
	else if (dynamic_cast<IncludeSeg*>(parent)) {
		auto iseg = static_cast<IncludeSeg*>(parent);
		
		auto includeFile = static_cast<include_segment*>(iseg->getinclude_segment())->getSYMBOL()
			->to_utf8_string();

		auto temp_file= lookupImportedFile(unit->working_file->directory, includeFile, monitor).get();
		if (temp_file)
		{
			return { temp_file };
		}
		else
		{
			return {};
		}
	}
	return {};
}

std::shared_ptr<CompilationUnit> WorkSpaceManager::find(const AbsolutePath& path)
{
	std::stringex temp(path.path);
	temp.replace("\\", "/");
	return  d_ptr->find(temp);
}

std::shared_ptr<CompilationUnit> WorkSpaceManager::find_or_open(const AbsolutePath& path, Monitor* monitor)
{
	auto unit = find(path);
	if(!unit)
	{
		unit = CreateUnit(path, monitor);
	}
	return  unit;
}

WorkSpaceManager::WorkSpaceManager(WorkingFiles& _w, RemoteEndPoint& end_point, lsp::Log& _log):d_ptr(new WorkSpaceManagerData(_w , end_point, _log))
{
	
}

WorkSpaceManager::~WorkSpaceManager()
{
	delete d_ptr;
}

std::shared_ptr<CompilationUnit> WorkSpaceManager::OnOpen(std::shared_ptr<WorkingFile>& _open, Monitor* monitor)
{
	return OnChange(_open, monitor);
}

std::shared_ptr<CompilationUnit> WorkSpaceManager::OnChange(std::shared_ptr<WorkingFile>& _change, Monitor* monitor)

{
	return OnChange(_change,{}, monitor);
}
extern void  process_type_binding(std::shared_ptr<CompilationUnit>& unit, ProblemHandler* handler);

std::shared_ptr<CompilationUnit> WorkSpaceManager::OnChange(std::shared_ptr<WorkingFile>& _change, std::wstring&& content , Monitor* monitor)
{

	if (!_change)
		return {};
	if (monitor && monitor->isCancelled())
		return {};
	ProblemHandler handle;
	std::shared_ptr<CompilationUnit> unit;
	{
		WorkSpaceManagerData::writeLock b(d_ptr->_rw_mutex);

		auto findIt = d_ptr->units.find(_change->filename);
		if (findIt != d_ptr->units.end())
		{
			if (findIt->second->counter.load(std::memory_order_relaxed) == _change->counter.load(std::memory_order_relaxed))
			{
				return  findIt->second;
			}
		}
		if (content.empty())
		{
			if (!_change->parent.GetFileBufferContent(_change, content))
			{
				return {};
			}
		}


		unit = std::make_shared<CompilationUnit>(_change, *this);
		unit->counter.store(_change->counter.load(std::memory_order_relaxed));

		handle.notify.params.uri = _change->filename;

		unit->_lexer.reset(content, IcuUtil::s2ws(_change->filename));

		unit->_parser.reset(unit->_lexer.getLexStream());

		unit->_lexer.getLexStream()->setMessageHandler(&handle);
		unit->_parser.getIPrsStream()->setMessageHandler(&handle);
		if (monitor && monitor->isCancelled())
			return {};

		unit->parser(monitor);
		if (monitor && monitor->isCancelled())
			return {};
		d_ptr->update_unit(_change->filename, unit);
	}
	
	process_type_binding(unit, &handle);
	d_ptr->end_point.sendNotification(handle.notify);
	return unit;
}

void WorkSpaceManager::OnClose(const lsDocumentUri& close)
{
	d_ptr->remove(close.GetAbsolutePath());
}

void WorkSpaceManager::OnSave(const lsDocumentUri& _save)
{
	
}

void WorkSpaceManager::OnDidChangeWorkspaceFolders(const DidChangeWorkspaceFoldersParams& params)
{

	std::vector<Directory> remove;
	for (auto& it : params.event.removed)
	{
		remove.emplace_back(Directory(it.uri.GetAbsolutePath()));
	}
	d_ptr->remove_in_directory(remove);
}

void WorkSpaceManager::UpdateIncludePaths(const std::vector<Directory>& dirs)
{
	d_ptr->includeDirs = dirs;
}

RemoteEndPoint& WorkSpaceManager::GetEndPoint() const
{
	return  d_ptr->end_point;
}

std::shared_ptr<CompilationUnit> WorkSpaceManager::FindFile(ILexStream* lex)
{
	return  d_ptr->FindFile(lex);
}


