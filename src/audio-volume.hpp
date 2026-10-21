#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <cwctype>
#include <appmodel.h>

static std::wstring audioLower(std::wstring value){if(!value.empty())CharLowerBuffW(value.data(),(DWORD)value.size());return value;}
static bool audioMatches(DWORD pid,const std::string&session){
 HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process)return false;
 wchar_t path[32768];DWORD size=32768;bool found=QueryFullProcessImageNameW(process,0,path,&size)!=FALSE;
 wchar_t app[4096];UINT32 appSize=4096;bool packaged=GetApplicationUserModelId(process,&appSize,app)==ERROR_SUCCESS;
 CloseHandle(process);auto id=audioLower(to_hstring(session).c_str());
 if(packaged&&audioLower(app)==id)return true;if(!found)return false;
 auto exe=audioLower(fs::path(std::wstring(path,size)).stem().wstring());
 if(id==exe)return true;
 if(id==L"ru.yandex.desktop.music"&&exe==L"яндекс музыка")return true;
 struct Match{const wchar_t*id;const wchar_t*exe;};
 static const Match known[]={{L"yandex",L"yandexmusic"},{L"spotify",L"spotify"},{L"chrome",L"chrome"},{L"msedge",L"msedge"},{L"firefox",L"firefox"},{L"zunemusic",L"music.ui"},{L"zunemusic",L"mediaplayer"},{L"wmplayer",L"wmplayer"},{L"opera",L"opera"}};
 for(auto&m:known)if(id.find(m.id)!=std::wstring::npos&&exe==m.exe)return true;
 return false;
}
struct ApplicationVolume{bool available=false,muted=false;float volume=1;};
struct CachedAudioSession {com_ptr<IMMDevice> device;com_ptr<IAudioSessionControl> control;};
struct CachedApplicationAudio {std::vector<CachedAudioSession> sessions;long long scannedAt=0;};
static std::map<std::string,CachedApplicationAudio> applicationAudioCache;
static com_ptr<IMMDeviceEnumerator> audioDeviceEnumerator;
static void clearApplicationVolumeCache(){applicationAudioCache.clear();audioDeviceEnumerator=nullptr;}
static ApplicationVolume applicationVolume(const std::string&id,int volume=-1,int mute=-1){
 ApplicationVolume result;
 try{
  if(!audioDeviceEnumerator)check_hresult(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),audioDeviceEnumerator.put_void()));
  auto&cached=applicationAudioCache[id];long long sampledAt=now();bool rescan=!cached.scannedAt||sampledAt-cached.scannedAt>=(cached.sessions.empty()?1000:3000)||volume>=0||mute>=0;
  for(auto&session:cached.sessions){DWORD deviceState=0;AudioSessionState sessionState;auto info=session.control.try_as<IAudioSessionControl2>();if(!info||FAILED(session.device->GetState(&deviceState))||deviceState!=DEVICE_STATE_ACTIVE||FAILED(info->GetState(&sessionState))||sessionState==AudioSessionStateExpired){rescan=true;break;}}
  if(rescan){
  cached.sessions.clear();cached.scannedAt=sampledAt;
  com_ptr<IMMDeviceCollection> devices;check_hresult(audioDeviceEnumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,devices.put()));UINT count=0;check_hresult(devices->GetCount(&count));
  for(UINT i=0;i<count;i++){
   com_ptr<IMMDevice> device;if(FAILED(devices->Item(i,device.put())))continue;
   com_ptr<IAudioSessionManager2> manager;if(FAILED(device->Activate(__uuidof(IAudioSessionManager2),CLSCTX_ALL,nullptr,manager.put_void())))continue;
   com_ptr<IAudioSessionEnumerator> sessions;if(FAILED(manager->GetSessionEnumerator(sessions.put())))continue;int n=0;sessions->GetCount(&n);
   for(int j=0;j<n;j++){
    com_ptr<IAudioSessionControl> control;if(FAILED(sessions->GetSession(j,control.put())))continue;
    auto info=control.try_as<IAudioSessionControl2>();DWORD pid=0;AudioSessionState state;
    if(!info||FAILED(info->GetProcessId(&pid))||!pid||FAILED(info->GetState(&state))||state==AudioSessionStateExpired||!audioMatches(pid,id))continue;
    cached.sessions.push_back({device,control});
   }
  }
  }
  for(auto&session:cached.sessions){
    auto audio=session.control.try_as<ISimpleAudioVolume>();if(!audio)continue;
    if(volume>=0)check_hresult(audio->SetMasterVolume(std::clamp(volume,0,100)/100.f,nullptr));
    if(mute>=0)check_hresult(audio->SetMute(mute!=0,nullptr));
    float level;BOOL muted;if(SUCCEEDED(audio->GetMasterVolume(&level))&&SUCCEEDED(audio->GetMute(&muted))){if(!result.available){result.volume=level;result.muted=muted!=FALSE;}result.available=true;}
  }
 }catch(const hresult_error&){clearApplicationVolumeCache();}
 return result;
}
