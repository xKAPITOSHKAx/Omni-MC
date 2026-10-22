#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <propkey.h>
#include <wincodec.h>
#include <uiautomation.h>
#include <tlhelp32.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include "resource.h"
#pragma comment(lib,"winhttp.lib")
using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Storage::Streams;
namespace fs = std::filesystem;
using P = void*;
#define API(ret,name,...) static ret (*name)(__VA_ARGS__)
API(P,obs_data_create_from_json,const char*);
API(P,obs_data_create);
API(void,obs_data_release,P);
API(const char*,obs_data_get_json,P);
API(const char*,obs_data_get_string,P,const char*);
API(bool,obs_data_get_bool,P,const char*);
API(long long,obs_data_get_int,P,const char*);
API(double,obs_data_get_double,P,const char*);
API(void,obs_data_set_string,P,const char*,const char*);
API(void,obs_data_set_bool,P,const char*,bool);
API(void,obs_data_set_int,P,const char*,long long);
API(P,obs_get_source_by_name,const char*);
API(const char*,obs_source_get_name,P);
API(const char*,obs_source_get_id,P);
API(void,obs_source_set_name,P,const char*);
API(void,obs_enum_sources,bool(*)(P,P),P);
API(void,obs_enum_scenes,bool(*)(P,P),P);
API(void,obs_source_release,P);
API(P,obs_source_create,const char*,const char*,P,P);
API(void,obs_source_update,P,P);
API(P,obs_scene_from_source,P);
API(P,obs_scene_add,P,P);
API(P,obs_scene_find_source,P,const char*);
API(void,obs_scene_enum_items,P,bool(*)(P,P,P),P);
API(P,obs_sceneitem_get_source,P);
API(void,obs_queue_task,int,void(*)(P),P,bool);
API(P,obs_frontend_get_current_scene);
API(void,obs_frontend_add_tools_menu_item,const char*,void(*)(P),P);
API(bool,obs_frontend_add_dock_by_id,const char*,const char*,P);
API(void,obs_frontend_remove_dock,const char*);
API(void,obs_frontend_add_event_callback,void(*)(int,P),P);
API(void,blog,int,const char*,...);
API(unsigned,obs_source_get_width,P);
API(unsigned,obs_source_get_height,P);
API(unsigned,obs_source_get_output_flags,P);
API(void,obs_source_video_render,P);
struct CapturedAudio {unsigned char*data[8];unsigned frames;unsigned long long timestamp;};
using AudioCaptureCallback=void(*)(P,P,const CapturedAudio*,bool);
API(void,obs_source_add_audio_capture_callback,P,AudioCaptureCallback,P);
API(void,obs_source_remove_audio_capture_callback,P,AudioCaptureCallback,P);
API(P,obs_get_audio);
API(unsigned,audio_output_get_sample_rate,P);
API(void,obs_enter_graphics);
API(void,obs_leave_graphics);
API(P,gs_texrender_create,int,int);
API(void,gs_texrender_destroy,P);
API(bool,gs_texrender_begin,P,unsigned,unsigned);
API(void,gs_texrender_end,P);
API(P,gs_texrender_get_texture,P);
API(P,gs_stagesurface_create,unsigned,unsigned,int);
API(void,gs_stagesurface_destroy,P);
API(void,gs_stage_texture,P,P);
API(bool,gs_stagesurface_map,P,unsigned char**,unsigned*);
API(void,gs_stagesurface_unmap,P);
API(void,gs_ortho,float,float,float,float,float,float);
API(void,gs_clear,unsigned,const float*,float,unsigned char);
API(void,gs_blend_state_push);
API(void,gs_blend_state_pop);
API(void,gs_reset_blend_state);
static std::atomic<bool> running{false},capturePending{false},sourcePending{false},englishUi{false};
static SOCKET listener=INVALID_SOCKET;
static std::thread serverThread,mediaThread,updateThread;
static std::mutex mutex;
static fs::path root,configPath;
static constexpr const char*pluginVersion="1.3.6";
static constexpr const wchar_t*updateApiPath=L"/repos/xKAPITOSHKAx/Omni-mc/releases/latest";
static constexpr const wchar_t*updatePage=L"https://github.com/xKAPITOSHKAx/Omni-mc/releases/latest";
static std::string settings,media="{\"sessions\":[],\"title\":\"\",\"artist\":\"\",\"playing\":false}",sources="[]",audioSources="[]",systemFontList="[]",cover,background,mediaCommand;
#ifdef OMNI_MC_TEST
static unsigned short port=17844;
#else
static unsigned short port=17843;
#endif
static long long testUntil=0;
// Prefix of the public obs-browser QCef interface. QWidget is owned by OBS.
class QWidget;
class QCefWidget;
struct QCefCookieManager;
struct QCef {
 virtual ~QCef() {}
 virtual bool init_browser()=0;
 virtual bool initialized()=0;
 virtual bool wait_for_browser_init()=0;
 virtual QCefWidget* create_widget(QWidget*,const std::string&,QCefCookieManager*)=0;
};
static QCef* cef=nullptr;
static P settingsWidget=nullptr;
static bool dockRegistered=false;
static const wchar_t*ui(const wchar_t*ru,const wchar_t*en){return englishUi?en:ru;}
static const char*ui(const char*ru,const char*en){return englishUi?en:ru;}
static std::string json(const std::string &s){std::string r="\"";for(unsigned char c:s){if(c=='"'||c=='\\'){r+='\\';r+=c;}else if(c<32){char b[7];sprintf_s(b,"\\u%04x",c);r+=b;}else r+=c;}return r+'"';}
static std::string wideUtf8(const std::wstring&w){if(w.empty())return {};int size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),(int)w.size(),nullptr,0,nullptr,nullptr);if(!size)return {};std::string result((size_t)size,'\0');WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),(int)w.size(),result.data(),size,nullptr,nullptr);return result;}
static std::wstring utf8Wide(const std::string&s){if(s.empty())return {};int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0);if(!size)return {};std::wstring result((size_t)size,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),result.data(),size);return result;}
static bool parseVersion(const std::string&text,int parts[4])
{
 size_t position=!text.empty()&&(text[0]=='v'||text[0]=='V')?1:0;int index=0,value=0;bool digit=false;
 for(;position<text.size();++position){char c=text[position];if(c>='0'&&c<='9'){digit=true;if(value>1000000)return false;value=value*10+(c-'0');}else if(c=='.'&&digit&&index<3){parts[index++]=value;value=0;digit=false;}else if((c=='-'||c=='+')&&digit)break;else return false;}
 if(!digit)return false;parts[index]=value;return true;
}
static bool newerVersion(const std::string&latest)
{
 int available[4]{},installed[4]{};if(!parseVersion(latest,available)||!parseVersion(pluginVersion,installed))return false;
 for(int i=0;i<4;i++){if(available[i]!=installed[i])return available[i]>installed[i];}return false;
}
static std::string jsonField(const std::string&body,const char*name)
{
 std::string key="\""+std::string(name)+"\"";auto position=body.find(key);if(position==std::string::npos)return {};position=body.find(':',position+key.size());if(position==std::string::npos)return {};position=body.find('"',position+1);if(position==std::string::npos)return {};auto end=body.find('"',position+1);return end==std::string::npos?std::string():body.substr(position+1,end-position-1);
}
static std::string latestReleaseVersion()
{
 std::wstring agent=L"Omni-MC/"+utf8Wide(pluginVersion);HINTERNET session=WinHttpOpen(agent.c_str(),WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!session)return {};
 WinHttpSetTimeouts(session,3000,3000,3000,5000);HINTERNET connection=WinHttpConnect(session,L"api.github.com",INTERNET_DEFAULT_HTTPS_PORT,0);HINTERNET request=connection?WinHttpOpenRequest(connection,L"GET",updateApiPath,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE|WINHTTP_FLAG_REFRESH):nullptr;
 std::string body;const wchar_t*headers=L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
 if(request&&WinHttpSendRequest(request,headers,(DWORD)-1L,WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(request,nullptr)){DWORD status=0,length=sizeof(status);if(WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&length,WINHTTP_NO_HEADER_INDEX)&&status==200){for(;;){DWORD available=0;if(!WinHttpQueryDataAvailable(request,&available)||!available||body.size()+available>262144)break;size_t offset=body.size();body.resize(offset+available);DWORD read=0;if(!WinHttpReadData(request,body.data()+offset,available,&read)){body.clear();break;}body.resize(offset+read);if(!read)break;}}}
 if(request)WinHttpCloseHandle(request);if(connection)WinHttpCloseHandle(connection);WinHttpCloseHandle(session);return jsonField(body,"tag_name");
}
static fs::path ignoredUpdatePath(){return configPath.parent_path()/"ignored-update.txt";}
static bool updateIgnored(const std::string&version){std::ifstream file(ignoredUpdatePath(),std::ios::binary);std::string ignored;std::getline(file,ignored);return ignored==version;}
static void ignoreUpdate(const std::string&version){std::error_code error;fs::create_directories(configPath.parent_path(),error);std::ofstream file(ignoredUpdatePath(),std::ios::binary|std::ios::trunc);file.write(version.data(),(std::streamsize)version.size());}
struct UpdateDialogState{HWND checkbox=nullptr;int result=IDNO;bool ignored=false;HBRUSH background=nullptr;HFONT font=nullptr,titleFont=nullptr;};
static LRESULT CALLBACK updateDialogProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam)
{
 auto state=reinterpret_cast<UpdateDialogState*>(GetWindowLongPtrW(window,GWLP_USERDATA));if(message==WM_NCCREATE){state=static_cast<UpdateDialogState*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(state));}
 if(state&&message==WM_ERASEBKGND){RECT area;GetClientRect(window,&area);FillRect((HDC)wParam,&area,state->background);return 1;}
 if(state&&(message==WM_CTLCOLORSTATIC||message==WM_CTLCOLORBTN)){HDC dc=(HDC)wParam;SetTextColor(dc,RGB(235,235,235));SetBkMode(dc,TRANSPARENT);return (LRESULT)state->background;}
 if(state&&message==WM_DRAWITEM){auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lParam);if(item&&(item->CtlID==1001||item->CtlID==1002)){COLORREF fill=item->CtlID==1001?RGB(54,112,180):RGB(58,58,61);if(item->itemState&ODS_SELECTED)fill=RGB(44,86,132);HBRUSH brush=CreateSolidBrush(fill);FillRect(item->hDC,&item->rcItem,brush);DeleteObject(brush);HBRUSH border=CreateSolidBrush(RGB(92,92,96));FrameRect(item->hDC,&item->rcItem,border);DeleteObject(border);SetTextColor(item->hDC,RGB(245,245,245));SetBkMode(item->hDC,TRANSPARENT);wchar_t text[64]{};GetWindowTextW(item->hwndItem,text,(int)std::size(text));DrawTextW(item->hDC,text,-1,&item->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(item->itemState&ODS_FOCUS){RECT focus=item->rcItem;InflateRect(&focus,-4,-4);DrawFocusRect(item->hDC,&focus);}return TRUE;}}
 if(state&&(message==WM_CLOSE||(message==WM_COMMAND&&(LOWORD(wParam)==1001||LOWORD(wParam)==1002)))){state->result=message==WM_COMMAND&&LOWORD(wParam)==1001?IDYES:IDNO;state->ignored=state->checkbox&&SendMessageW(state->checkbox,BM_GETCHECK,0,0)==BST_CHECKED;DestroyWindow(window);return 0;}return DefWindowProcW(window,message,wParam,lParam);
}
static int updateDialog(const std::wstring&content,bool&ignored)
{
 HMODULE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&updateDialog),&module);const wchar_t*className=L"OmniMCUpdateDialog";WNDCLASSW type{};type.lpfnWndProc=updateDialogProc;type.hInstance=module;type.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));type.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);type.lpszClassName=className;if(!RegisterClassW(&type)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return IDCANCEL;
 HWND owner=GetActiveWindow();UpdateDialogState state;state.background=CreateSolidBrush(RGB(30,30,32));state.font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");state.titleFont=CreateFontW(-20,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");int width=520,height=250,x=(GetSystemMetrics(SM_CXSCREEN)-width)/2,y=(GetSystemMetrics(SM_CYSCREEN)-height)/2;HWND window=CreateWindowExW(WS_EX_DLGMODALFRAME,className,ui(L"Omni MC — Обновление",L"Omni MC — Update"),WS_POPUP|WS_CAPTION|WS_SYSMENU,x,y,width,height,owner,nullptr,module,&state);if(!window){DeleteObject(state.background);DeleteObject(state.font);DeleteObject(state.titleFont);return IDCANCEL;}
 auto dwm=LoadLibraryW(L"dwmapi.dll");if(dwm){auto attribute=reinterpret_cast<HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD)>(GetProcAddress(dwm,"DwmSetWindowAttribute"));if(attribute){BOOL dark=TRUE;if(FAILED(attribute(window,20,&dark,sizeof(dark))))attribute(window,19,&dark,sizeof(dark));}FreeLibrary(dwm);}
 auto control=[&](const wchar_t*kind,const wchar_t*text,DWORD style,int cx,int cy,int cw,int ch,int id,HFONT font=nullptr){HWND item=CreateWindowExW(0,kind,text,WS_CHILD|WS_VISIBLE|style,cx,cy,cw,ch,window,(HMENU)(INT_PTR)id,module,nullptr);SendMessageW(item,WM_SETFONT,(WPARAM)(font?font:state.font),TRUE);return item;};
 control(L"STATIC",ui(L"Доступно обновление Omni MC",L"Omni MC update available"),SS_LEFT,28,24,455,30,0,state.titleFont);control(L"STATIC",content.c_str(),SS_LEFT,28,66,455,48,0);state.checkbox=control(L"BUTTON",ui(L"Больше не напоминать об этой версии",L"Don't remind me about this version again"),BS_AUTOCHECKBOX|WS_TABSTOP,28,124,430,28,1003);control(L"BUTTON",ui(L"Открыть страницу",L"Open page"),BS_OWNERDRAW|WS_TABSTOP,250,174,140,34,1001);control(L"BUTTON",ui(L"Позже",L"Later"),BS_OWNERDRAW|WS_TABSTOP,400,174,82,34,1002);
 if(owner)EnableWindow(owner,FALSE);ShowWindow(window,SW_SHOW);SetForegroundWindow(window);MSG message;while(IsWindow(window)){BOOL result=GetMessageW(&message,nullptr,0,0);if(result<=0){if(result==0)PostQuitMessage((int)message.wParam);break;}if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}if(IsWindow(window))DestroyWindow(window);if(owner){EnableWindow(owner,TRUE);SetForegroundWindow(owner);}DeleteObject(state.background);DeleteObject(state.font);DeleteObject(state.titleFont);ignored=state.ignored;return state.result;
}
static void showUpdateNotice(P pointer)
{
 std::unique_ptr<std::string>latest(static_cast<std::string*>(pointer));if(!running||!latest)return;std::wstring content=std::wstring(ui(L"Новая версия: ",L"New version: "))+utf8Wide(*latest)+ui(L"\nТекущая версия: v",L"\nCurrent version: v")+utf8Wide(pluginVersion);
 bool ignored=false;int button=updateDialog(content,ignored);if(ignored)ignoreUpdate(*latest);if(button==IDYES)ShellExecuteW(nullptr,L"open",updatePage,nullptr,nullptr,SW_SHOWNORMAL);
}
static void checkForUpdates()
{
 std::string latest=latestReleaseVersion();if(!latest.empty())blog(200,"[omni-mc] Latest GitHub release: %s; installed: %s",latest.c_str(),pluginVersion);if(running&&newerVersion(latest)&&!updateIgnored(latest))obs_queue_task(0,showUpdateNotice,new std::string(latest),true);
}
struct SessionWindowLookup{std::wstring appId;std::string executable;};
static BOOL CALLBACK findSessionWindow(HWND window,LPARAM parameter){
 auto&lookup=*reinterpret_cast<SessionWindowLookup*>(parameter);IPropertyStore*store=nullptr;PROPVARIANT value;PropVariantInit(&value);
 if(SUCCEEDED(SHGetPropertyStoreForWindow(window,IID_PPV_ARGS(&store)))){if(SUCCEEDED(store->GetValue(PKEY_AppUserModel_ID,&value))&&value.vt==VT_LPWSTR&&value.pwszVal&&CompareStringOrdinal(value.pwszVal,-1,lookup.appId.c_str(),-1,TRUE)==CSTR_EQUAL){DWORD processId=0;GetWindowThreadProcessId(window,&processId);HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,processId);if(process){wchar_t path[32768];DWORD length=(DWORD)std::size(path);if(QueryFullProcessImageNameW(process,0,path,&length)){const wchar_t*name=path;for(DWORD i=0;i<length;i++)if(path[i]=='\\'||path[i]=='/')name=path+i+1;lookup.executable=wideUtf8(std::wstring(name,(size_t)((path+length)-name)));}CloseHandle(process);}}PropVariantClear(&value);store->Release();}
 return lookup.executable.empty()?TRUE:FALSE;
}
static std::string sessionExecutable(const std::string&appId){int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,appId.data(),(int)appId.size(),nullptr,0);if(!size)return {};std::wstring wide((size_t)size,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,appId.data(),(int)appId.size(),wide.data(),size);SessionWindowLookup lookup{wide,{}};EnumWindows(findSessionWindow,reinterpret_cast<LPARAM>(&lookup));return lookup.executable;}
static std::string pathUtf8(const fs::path&p){return wideUtf8(p.native());}
static int CALLBACK collectFont(const LOGFONTW*font,const TEXTMETRICW*,DWORD,LPARAM value){if(font->lfFaceName[0]&&font->lfFaceName[0]!=L'@')reinterpret_cast<std::vector<std::wstring>*>(value)->emplace_back(font->lfFaceName);return 1;}
static std::string windowsFonts(){std::vector<std::wstring> names;HDC dc=GetDC(nullptr);if(dc){LOGFONTW query{};query.lfCharSet=DEFAULT_CHARSET;EnumFontFamiliesExW(dc,&query,(FONTENUMPROCW)collectFont,(LPARAM)&names,0);ReleaseDC(nullptr,dc);}std::sort(names.begin(),names.end(),[](const auto&a,const auto&b){return _wcsicmp(a.c_str(),b.c_str())<0;});names.erase(std::unique(names.begin(),names.end(),[](const auto&a,const auto&b){return _wcsicmp(a.c_str(),b.c_str())==0;}),names.end());std::string result="[";for(const auto&name:names){if(result.size()>1)result+=',';result+=json(wideUtf8(name));}return result+"]";}
struct LegacyTimeline {double position=0,duration=0;bool available=false;};
static DWORD legacyYandexPid(){DWORD result=0;HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot==INVALID_HANDLE_VALUE)return 0;PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);if(Process32FirstW(snapshot,&entry))do{if(!_wcsicmp(entry.szExeFile,L"Y.Music.exe")){result=entry.th32ProcessID;break;}}while(Process32NextW(snapshot,&entry));CloseHandle(snapshot);return result;}
static double mediaClock(const wchar_t*text){if(!text||!*text)return -1;double value=0,part=0;bool digit=false;for(const wchar_t*p=text;;++p){if(*p>=L'0'&&*p<=L'9'){part=part*10+(*p-L'0');digit=true;}else if(*p==L':'&&digit){value=value*60+part;part=0;digit=false;}else if(!*p&&digit)return value*60+part;else return -1;}}
static LegacyTimeline legacyYandexTimeline(){LegacyTimeline result;DWORD pid=legacyYandexPid();if(!pid)return result;com_ptr<IUIAutomation> automation;if(FAILED(CoCreateInstance(CLSID_CUIAutomation,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(automation.put()))))return result;com_ptr<IUIAutomationElement> root;if(FAILED(automation->GetRootElement(root.put())))return result;VARIANT processValue{},idValue{};processValue.vt=VT_I4;processValue.lVal=(LONG)pid;idValue.vt=VT_BSTR;idValue.bstrVal=SysAllocString(L"Slider");com_ptr<IUIAutomationCondition> processCondition,idCondition,condition;HRESULT hr=automation->CreatePropertyCondition(UIA_ProcessIdPropertyId,processValue,processCondition.put());if(SUCCEEDED(hr))hr=automation->CreatePropertyCondition(UIA_AutomationIdPropertyId,idValue,idCondition.put());if(SUCCEEDED(hr))hr=automation->CreateAndCondition(processCondition.get(),idCondition.get(),condition.put());VariantClear(&idValue);if(FAILED(hr))return result;com_ptr<IUIAutomationElement> slider;if(FAILED(root->FindFirst(TreeScope_Descendants,condition.get(),slider.put()))||!slider)return result;double minimum=0,maximum=0,current=0;com_ptr<IUIAutomationRangeValuePattern> range;if(SUCCEEDED(slider->GetCurrentPatternAs(UIA_RangeValuePatternId,IID_PPV_ARGS(range.put())))&&range){range->get_CurrentMinimum(&minimum);range->get_CurrentMaximum(&maximum);range->get_CurrentValue(&current);}com_ptr<IUIAutomationTreeWalker> walker;com_ptr<IUIAutomationElement> parent;if(FAILED(automation->get_RawViewWalker(walker.put()))||FAILED(walker->GetParentElement(slider.get(),parent.put()))||!parent)return result;com_ptr<IUIAutomationElement> child;if(FAILED(walker->GetFirstChildElement(parent.get(),child.put())))return result;bool afterCurrent=false;double displayedPosition=-1;while(child){CONTROLTYPEID type=0;BSTR automationId=nullptr,name=nullptr;child->get_CurrentControlType(&type);child->get_CurrentAutomationId(&automationId);bool isCurrent=automationId&&!wcscmp(automationId,L"Time");if(isCurrent){child->get_CurrentName(&name);displayedPosition=mediaClock(name);afterCurrent=true;}else if(afterCurrent&&type==UIA_TextControlTypeId){child->get_CurrentName(&name);result.duration=mediaClock(name);if(name)SysFreeString(name);if(automationId)SysFreeString(automationId);break;}if(name)SysFreeString(name);if(automationId)SysFreeString(automationId);com_ptr<IUIAutomationElement> next;if(FAILED(walker->GetNextSiblingElement(child.get(),next.put())))break;child=std::move(next);}if(result.duration>0){result.position=maximum>minimum?result.duration*std::clamp((current-minimum)/(maximum-minimum),0.0,1.0):displayedPosition;result.available=result.position>=0;}return result;}
static std::string read(const fs::path&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static std::string webResource(int id){HMODULE module=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&running),&module))return {};HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));if(!resource)return {};HGLOBAL loaded=LoadResource(module,resource);DWORD size=SizeofResource(module,resource);const char*bytes=loaded?static_cast<const char*>(LockResource(loaded)):nullptr;return bytes&&size?std::string(bytes,size):std::string();}
static long long now(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
static std::string base(){return "http://127.0.0.1:"+std::to_string(port);}
static void skinSize(const std::string&skin,int&w,int&h){w=720;h=240;if(skin=="compact"){w=720;h=200;}else if(skin=="compact-bar"){w=768;h=90;}else if(skin=="cover"){w=210;h=214;}else if(skin=="vinyl"){w=360;h=380;}else if(skin=="glass"){w=360;h=420;}else if(skin=="poster"){w=360;h=360;}else if(skin=="cinematic"){w=640;h=360;}else if(skin=="split"){w=360;h=460;}else if(skin=="orbit"){w=750;h=280;}else if(skin=="cassette"){w=680;h=360;}else if(skin=="prism"){w=600;h=220;}else if(skin=="eclipse"){w=764;h=278;}else if(skin=="neon"){w=714;h=249;}else if(skin=="vector"){w=680;h=230;}else if(skin=="pulse"){w=700;h=270;}else if(skin=="rhythm"){w=640;h=220;}else if(skin=="cosmos"||skin=="gargantua"){w=720;h=280;}}
#include "profiles.hpp"
#include "backdrop.hpp"
#include "audio-volume.hpp"
static std::atomic<unsigned> visualizerSampleRate{48000};
static void captureVisualizer(P pointer,P,const CapturedAudio*audio,bool muted)
{
 auto*profile=static_cast<Profile*>(pointer);if(!profile||!audio||muted||audio->frames<32){if(profile)for(auto&band:profile->visualizerBands)band.store(0,std::memory_order_relaxed);return;}
 std::array<const float*,8> channels{};size_t channelCount=0;for(auto*plane:audio->data)if(plane)channels[channelCount++]=reinterpret_cast<const float*>(plane);if(!channelCount){for(auto&band:profile->visualizerBands)band.store(0,std::memory_order_relaxed);return;}
 unsigned count=std::min(audio->frames,1024u),offset=audio->frames-count,rate=std::max(8000u,visualizerSampleRate.load(std::memory_order_relaxed));constexpr double pi=3.14159265358979323846;
 for(size_t bandIndex=0;bandIndex<profile->visualizerBands.size();++bandIndex){double center=65.0*std::pow(12000.0/65.0,(double)bandIndex/(profile->visualizerBands.size()-1)),magnitude=0;
  for(double factor:{.84,1.0,1.19}){double frequency=std::min(center*factor,rate*.46),omega=2*pi*frequency/rate,coefficient=2*std::cos(omega);
   for(size_t channel=0;channel<channelCount;++channel){double s0=0,s1=0,s2=0;const float*samples=channels[channel];
    for(unsigned i=0;i<count;++i){double window=.5-.5*std::cos(2*pi*i/(count-1)),sample=std::clamp((double)samples[offset+i],-1.0,1.0)*window;s0=sample+coefficient*s1-s2;s2=s1;s1=s0;}
    magnitude=std::max(magnitude,2*std::sqrt(std::max(0.0,s1*s1+s2*s2-coefficient*s1*s2))/count);
   }
  }
  double db=20*std::log10(std::max(magnitude,1e-7)),value=std::pow(std::clamp((db+68.0)/56.0,0.0,1.0),.72);float previous=profile->visualizerBands[bandIndex].load(std::memory_order_relaxed),next=(float)(value>previous?previous*.28+value*.72:previous*.76+value*.24);profile->visualizerBands[bandIndex].store(next,std::memory_order_relaxed);
 }
 profile->visualizerUpdated.store(now(),std::memory_order_relaxed);
}
static void syncProfileVisualizer(P pointer)
{
 std::unique_ptr<std::string> id(static_cast<std::string*>(pointer));std::shared_ptr<Profile> profile;std::string wanted;P previous=nullptr;
 {std::lock_guard lock(mutex);profile=profileLocked(*id);if(!profile)return;P data=obs_data_create_from_json(profile->cfg.c_str());wanted=obs_data_get_string(data,"rhythmAudioSource");obs_data_release(data);if(profile->visualizerSource&&profile->visualizerSourceName==wanted)return;previous=profile->visualizerSource;profile->visualizerSource=nullptr;profile->visualizerSourceName.clear();}
 if(previous){obs_source_remove_audio_capture_callback(previous,captureVisualizer,profile.get());obs_source_release(previous);}for(auto&band:profile->visualizerBands)band.store(0,std::memory_order_relaxed);profile->visualizerUpdated.store(0,std::memory_order_relaxed);
 if(wanted.empty())return;P source=obs_get_source_by_name(wanted.c_str());if(!source)return;if(!(obs_source_get_output_flags(source)&2)){obs_source_release(source);return;}obs_source_add_audio_capture_callback(source,captureVisualizer,profile.get());{std::lock_guard lock(mutex);profile->visualizerSource=source;profile->visualizerSourceName=wanted;}
}
static void detachVisualizers()
{
 std::vector<std::shared_ptr<Profile>> work;{std::lock_guard lock(mutex);for(auto&entry:profiles)work.push_back(entry.second);}
 for(auto&profile:work){P source=nullptr;{std::lock_guard lock(mutex);source=profile->visualizerSource;profile->visualizerSource=nullptr;profile->visualizerSourceName.clear();}if(source){obs_source_remove_audio_capture_callback(source,captureVisualizer,profile.get());obs_source_release(source);}}
}
static std::string visualizerJson(const Profile&profile)
{
 bool available=profile.visualizerSource&&now()-profile.visualizerUpdated.load(std::memory_order_relaxed)<600;std::string result="{\"available\":"+std::string(available?"true":"false")+",\"bands\":[";for(size_t i=0;i<profile.visualizerBands.size();++i){if(i)result+=',';float value=available?profile.visualizerBands[i].load(std::memory_order_relaxed):0;result+=std::to_string(std::clamp(value,0.f,1.f));}return result+"]}";
}
static bool listSource(P out,P source)
{
 std::string name=obs_source_get_name(source),id=obs_source_get_id(source);
 if(name.rfind("Omni MC",0)==0&&id=="browser_source")return true;
 if(id=="scene"||id=="group"||(obs_source_get_output_flags(source)&1))
  static_cast<std::vector<std::string>*>(out)->push_back(name);
 return true;
}
static bool listAudioSource(P out,P source)
{
 std::string name=obs_source_get_name(source);if(name.rfind("Omni MC",0)!=0&&(obs_source_get_output_flags(source)&2))static_cast<std::vector<std::string>*>(out)->push_back(name);return true;
}
static void refreshSources(P)
{
 if(!running){sourcePending=false;return;}
 std::vector<std::string> names,audioNames;
 obs_enum_scenes(listSource,&names);
 obs_enum_sources(listSource,&names);
 obs_enum_sources(listAudioSource,&audioNames);
 std::sort(names.begin(),names.end());
 names.erase(std::unique(names.begin(),names.end()),names.end());
 std::sort(audioNames.begin(),audioNames.end());audioNames.erase(std::unique(audioNames.begin(),audioNames.end()),audioNames.end());
 std::string r="[";
 for(auto&name:names){if(r.size()>1)r+=',';r+=json(name);}
 r+=']';
 std::string audio="[";for(auto&name:audioNames){if(audio.size()>1)audio+=',';audio+=json(name);}audio+=']';
 {std::lock_guard lock(mutex);sources=r;audioSources=audio;}
 attachBackdrop();
 sourcePending=false;
}
struct FindPlayer {std::string url,name;P source=nullptr;};
static bool findExistingPlayer(P pointer,P source){auto&f=*static_cast<FindPlayer*>(pointer);if(std::string(obs_source_get_id(source))!="browser_source")return true;P data=obs_source_get_settings(source);std::string url=obs_data_get_string(data,"url");obs_data_release(data);if(url==f.url||url.rfind(f.url+"&",0)==0){obs_source_set_name(source,f.name.c_str());f.source=obs_get_source_by_name(f.name.c_str());return false;}return true;}
static void removeObsoleteBackdrop(P source,P filter,P){std::string name=obs_source_get_name(filter),id=obs_source_get_id(filter);if(name.find("Фон (Composite Blur)")!=std::string::npos&&id!="omni_mc_backdrop")obs_source_filter_remove(source,filter);}
static void createScenePlayer(P sceneSource,bool add){
 auto profile=ensureProfile(sceneSource);P scene=obs_scene_from_source(sceneSource);std::string name=playerName(profile->id);
 P legacy=obs_scene_find_source(scene,"Omni MC \u2014 \u041f\u043b\u0435\u0435\u0440");P source=obs_get_source_by_name(name.c_str());
 if(!source){FindPlayer found{base()+"/overlay?context="+profile->id+"&padding=50",name};obs_enum_sources(findExistingPlayer,&found);source=found.source;if(source)obs_source_enum_filters(source,removeObsoleteBackdrop,nullptr);}
 if(!source&&(add||legacy)){P original=legacy?obs_source_get_settings(obs_sceneitem_get_source(legacy)):nullptr;P d=original?obs_data_create_from_json(obs_data_get_json(original)):obs_data_create();if(original)obs_data_release(original);int w,h;profileDimensions(profile,w,h);obs_data_set_int(d,"width",w);obs_data_set_int(d,"height",h);obs_data_set_string(d,"omni_mc_context",profile->id.c_str());obs_data_set_string(d,"url",(base()+"/overlay?context="+profile->id+"&padding=50").c_str());obs_data_set_bool(d,"shutdown",false);obs_data_set_bool(d,"restart_when_active",false);source=obs_source_create("browser_source",name.c_str(),d,nullptr);obs_data_release(d);}
 if(source){P current=obs_source_get_settings(source);std::string linked=obs_data_get_string(current,"omni_mc_context");int w,h;profileDimensions(profile,w,h);bool repair=linked!=profile->id||obs_data_get_int(current,"width")!=w||obs_data_get_int(current,"height")!=h;obs_data_release(current);if(repair){refreshProfilePlayer(new std::string(profile->id));blog(200,"[omni-mc] Repaired scene binding: %s",profile->id.c_str());}P item=obs_scene_find_source(scene,name.c_str());if(!item&&(add||legacy))item=obs_scene_add(scene,source);
  if(legacy&&item){TransformInfo info{};CropInfo crop{};obs_sceneitem_get_info2(legacy,&info);obs_sceneitem_get_crop(legacy,&crop);int order=obs_sceneitem_get_order_position(legacy);bool visible=obs_sceneitem_visible(legacy);obs_sceneitem_set_info2(item,&info);obs_sceneitem_set_crop(item,&crop);obs_sceneitem_set_visible(item,visible);obs_sceneitem_remove(legacy);obs_sceneitem_set_order_position(item,order);}
  attachBackdropSource(source);syncShadowSource(source,profile);obs_source_release(source);saveProfile(profile);
 }
}
static bool prepareScene(P,P scene){createScenePlayer(scene,false);auto profile=ensureProfile(scene);syncProfileVisualizer(new std::string(profile->id));return true;}
static void syncScenes(P){if(!running)return;obs_enum_scenes(prepareScene,nullptr);P scene=obs_frontend_get_current_scene();if(scene){auto p=ensureProfile(scene);{std::lock_guard lock(mutex);activeProfile=p->id;}obs_source_release(scene);}}
static void addPlayer(P pointer){std::unique_ptr<std::string> id(static_cast<std::string*>(pointer));if(!running)return;P scene=id&&!id->empty()?obs_get_source_by_uuid(id->c_str()):obs_frontend_get_current_scene();if(scene){createScenePlayer(scene,true);obs_source_release(scene);}}
static void mediaLoop(){init_apartment(apartment_type::multi_threaded);try{auto manager=GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();std::map<std::string,std::pair<std::string,std::string>> coverCache;std::map<std::string,std::pair<std::string,unsigned>> coverRetryDone;std::map<std::string,std::string> sessionExecutableCache;auto loadCover=[](const auto&props){std::string art;try{auto thumb=props.Thumbnail();if(thumb){auto stream=thumb.OpenReadAsync().get();if(stream.Size()<8*1024*1024){DataReader reader(stream);auto count=reader.LoadAsync((unsigned)stream.Size()).get();std::vector<unsigned char> bytes(count);reader.ReadBytes(bytes);art.assign((char*)bytes.data(),bytes.size());}}}catch(const hresult_error&e){blog(300,"[omni-mc] Track cover: %s",to_string(e.message()).c_str());}return art;};
while(running){
 decltype(manager.GetSessions()) all{nullptr};GlobalSystemMediaTransportControlsSession currentSession{nullptr};try{all=manager.GetSessions();currentSession=manager.GetCurrentSession();}catch(const hresult_error&e){blog(300,"[omni-mc] Media sessions: %s",to_string(e.message()).c_str());for(int i=0;i<5&&running;i++)Sleep(100);continue;}
 std::map<std::string,GlobalSystemMediaTransportControlsSessionPlaybackInfo> playbackCache;
 struct SessionRead {GlobalSystemMediaTransportControlsSessionMediaProperties props{nullptr};GlobalSystemMediaTransportControlsSessionTimelineProperties timeline{nullptr};};
 std::map<std::string,SessionRead> sessionCache;std::map<std::string,ApplicationVolume> audioCache;LegacyTimeline legacyTimeline;bool legacyTimelineRead=false;
 auto playback=[&](const GlobalSystemMediaTransportControlsSession&session){auto id=to_string(session.SourceAppUserModelId());auto found=playbackCache.find(id);if(found==playbackCache.end())found=playbackCache.emplace(id,session.GetPlaybackInfo()).first;return found->second;};
  std::string list="[",sessionApps="{";for(auto session:all){auto id=to_string(session.SourceAppUserModelId());if(list.size()>1)list+=',';list+=json(id);auto found=sessionExecutableCache.find(id);if(found==sessionExecutableCache.end()){auto executable=sessionExecutable(id);if(!executable.empty())found=sessionExecutableCache.emplace(id,executable).first;}if(found!=sessionExecutableCache.end()){if(sessionApps.size()>1)sessionApps+=',';sessionApps+=json(id)+":"+json(found->second);}}list+=']';sessionApps+='}';
 for(auto i=coverCache.begin();i!=coverCache.end();){bool present=false;for(auto session:all)if(to_string(session.SourceAppUserModelId())==i->first){present=true;break;}if(!present)i=coverCache.erase(i);else ++i;}
 for(auto i=applicationAudioCache.begin();i!=applicationAudioCache.end();){bool present=false;for(auto session:all)if(to_string(session.SourceAppUserModelId())==i->first){present=true;break;}if(!present)i=applicationAudioCache.erase(i);else ++i;}
 std::vector<std::shared_ptr<Profile>> work;{std::lock_guard lock(mutex);for(auto&entry:profiles)work.push_back(entry.second);}for(auto&profile:work){if(!running)break;std::string&lastKey=profile->lastKey;try{std::string cfg,cmd,seekKey,audioSession;long long seekMs;int volume,mute;{std::lock_guard lock(mutex);cfg=profile->cfg;cmd=std::exchange(profile->command,"");seekMs=std::exchange(profile->seekMs,-1);seekKey=profile->seekKey;volume=std::exchange(profile->audioVolume,-1);mute=std::exchange(profile->audioMute,-1);audioSession=profile->audioSession;}P d=obs_data_create_from_json(cfg.c_str());std::string wanted=obs_data_get_string(d,"session");int showSeconds=(int)std::clamp<long long>(obs_data_get_int(d,"showSeconds")?obs_data_get_int(d,"showSeconds"):5,1,30);obs_data_release(d);auto selected=currentSession;if(!wanted.empty())selected=nullptr;
for(auto session:all)if(!wanted.empty()&&to_string(session.SourceAppUserModelId())==wanted){selected=session;break;}
if(wanted.empty()){for(auto session:all)if(playback(session).PlaybackStatus()==GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing){selected=session;break;}}
 std::string state="{\"sessions\":"+list+",\"sessionApps\":"+sessionApps;std::string art;if(selected){if(cmd=="toggle")selected.TryTogglePlayPauseAsync().get();else if(cmd=="next")selected.TrySkipNextAsync().get();else if(cmd=="previous")selected.TrySkipPreviousAsync().get();std::string selectedId=to_string(selected.SourceAppUserModelId());if(!cmd.empty()){sessionCache.erase(selectedId);playbackCache.erase(selectedId);}
auto found=sessionCache.find(selectedId);if(found==sessionCache.end())found=sessionCache.emplace(selectedId,SessionRead{selected.TryGetMediaPropertiesAsync().get(),selected.GetTimelineProperties()}).first;
auto props=found->second.props;auto timeline=found->second.timeline;bool playing=playback(selected).PlaybackStatus()==GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;std::string id=to_string(selected.SourceAppUserModelId()),title=to_string(props.Title()),artist=to_string(props.Artist());std::string key=id+"|"+title+"|"+artist;auto audioFound=audioCache.find(id);if(audioFound==audioCache.end()||(audioSession==id&&(volume>=0||mute>=0))){auto audio=applicationVolume(id,audioSession==id?volume:-1,audioSession==id?mute:-1);audioFound=audioCache.insert_or_assign(id,audio).first;}auto audio=audioFound->second;state+=",\"volumeAvailable\":"+std::string(audio.available?"true":"false")+",\"volume\":"+std::to_string((int)std::lround(audio.volume*100))+",\"muted\":"+(audio.muted?"true":"false");if(seekMs>=0&&key==seekKey){auto start=timeline.StartTime().count(),end=timeline.EndTime().count();auto target=std::clamp<long long>(start+seekMs*10000,start,std::max(start,end));if(selected.TryChangePlaybackPositionAsync(target).get()){timeline=selected.GetTimelineProperties();found->second.timeline=timeline;}}bool trackChanged=key!=lastKey,retryCover=!trackChanged&&profile->coverRetryPending&&now()>=profile->coverRetryAt;if(trackChanged||retryCover){if(retryCover){unsigned attempt=profile->coverRetryAttempt;profile->coverRetryPending=false;profile->coverRetryAt=0;auto retried=coverRetryDone.find(id);if(retried!=coverRetryDone.end()&&retried->second.first==key&&retried->second.second>=attempt){auto cachedCover=coverCache.find(id);if(cachedCover!=coverCache.end()&&cachedCover->second.first==key)art=cachedCover->second.second;}else{coverRetryDone[id]={key,attempt};art=loadCover(props);coverCache[id]={key,art};}if(art.empty()&&attempt<3){profile->coverRetryAttempt=attempt+1;profile->coverRetryPending=true;profile->coverRetryAt=now()+2000;}else profile->coverRetryAttempt=0;}else{auto cachedCover=coverCache.find(id);if(cachedCover!=coverCache.end()&&cachedCover->second.first==key)art=cachedCover->second.second;else{coverRetryDone[id]={key,0};art=loadCover(props);coverCache[id]={key,art};}profile->coverRetryAttempt=art.empty()?1:0;profile->coverRetryPending=art.empty();profile->coverRetryAt=profile->coverRetryPending?now()+1000:0;}lastKey=key;{std::lock_guard lock(mutex);profile->cover=art;++profile->coverRevision;if(trackChanged)profile->overlayUntil=now()+std::max(1,showSeconds)*1000;}}
 double duration=std::chrono::duration<double>(timeline.EndTime()-timeline.StartTime()).count();
 double position=std::chrono::duration<double>(timeline.Position()-timeline.StartTime()).count();bool legacyFallback=false;
 if(id=="A025C540.Yandex.Music_vfvw9svesycw6!App"&&duration<=0){if(!legacyTimelineRead){legacyTimeline=legacyYandexTimeline();legacyTimelineRead=true;}if(legacyTimeline.available){duration=legacyTimeline.duration;position=legacyTimeline.position;legacyFallback=true;}}
 if(playing&&!legacyFallback){auto rate=playback(selected).PlaybackRate();double speed=rate?rate.Value():1.0;position+=std::max(0.0,std::chrono::duration<double>(clock::now()-timeline.LastUpdatedTime()).count())*speed;}
auto playbackRate=playback(selected).PlaybackRate();state+=",\"rate\":"+std::to_string(playbackRate?playbackRate.Value():1.0);state+=",\"session\":"+json(id)+",\"title\":"+json(title)+",\"artist\":"+json(artist)+",\"album\":"+json(to_string(props.AlbumTitle()))+",\"playing\":"+(playing?"true":"false")+",\"duration\":"+std::to_string(std::max(0.0,duration))+",\"position\":"+std::to_string(std::clamp(position,0.0,std::max(0.0,duration)))+",\"coverKey\":"+json(key);}else{lastKey.clear();profile->coverRetryAttempt=0;profile->coverRetryPending=false;profile->coverRetryAt=0;state+=",\"title\":\"\",\"artist\":\"\",\"playing\":false,\"duration\":0,\"position\":0,\"coverKey\":\"\"";std::lock_guard lock(mutex);if(!profile->cover.empty()){profile->cover.clear();++profile->coverRevision;}profile->overlayUntil=0;}unsigned coverVersion;{std::lock_guard lock(mutex);coverVersion=profile->coverRevision;}state+=",\"coverRevision\":"+std::to_string(coverVersion)+",\"updated\":"+std::to_string(now())+"}";{std::lock_guard lock(mutex);profile->media=state;}}catch(const hresult_error&e){blog(300,"[omni-mc] Media session: %s",to_string(e.message()).c_str());std::lock_guard lock(mutex);profile->media="{\"sessions\":[],\"playing\":false,\"error\":"+json(to_string(e.message()))+"}";}}for(int i=0;i<3&&running;i++)Sleep(100);}}catch(const hresult_error&e){std::lock_guard lock(mutex);media="{\"sessions\":[],\"error\":"+json(to_string(e.message()))+"}";}clearApplicationVolumeCache();uninit_apartment();}
static void reply(SOCKET c,const std::string&body,const char*type,int status=200){std::string head="HTTP/1.1 "+std::to_string(status)+(status==200?" OK":" Error")+"\r\nContent-Type: "+type+"\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n";std::string bytes=head+body;size_t sent=0;while(sent<bytes.size()){int n=send(c,bytes.data()+sent,(int)std::min<size_t>(65536,bytes.size()-sent),0);if(n<=0)break;sent+=n;}}
static void serve(){while(running){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval tv{0,200000};if(select(0,&set,nullptr,nullptr,&tv)<=0)continue;SOCKET c=accept(listener,nullptr,nullptr);if(c==INVALID_SOCKET)continue;DWORD timeout=2000;setsockopt(c,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));setsockopt(c,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(timeout));std::string req;char b[4096];size_t end=std::string::npos,total=0;while(req.size()<65536){int n=recv(c,b,sizeof(b),0);if(n<=0)break;req.append(b,n);end=req.find("\r\n\r\n");if(end!=std::string::npos){std::string header=req.substr(0,end);std::transform(header.begin(),header.end(),header.begin(),[](unsigned char c){return (char)std::tolower(c);});auto p=header.find("content-length:");if(p!=std::string::npos)total=strtoul(header.c_str()+p+15,nullptr,10);if(req.size()>=end+4+total)break;}}std::istringstream line(req);std::string method,path,proto;line>>method>>path>>proto;std::string context;auto query=path.find('?');if(query!=std::string::npos){auto at=path.find("context=",query);if(at!=std::string::npos){at+=8;context=path.substr(at,path.find('&',at)-at);}}path=path.substr(0,query);std::string body=end==std::string::npos?"":req.substr(end+4,total);
// Reject cross-origin writes and DNS-rebinding requests.
std::string headers=req.substr(0,end+2);std::transform(headers.begin(),headers.end(),headers.begin(),[](unsigned char c){return (char)std::tolower(c);});std::string host="host: 127.0.0.1:"+std::to_string(port);bool trusted=headers.find(host+"\r\n")!=std::string::npos;auto op=headers.find("origin:");if(op!=std::string::npos && headers.find("origin: "+base()+"\r\n")==std::string::npos)trusted=false;
if(!trusted)reply(c,"{}","application/json",403);
else if(method=="GET"&&(path=="/"||path=="/overlay")){
#ifdef OMNI_MC_TEST
if(path=="/")blog(200,"[omni-mc] Test: CEF settings page requested");
#endif
 int id=path=="/"?IDR_SETTINGS_HTML:IDR_OVERLAY_HTML;std::string page=webResource(id);const std::string languageMarker="{{LANG}}";auto languageAt=page.find(languageMarker);if(languageAt!=std::string::npos)page.replace(languageAt,languageMarker.size(),englishUi?"en":"ru");if(path=="/"){const std::string marker="{{VERSION}}";auto at=page.find(marker);if(at!=std::string::npos)page.replace(at,marker.size(),pluginVersion);}reply(c,page,"text/html; charset=utf-8",page.empty()?500:200);}
else if(method=="GET"&&(path=="/app.js"||path=="/overlay.js"||path=="/timeline.js"||path=="/i18n.js")){int id=path=="/app.js"?IDR_APP_JS:path=="/overlay.js"?IDR_OVERLAY_JS:path=="/timeline.js"?IDR_TIMELINE_JS:IDR_I18N_JS;std::string script=webResource(id);reply(c,script,"text/javascript; charset=utf-8",script.empty()?500:200);}
else if(method=="GET"&&(path=="/style.css"||path=="/skins.css")){int id=path=="/style.css"?IDR_STYLE_CSS:IDR_SKINS_CSS;std::string style=webResource(id);reply(c,style,"text/css; charset=utf-8",style.empty()?500:200);}
else if(method=="GET"&&path=="/state"){std::string r;{std::lock_guard lock(mutex);auto p=profileLocked(context);if(p){idleVisibilityLocked(*p);r="{\"context\":"+json(p->id)+",\"idleFade\":{\"from\":"+std::to_string(p->fadeFrom)+",\"to\":"+std::to_string(p->fadeTo)+",\"at\":"+std::to_string(p->fadeAt)+"},\"media\":"+p->media+",\"settings\":"+p->cfg+",\"testUntil\":"+std::to_string(p->testUntil)+",\"overlayUntil\":"+std::to_string(p->overlayUntil)+"}";}else r="{}";}reply(c,r,"application/json",r=="{}"?503:200);}
else if(method=="GET"&&path=="/sources"){if(!sourcePending.exchange(true))obs_queue_task(0,refreshSources,nullptr,false);std::string r;{std::lock_guard lock(mutex);r=sources;}reply(c,r,"application/json");}
else if(method=="GET"&&path=="/audio-sources"){if(!sourcePending.exchange(true))obs_queue_task(0,refreshSources,nullptr,false);std::string r;{std::lock_guard lock(mutex);r=audioSources;}reply(c,r,"application/json");}
else if(method=="GET"&&path=="/visualizer"){std::string r;{std::lock_guard lock(mutex);auto p=profileLocked(context);r=p?visualizerJson(*p):"{}";}reply(c,r,"application/json",r=="{}"?503:200);}
else if(method=="GET"&&path=="/fonts"){reply(c,systemFontList,"application/json");}
else if(method=="GET"&&path=="/cover"){std::string r;{std::lock_guard lock(mutex);auto p=profileLocked(context);if(p)r=p->cover;}reply(c,r,"application/octet-stream");}
else if(method=="POST"&&path=="/shadow-color"){P data=obs_data_create_from_json(body.c_str());std::string color=data?obs_data_get_string(data,"color"):"";if(data)obs_data_release(data);std::string id;bool valid=validRgbColor(color);if(valid){std::lock_guard lock(mutex);auto p=profileLocked(context);if(p){p->shadowCoverColor=color;id=p->id;}else valid=false;}if(valid){obs_queue_task(0,syncProfileShadow,new std::string(id),false);reply(c,"{}","application/json");}else reply(c,"{}","application/json",400);}
else if(method=="POST"&&path=="/settings"){P d=obs_data_create_from_json(body.c_str());if(d){std::string normalized=obs_data_get_json(d);obs_data_release(d);std::shared_ptr<Profile> p;bool changed=false,resized=false,visualizerChanged=false;{std::lock_guard lock(mutex);p=profileLocked(context);if(p){P previous=obs_data_create_from_json(p->cfg.c_str()),next=obs_data_create_from_json(normalized.c_str());changed=std::string(obs_data_get_string(previous,"skin"))!=obs_data_get_string(next,"skin");visualizerChanged=std::string(obs_data_get_string(previous,"rhythmAudioSource"))!=obs_data_get_string(next,"rhythmAudioSource");for(const char*key:{"adaptiveSize","progressVisible","showTrackCover"}){bool a=!obs_data_has_user_value(previous,key)||obs_data_get_bool(previous,key),b=!obs_data_has_user_value(next,key)||obs_data_get_bool(next,key);changed=changed||a!=b;}for(const char*key:{"widgetWidthScale","widgetHeightScale","coverScale"}){auto a=obs_data_get_int(previous,key),b=obs_data_get_int(next,key);resized=resized||(a?a:100)!=(b?b:100);}obs_data_release(previous);obs_data_release(next);p->cfg=normalized;if(changed){p->layoutWidth=0;p->layoutHeight=0;}}}if(p){saveProfile(p);obs_queue_task(0,syncProfileShadow,new std::string(p->id),false);if(visualizerChanged)obs_queue_task(0,syncProfileVisualizer,new std::string(p->id),false);if(changed)obs_queue_task(0,refreshProfilePlayer,new std::string(p->id),false);else if(resized)obs_queue_task(0,applyMeasuredLayout,new std::string(p->id),false);reply(c,"{\"ok\":true}","application/json");}else reply(c,"{}","application/json",404);}else reply(c,"{}","application/json",400);}
else if(method=="POST"&&path=="/add"){std::string id;{std::lock_guard lock(mutex);auto p=profileLocked(context);if(p)id=p->id;}obs_queue_task(0,addPlayer,new std::string(id),false);reply(c,"{\"ok\":true}","application/json");}
else if(method=="POST"&&path=="/test"){std::lock_guard lock(mutex);auto p=profileLocked(context);if(p)p->testUntil=now()+5000;reply(c,"{}","application/json");}
else if(method=="POST"&&path=="/layout"){P data=obs_data_create_from_json(body.c_str());bool changed=false;std::string id;if(data){int w=(int)obs_data_get_int(data,"width"),h=(int)obs_data_get_int(data,"height");std::string skin=obs_data_get_string(data,"skin");obs_data_release(data);std::lock_guard lock(mutex);auto p=profileLocked(context);if(p&&w>=200&&w<=1000&&h>=100&&h<=800){P cfg=obs_data_create_from_json(p->cfg.c_str());bool adapt=!obs_data_has_user_value(cfg,"adaptiveSize")||obs_data_get_bool(cfg,"adaptiveSize");bool coverHidden=skin!="cover"&&skin!="vinyl"&&skin!="orbit"&&skin!="prism"&&skin!="eclipse"&&obs_data_has_user_value(cfg,"showTrackCover")&&!obs_data_get_bool(cfg,"showTrackCover");if(coverHidden&&skin==obs_data_get_string(cfg,"skin")&&(p->layoutWidth!=w||p->layoutHeight!=h)){p->layoutWidth=w;p->layoutHeight=h;changed=true;id=p->id;}obs_data_release(cfg);}}if(changed)obs_queue_task(0,applyMeasuredLayout,new std::string(id),false);reply(c,"{}","application/json");}
else if(method=="POST"&&path=="/volume"){P d=obs_data_create_from_json(body.c_str());bool valid=false;if(d){std::string session=obs_data_get_string(d,"session");bool hasVolume=obs_data_has_user_value(d,"volume"),hasMute=obs_data_has_user_value(d,"muted");long long volume=obs_data_get_int(d,"volume");bool mute=obs_data_get_bool(d,"muted");obs_data_release(d);std::lock_guard lock(mutex);auto p=profileLocked(context);if(p&&!session.empty()&&(hasVolume||hasMute)&&(!hasVolume||(volume>=0&&volume<=100))){P media=obs_data_create_from_json(p->media.c_str());valid=obs_data_get_bool(media,"volumeAvailable")&&session==obs_data_get_string(media,"session");obs_data_release(media);if(valid){p->audioSession=session;if(hasVolume)p->audioVolume=(int)volume;if(hasMute)p->audioMute=mute?1:0;}}}reply(c,"{}","application/json",valid?200:400);}
else if(method=="POST"&&path=="/seek"){P d=obs_data_create_from_json(body.c_str());if(!d){reply(c,"{}","application/json",400);}else{long long ms=obs_data_get_int(d,"positionMs");std::string key=obs_data_get_string(d,"coverKey");obs_data_release(d);std::lock_guard lock(mutex);auto p=profileLocked(context);if(p&&ms>=0&&ms<=86400000&&!key.empty()){p->seekMs=ms;p->seekKey=key;reply(c,"{}","application/json");}else reply(c,"{}","application/json",400);}}
else if(method=="POST"&&(path=="/toggle"||path=="/next"||path=="/previous")){std::lock_guard lock(mutex);auto p=profileLocked(context);if(p)p->command=path.substr(1);reply(c,"{}","application/json");}
else reply(c,"{}","application/json",404);closesocket(c);}}
static void openSettings(P){std::wstring url=to_hstring(base()).c_str();ShellExecuteW(nullptr,L"open",url.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
static void stopWorkers()
{
 detachVisualizers();
 running=false;
 if(serverThread.joinable())serverThread.join();
 if(mediaThread.joinable())mediaThread.join();
 if(updateThread.joinable())updateThread.join();
 if(listener!=INVALID_SOCKET){closesocket(listener);listener=INVALID_SOCKET;WSACleanup();}
}
static void graphicsBarrier(P){}
static void frontendEvent(int event,P)
{
 if((event==8||event==26||event==13)&&running){syncScenes(nullptr);return;}
 // OBS 32.1: SCRIPTING_SHUTDOWN=36, EXIT=17. Both precede obs_shutdown.
 if((event!=36&&event!=17)||!running.exchange(false))return;
 blog(200,"[omni-mc] Closing panel before browser shutdown");
 stopWorkers();
 obs_queue_task(1,graphicsBarrier,nullptr,true);
 auto qt=GetModuleHandleW(L"Qt6Core.dll");
 auto flush=reinterpret_cast<void(*)(P,int)>(GetProcAddress(qt,"?sendPostedEvents@QCoreApplication@@SAXPEAVQObject@@H@Z"));
 if(dockRegistered&&settingsWidget){
  // CEF runs a nested event loop in its destructor. Keep the dock and its
  // layout alive until that destructor has finished, with the widget detached.
  auto widgets=GetModuleHandleW(L"Qt6Widgets.dll");
  auto parent=reinterpret_cast<P(*)(P)>(GetProcAddress(widgets,"?parentWidget@QWidget@@QEBAPEAV1@XZ"));
  auto setWidget=reinterpret_cast<void(*)(P,P)>(GetProcAddress(widgets,"?setWidget@QDockWidget@@QEAAXPEAVQWidget@@@Z"));
  auto setParent=reinterpret_cast<void(*)(P,P)>(GetProcAddress(widgets,"?setParent@QWidget@@QEAAXPEAV1@@Z"));
  auto later=reinterpret_cast<void(*)(P)>(GetProcAddress(qt,"?deleteLater@QObject@@QEAAXXZ"));
  if(parent&&setWidget&&setParent&&later&&flush){
   P container=parent(settingsWidget);
   if(container)setWidget(container,nullptr);
   setParent(settingsWidget,nullptr);
   later(settingsWidget);flush(settingsWidget,52); // QEvent::DeferredDelete only
   settingsWidget=nullptr;
   blog(200,"[omni-mc] Browser widget destroyed before dock container");
  }
 }
 if(dockRegistered){dockRegistered=false;obs_frontend_remove_dock("omni-mc-player");}
 delete cef;
 cef=nullptr;
 blog(200,"[omni-mc] Panel and workers stopped");
}
extern "C" __declspec(dllexport) void obs_module_set_pointer(P){}
extern "C" __declspec(dllexport) unsigned obs_module_ver(){return (32u<<24)|(1u<<16);}
extern "C" __declspec(dllexport) const char*obs_module_name(){return "Omni MC";}
extern "C" __declspec(dllexport) const char*obs_module_description(){return ui("Музыкальный плеер Windows с браузерным оверлеем","Windows music player with a browser overlay");}
extern "C" __declspec(dllexport) void obs_module_set_locale(const char*locale){englishUi=locale&&!(locale[0]=='r'&&locale[1]=='u');}
extern "C" __declspec(dllexport) void obs_module_free_locale(){}
extern "C" __declspec(dllexport) void obs_module_post_load()
{
 syncScenes(nullptr);
 auto browser=GetModuleHandleW(L"obs-browser.dll");
 if(!browser)return;
 auto factory=reinterpret_cast<QCef*(*)()>(GetProcAddress(browser,"obs_browser_create_qcef"));
 auto front=GetModuleHandleW(L"obs-frontend-api.dll");
 obs_frontend_add_dock_by_id=reinterpret_cast<decltype(obs_frontend_add_dock_by_id)>(GetProcAddress(front,"obs_frontend_add_dock_by_id"));
 if(!factory||!obs_frontend_add_dock_by_id)return;
 cef=factory();
 if(!cef)return;
 cef->init_browser();
 auto widget=cef->create_widget(nullptr,base()+"/",nullptr);
 if(!widget)return;
 settingsWidget=widget;
 // Qt 6 Windows x64 exports: the first argument is the QWidget instance.
 // Set constraints before OBS restores the saved dock geometry.
 auto qt=GetModuleHandleW(L"Qt6Widgets.dll");
 using WidgetSize=void(*)(P,int,int);
 auto minimum=reinterpret_cast<WidgetSize>(GetProcAddress(qt,"?setMinimumSize@QWidget@@QEAAXHH@Z"));
 auto resize=reinterpret_cast<WidgetSize>(GetProcAddress(qt,"?resize@QWidget@@QEAAXHH@Z"));
 if(minimum)minimum(widget,460,100);
 if(resize)resize(widget,482,900);
 dockRegistered=obs_frontend_add_dock_by_id("omni-mc-player","Omni MC",widget);
 if(dockRegistered)
  blog(200,"[omni-mc] Settings dock registered; minimum 460x100, initial 482x900");
#ifdef OMNI_MC_TEST
 // Exercise an initialized, visible CEF panel in shutdown regression checks.
 auto parent=reinterpret_cast<P(*)(P)>(GetProcAddress(qt,"?parentWidget@QWidget@@QEBAPEAV1@XZ"));
 auto floating=reinterpret_cast<void(*)(P,bool)>(GetProcAddress(qt,"?setFloating@QDockWidget@@QEAAX_N@Z"));
 if(dockRegistered&&parent&&floating)floating(parent(widget),false);
 auto window=reinterpret_cast<P(*)(P)>(GetProcAddress(qt,"?window@QWidget@@QEBAPEAV1@XZ"));
 auto visible=reinterpret_cast<void(*)(P,bool)>(GetProcAddress(qt,"?setVisible@QWidget@@UEAAX_N@Z"));
 if(dockRegistered&&window&&visible)visible(window(widget),true);
#endif
}
extern "C" __declspec(dllexport) bool obs_module_load(){
#ifdef OMNI_MC_TEST
if(std::wstring(GetCommandLineW()).find(L"--portable")==std::wstring::npos)return false;
#endif
auto lib=GetModuleHandleW(L"obs.dll"),front=GetModuleHandleW(L"obs-frontend-api.dll");
#define LOAD(name) name=reinterpret_cast<decltype(name)>(GetProcAddress(lib,#name));if(!name)return false;
LOAD(obs_data_create_from_json) LOAD(obs_data_create) LOAD(obs_data_release) LOAD(obs_data_get_json) LOAD(obs_data_get_string) LOAD(obs_data_get_bool) LOAD(obs_data_get_int) LOAD(obs_data_get_double) LOAD(obs_data_set_string) LOAD(obs_data_set_bool) LOAD(obs_data_set_int) LOAD(obs_source_set_name) LOAD(obs_enum_sources) LOAD(obs_enum_scenes) LOAD(obs_get_source_by_name) LOAD(obs_source_get_name) LOAD(obs_source_get_id) LOAD(obs_source_release) LOAD(obs_source_create) LOAD(obs_source_update) LOAD(obs_scene_from_source) LOAD(obs_scene_add) LOAD(obs_scene_find_source) LOAD(obs_scene_enum_items) LOAD(obs_sceneitem_get_source) LOAD(obs_queue_task) LOAD(blog) LOAD(obs_source_get_width) LOAD(obs_source_get_height) LOAD(obs_source_get_output_flags) LOAD(obs_source_video_render) LOAD(obs_source_add_audio_capture_callback) LOAD(obs_source_remove_audio_capture_callback) LOAD(obs_get_audio) LOAD(audio_output_get_sample_rate) LOAD(obs_enter_graphics) LOAD(obs_leave_graphics) LOAD(gs_texrender_create) LOAD(gs_texrender_destroy) LOAD(gs_texrender_begin) LOAD(gs_texrender_end) LOAD(gs_texrender_get_texture) LOAD(gs_stagesurface_create) LOAD(gs_stagesurface_destroy) LOAD(gs_stage_texture) LOAD(gs_stagesurface_map) LOAD(gs_stagesurface_unmap) LOAD(gs_ortho) LOAD(gs_clear) LOAD(gs_blend_state_push) LOAD(gs_blend_state_pop) LOAD(gs_reset_blend_state)
#undef LOAD
if(!loadProfiles(lib)||!loadBackdrop(lib))return false;
if(P audio=obs_get_audio())visualizerSampleRate=audio_output_get_sample_rate(audio);
obs_frontend_get_current_scene=reinterpret_cast<decltype(obs_frontend_get_current_scene)>(GetProcAddress(front,"obs_frontend_get_current_scene"));obs_frontend_add_tools_menu_item=reinterpret_cast<decltype(obs_frontend_add_tools_menu_item)>(GetProcAddress(front,"obs_frontend_add_tools_menu_item"));obs_frontend_remove_dock=reinterpret_cast<decltype(obs_frontend_remove_dock)>(GetProcAddress(front,"obs_frontend_remove_dock"));obs_frontend_add_event_callback=reinterpret_cast<decltype(obs_frontend_add_event_callback)>(GetProcAddress(front,"obs_frontend_add_event_callback"));if(!obs_frontend_get_current_scene||!obs_frontend_add_tools_menu_item||!obs_frontend_remove_dock||!obs_frontend_add_event_callback)return false;
wchar_t exe[MAX_PATH];GetModuleFileNameW(nullptr,exe,MAX_PATH);root=fs::path(exe).parent_path().parent_path().parent_path()/"data/obs-plugins/omni-mc";wchar_t appdata[MAX_PATH];GetEnvironmentVariableW(L"APPDATA",appdata,MAX_PATH);configPath=fs::path(appdata)/"obs-studio/plugin_config/omni-mc/settings.json";
#ifdef OMNI_MC_TEST
configPath=root/"../../../omni-mc/test-settings.json";
#endif
 systemFontList=windowsFonts();settings=read(configPath);P validated=settings.empty()?nullptr:obs_data_create_from_json(settings.c_str());if(!validated)settings=R"({"session":"@none","skin":"overlay","blur":false,"backgroundSource":"","blurStrength":7,"backgroundColor":"#285154","opacity":42,"brightness":96,"barColor":"#ed4169","cornerRadius":25,"brand":"Omni MC","auto":false,"hideIfIdle":false,"showSeconds":1,"direction":"left"})";if(validated){settings=obs_data_get_json(validated);obs_data_release(validated);}
 WSADATA ws;if(WSAStartup(MAKEWORD(2,2),&ws))return false;listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);BOOL exclusive=TRUE;setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char*)&exclusive,sizeof(exclusive));sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons(port);if(bind(listener,(sockaddr*)&a,sizeof(a))||listen(listener,16)){blog(100,"[omni-mc] Local port 17843 is occupied");closesocket(listener);WSACleanup();return false;}obs_frontend_add_event_callback(frontendEvent,nullptr);running=true;serverThread=std::thread(serve);mediaThread=std::thread(mediaLoop);
#ifndef OMNI_MC_TEST
 updateThread=std::thread(checkForUpdates);
#endif
 obs_frontend_add_tools_menu_item(ui("Omni MC — Музыкальный плеер","Omni MC — Music Player"),openSettings,nullptr);blog(200,"[omni-mc] Loaded version %s; settings at http://127.0.0.1:17843/",pluginVersion);return true;}
extern "C" __declspec(dllexport) void obs_module_unload(){stopWorkers();if(cef&&GetModuleHandleW(L"obs-browser.dll"))delete cef;cef=nullptr;}
