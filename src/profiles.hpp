#include <map>
#include <memory>
API(const char*,obs_source_get_uuid,P);
API(bool,obs_data_has_user_value,P,const char*);
API(P,obs_get_source_by_uuid,const char*);
API(P,obs_source_get_settings,P);
struct TransformInfo {float pos[2],rot,scale[2];unsigned alignment;int boundsType;unsigned boundsAlignment;float bounds[2];bool cropToBounds;};
struct CropInfo {int left,top,right,bottom;};
API(void,obs_sceneitem_get_info2,P,TransformInfo*);
API(void,obs_sceneitem_set_info2,P,const TransformInfo*);
API(void,obs_sceneitem_get_crop,P,CropInfo*);
API(void,obs_sceneitem_set_crop,P,const CropInfo*);
API(void,obs_sceneitem_remove,P);
API(int,obs_sceneitem_get_order_position,P);
API(void,obs_sceneitem_set_order_position,P,int);
API(bool,obs_sceneitem_set_visible,P,bool);
struct Profile {
 std::string id,cfg,media=R"({"sessions":[],"playing":false})",cover,command,lastKey,shadowCoverColor;
 long long overlayUntil=0,testUntil=0,fadeAt=0,idleChangedAt=0;float fadeFrom=1,fadeTo=1;bool idleInitialized=false,idlePlaying=false,idleHide=false;
 unsigned coverRevision=0,coverRetryAttempt=0;long long coverRetryAt=0;bool coverRetryPending=false;int layoutWidth=0,layoutHeight=0;
 long long seekMs=-1;std::string seekKey;
 int audioVolume=-1,audioMute=-1;std::string audioSession;
 P visualizerSource=nullptr;std::string visualizerSourceName;std::array<std::atomic<float>,21> visualizerBands{};std::atomic<long long> visualizerUpdated{0};
};
static std::map<std::string,std::shared_ptr<Profile>> profiles;
static std::mutex profileDiskMutex;
static std::string activeProfile;
static std::shared_ptr<Profile> profileLocked(const std::string&id){auto i=profiles.find(id.empty()?activeProfile:id);return i==profiles.end()?nullptr:i->second;}
static std::shared_ptr<Profile> ensureProfile(P scene){std::string id=obs_source_get_uuid(scene);std::lock_guard lock(mutex);auto&p=profiles[id];if(!p){p=std::make_shared<Profile>();p->id=id;p->cfg=read(configPath.parent_path()/"scenes"/(id+".json"));P d=p->cfg.empty()?nullptr:obs_data_create_from_json(p->cfg.c_str());if(!d){p->cfg=settings;d=obs_data_create_from_json(p->cfg.c_str());}if(d){bool normalized=false;if(!obs_data_has_user_value(d,"session")){obs_data_set_string(d,"session","@none");normalized=true;}if(!obs_data_has_user_value(d,"showSeconds")){obs_data_set_int(d,"showSeconds",1);normalized=true;}if(normalized)p->cfg=obs_data_get_json(d);obs_data_release(d);}}return p;}
static std::string playerName(const std::string&id){return "Omni MC — Плеер ["+id+"]";}
static void saveProfile(const std::shared_ptr<Profile>&p){std::lock_guard diskLock(profileDiskMutex);std::string cfg;{std::lock_guard lock(mutex);cfg=p->cfg;}fs::create_directories(configPath.parent_path()/"scenes");std::ofstream file(configPath.parent_path()/"scenes"/(p->id+".json"),std::ios::binary);file<<cfg;}
static void profileDimensions(const std::shared_ptr<Profile>&p,int&w,int&h)
{
 std::string cfg;int layoutW,layoutH;
 {std::lock_guard lock(mutex);cfg=p->cfg;layoutW=p->layoutWidth;layoutH=p->layoutHeight;}
 P d=obs_data_create_from_json(cfg.c_str());std::string skin=obs_data_get_string(d,"skin");skinSize(skin,w,h);
 bool adapt=!obs_data_has_user_value(d,"adaptiveSize")||obs_data_get_bool(d,"adaptiveSize"),visible=!obs_data_has_user_value(d,"progressVisible")||obs_data_get_bool(d,"progressVisible");
 if(adapt&&!visible){if(skin=="compact")h-=36;else if(skin=="split")h-=20;else if(skin=="orbit")h-=8;else if(skin!="cover")h-=30;}
 bool coverHidden=skin!="compact-bar"&&skin!="cover"&&skin!="vinyl"&&skin!="orbit"&&skin!="prism"&&skin!="eclipse"&&obs_data_has_user_value(d,"showTrackCover")&&!obs_data_get_bool(d,"showTrackCover");
 long long defaultCoverScale=skin=="compact"||skin=="prism"?80:skin=="cassette"?50:skin=="cosmos"||skin=="gargantua"||skin=="rhythm"?110:100,coverValue=obs_data_get_int(d,"coverScale"),coverScale=std::clamp<long long>(coverValue?coverValue:defaultCoverScale,50,150);int coverSize=(int)std::lround(150.0*coverScale/100.0);
 if(!coverHidden){if(skin=="compact")h=std::max(h,coverSize+74);else if(skin=="cover"){w=coverSize+60;h=coverSize+64;}else if(skin=="prism")h=std::max(h,coverSize+70);else if(skin=="cassette")h=std::max(h,coverSize+266);else if(skin=="neon")h=std::max(h,coverSize+92);else if(skin=="vector")h=std::max(h,coverSize+76);else if(skin=="pulse")h=std::max(h,coverSize+112);else if(skin=="rhythm")h=std::max(h,coverSize+55);else if(skin=="cosmos"||skin=="gargantua")h=std::max(h,coverSize+122);}
 if(coverHidden){if(skin=="compact")w-=112;else if(skin=="orbit")w-=208;else if(skin=="prism")w-=104;else if(skin!="cassette")h=180;}
 if(coverHidden&&layoutW>0&&layoutH>0){w=layoutW;h=layoutH;}
 auto factor=[&](const char*key){auto value=obs_data_get_int(d,key);return std::clamp<long long>(value?value:100,50,150)/100.0;};w=(int)std::lround(w*factor("widgetWidthScale"))+100;h=(int)std::lround(h*factor("widgetHeightScale"))+100;if(coverHidden)h=std::max(200,h);obs_data_release(d);
}
static void refreshProfilePlayer(P pointer){std::unique_ptr<std::string> id(static_cast<std::string*>(pointer));if(!running)return;std::shared_ptr<Profile> p;{std::lock_guard lock(mutex);p=profileLocked(*id);}if(!p)return;P source=obs_get_source_by_name(playerName(p->id).c_str());if(!source)return;int w,h;profileDimensions(p,w,h);P d=obs_data_create();obs_data_set_int(d,"width",w);obs_data_set_int(d,"height",h);obs_data_set_string(d,"omni_mc_context",p->id.c_str());obs_data_set_string(d,"url",(base()+"/overlay?context="+p->id+"&padding=50&refresh="+std::to_string(now())).c_str());obs_source_update(source,d);obs_data_release(d);obs_source_release(source);}
static void applyMeasuredLayout(P pointer){std::unique_ptr<std::string> id(static_cast<std::string*>(pointer));if(!running)return;std::shared_ptr<Profile> p;{std::lock_guard lock(mutex);p=profileLocked(*id);}if(!p)return;P source=obs_get_source_by_name(playerName(p->id).c_str());if(!source)return;int w,h;profileDimensions(p,w,h);P data=obs_data_create();obs_data_set_int(data,"width",w);obs_data_set_int(data,"height",h);obs_source_update(source,data);obs_data_release(data);obs_source_release(source);}
static bool loadProfiles(HMODULE lib){
#define PLOAD(name) name=reinterpret_cast<decltype(name)>(GetProcAddress(lib,#name));if(!name)return false;
 PLOAD(obs_source_get_uuid) PLOAD(obs_get_source_by_uuid) PLOAD(obs_source_get_settings) PLOAD(obs_sceneitem_get_info2) PLOAD(obs_sceneitem_set_info2) PLOAD(obs_sceneitem_get_crop) PLOAD(obs_sceneitem_set_crop) PLOAD(obs_sceneitem_remove) PLOAD(obs_sceneitem_get_order_position) PLOAD(obs_sceneitem_set_order_position) PLOAD(obs_sceneitem_set_visible)
#undef PLOAD
 return true;
}
