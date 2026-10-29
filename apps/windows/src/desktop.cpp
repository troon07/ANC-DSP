#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include "tonal_session.h"
#include <memory>
#include <sstream>
#include <vector>
namespace {
HWND window,inputBox,outputBox,frequencyBox,statusBox,startButton,stopButton;HFONT font;
std::vector<anc::AudioDeviceInfo> inputs,outputs;std::unique_ptr<anc::TonalSession> session;
std::filesystem::path lastFolder;
std::wstring wide(const std::string&s){if(s.empty())return L"";int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);std::wstring w(n,L'\0');MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,w.data(),n);w.pop_back();return w;}
HWND control(const wchar_t*type,const wchar_t*text,int id,int x,int y,int w,int h,DWORD style=0){HWND c=CreateWindowW(type,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,reinterpret_cast<HMENU>(INT_PTR(id)),nullptr,nullptr);SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return c;}
void status(const std::string&s){SetWindowTextW(statusBox,wide(s).c_str());}
void refresh(){anc::WasapiBackend a;inputs=a.enumerateCaptureDevices();outputs=a.enumerateRenderDevices();SendMessageW(inputBox,CB_RESETCONTENT,0,0);SendMessageW(outputBox,CB_RESETCONTENT,0,0);int in=0,out=0;
 for(size_t k=0;k<inputs.size();++k){auto&s=inputs[k];std::string label=s.name+" | "+std::to_string(s.default_sample_rate)+" Hz";SendMessageW(inputBox,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(wide(label).c_str()));if(s.name.find("Realtek")!=std::string::npos)in=int(k);}
 for(size_t k=0;k<outputs.size();++k){auto&s=outputs[k];std::string label=s.name+" | "+std::to_string(s.default_sample_rate)+" Hz";SendMessageW(outputBox,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(wide(label).c_str()));if(s.name.find("Nirvana")!=std::string::npos)out=int(k);}
 SendMessageW(inputBox,CB_SETCURSEL,in,0);SendMessageW(outputBox,CB_SETCURSEL,out,0);
}
void finish(bool user){if(!session)return;try{status(session->finish(user));lastFolder=session->folder();}catch(const std::exception&e){status(e.what());}session.reset();EnableWindow(startButton,TRUE);EnableWindow(stopButton,FALSE);}
LRESULT CALLBACK procedure(HWND h,UINT message,WPARAM w,LPARAM l){
 if(message==WM_CREATE){window=h;font=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 control(L"STATIC",L"Universal ANC Lab — live tonal experiment",0,25,20,850,35);
 control(L"STATIC",L"Bench test outside your ears. Turn boAt ANC/Ambient OFF.\nPhone: play SPEAKER TONE. Keep the LEFT earbud fixed beside the laptop microphone.\nResult applies to this microphone point; ear-canal ANC is unverified.",0,25,65,850,85);
 control(L"STATIC",L"Error sensor — choose the laptop microphone",0,25,155,800,25);inputBox=control(L"COMBOBOX",L"",101,25,185,820,200,CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP);
 control(L"STATIC",L"Control output — choose Nirvana Crystl Headphones (not communications)",0,25,230,820,25);outputBox=control(L"COMBOBOX",L"",102,25,260,820,200,CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP);
 control(L"STATIC",L"Tone frequency, Hz (100–1000)",0,25,305,310,25);frequencyBox=control(L"EDIT",L"300",103,350,300,120,30,WS_BORDER|ES_NUMBER|WS_TABSTOP);
 startButton=control(L"BUTTON",L"Start 32-second test",104,25,350,230,42,WS_TABSTOP);stopButton=control(L"BUTTON",L"Stop and mute",105,270,350,180,42,WS_TABSTOP);EnableWindow(stopButton,FALSE);
 control(L"BUTTON",L"Open session files",106,465,350,190,42,WS_TABSTOP);control(L"BUTTON",L"Refresh devices",107,670,350,175,42,WS_TABSTOP);
 statusBox=control(L"EDIT",L"Ready. Start the independent phone tone first. Calibration automatically stops if the tone/path is unsuitable.",108,25,420,820,160,ES_MULTILINE|ES_READONLY|WS_BORDER|WS_VSCROLL);
 refresh();SetTimer(h,1,250,nullptr);return 0;}
 if(message==WM_COMMAND){int id=LOWORD(w);
  if(id==104 && !session){try{int i=int(SendMessageW(inputBox,CB_GETCURSEL,0,0)),o=int(SendMessageW(outputBox,CB_GETCURSEL,0,0));if(i<0||o<0)throw std::runtime_error("Select microphone and headphones");wchar_t text[32]{};GetWindowTextW(frequencyBox,text,32);double f=std::stod(text);
    wchar_t docs[MAX_PATH];if(FAILED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,SHGFP_TYPE_CURRENT,docs)))throw std::runtime_error("Cannot find Documents folder");
    session=std::make_unique<anc::TonalSession>();session->start(inputs.at(i),outputs.at(o),f,std::filesystem::path(docs)/L"UniversalANC"/L"sessions");EnableWindow(startButton,FALSE);EnableWindow(stopButton,TRUE);status("Recording live microphone; calibration in progress.");
   }catch(const std::exception&e){session.reset();status(e.what());}}
  if(id==105)finish(true);
  if(id==106&&!lastFolder.empty())ShellExecuteW(h,L"open",lastFolder.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
  if(id==107&&!session)refresh();return 0;}
 if(message==WM_TIMER&&session){if(session->finished()||!session->running())finish(false);else {std::ostringstream s;s<<anc::toneStageName(session->stage())<<" | "<<int(session->seconds())<<" / 32 seconds\r\nLive microphone RMS: "<<session->rms()<<" FS\r\nTarget-tone power: "<<session->tonePower()<<" FS^2\r\nLast DSP callback: "<<session->callbackUs()<<" us (not round-trip latency)\r\nKeep source, volume and microphone position fixed. Output is capped at 0.08 FS.";status(s.str());}return 0;}
 if(message==WM_CLOSE){finish(true);DestroyWindow(h);return 0;}
 if(message==WM_DESTROY){KillTimer(h,1);if(font)DeleteObject(font);PostQuitMessage(0);return 0;}
 return DefWindowProcW(h,message,w,l);
}
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR,int show){WNDCLASSW c{};c.lpfnWndProc=procedure;c.hInstance=instance;c.lpszClassName=L"UniversalANCTonal";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&c);
 HWND h=CreateWindowW(c.lpszClassName,L"Universal ANC Lab — experimental acoustic control",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,900,650,nullptr,nullptr,instance,nullptr);ShowWindow(h,show);MSG m{};while(GetMessageW(&m,nullptr,0,0)>0){if(!IsDialogMessageW(h,&m)){TranslateMessage(&m);DispatchMessageW(&m);}}return 0;}
