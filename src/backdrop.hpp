struct alignas(16) Matrix {float v[16];};
struct SourceInfo {
 const char*id;int type;unsigned flags;
 const char*(*name)(P);P(*create)(P,P);void(*destroy)(P);
 unsigned(*width)(P);unsigned(*height)(P);
 void(*defaults)(P);P(*properties)(P);void(*update)(P,P);
 void(*activate)(P);void(*deactivate)(P);void(*show)(P);void(*hide)(P);
 void(*tick)(P,float);void(*render)(P,P);
};
API(void,obs_register_source_s,const SourceInfo*,size_t);
API(P,obs_source_create_private,const char*,const char*,P);
API(P,obs_source_get_filter_by_name,P,const char*);
API(void,obs_source_filter_add,P,P);
API(void,obs_source_filter_remove,P,P);
API(void,obs_source_enum_filters,P,void(*)(P,P,P),P);
API(void,obs_source_inc_showing,P);
API(void,obs_source_dec_showing,P);
API(P,obs_source_get_weak_source,P);
API(P,obs_weak_source_get_source,P);
API(void,obs_weak_source_release,P);
API(P,obs_filter_get_parent,P);
API(void,obs_source_skip_video_filter,P);
API(bool,obs_source_process_filter_begin,P,int,int);
API(void,obs_source_process_filter_end,P,P,unsigned,unsigned);
API(void,obs_data_set_double,P,const char*,double);
API(P,obs_group_from_source,P);
API(void,obs_sceneitem_get_draw_transform,P,Matrix*);
API(bool,obs_sceneitem_visible,P);
static void matrix4_identity(Matrix*m){std::fill(std::begin(m->v),std::end(m->v),0.f);m->v[0]=m->v[5]=m->v[10]=m->v[15]=1.f;}
API(void,matrix4_mul,Matrix*,const Matrix*,const Matrix*);
API(bool,matrix4_inv,Matrix*,const Matrix*);
API(void,gs_matrix_push);
API(void,gs_matrix_pop);
API(void,gs_matrix_mul,const Matrix*);
API(void,gs_texrender_reset,P);
API(P,gs_effect_create_from_file,const char*,char**);
API(void,gs_effect_destroy,P);
API(P,gs_effect_get_param_by_name,P,const char*);
API(void,gs_effect_set_texture,P,P);
API(void,gs_effect_set_float,P,float);
API(void,gs_effect_set_vec4,P,const float*);
API(P,gs_texture_create,unsigned,unsigned,int,unsigned,const unsigned char**,unsigned);
API(void,gs_texture_destroy,P);
API(unsigned,gs_texture_get_width,P);
API(unsigned,gs_texture_get_height,P);
API(void,gs_draw_sprite,P,unsigned,unsigned,unsigned);
API(P,obs_get_base_effect,int);
API(bool,gs_effect_loop,P,const char*);
API(void,gs_matrix_translate3f,float,float,float);
API(void,gs_blend_function,int,int);
static std::atomic<bool> backdropPending{false};
static thread_local bool renderingBackdrop=false;
static float idleVisibilityLocked(Profile&p){long long t=now();float value=p.fadeFrom+(p.fadeTo-p.fadeFrom)*std::clamp((float)(t-p.fadeAt)/500.f,0.f,1.f);P cfg=obs_data_create_from_json(p.cfg.c_str()),state=obs_data_create_from_json(p.media.c_str());bool hide=obs_data_get_bool(cfg,"hideIfIdle"),playing=obs_data_get_bool(state,"playing");int seconds=(int)std::clamp<long long>(obs_data_get_int(cfg,"showSeconds")?obs_data_get_int(cfg,"showSeconds"):1,1,30);obs_data_release(cfg);obs_data_release(state);if(!p.idleInitialized||hide!=p.idleHide){p.idleInitialized=true;p.idleHide=hide;p.idlePlaying=playing;p.idleChangedAt=t;}else if(playing!=p.idlePlaying){p.idlePlaying=playing;p.idleChangedAt=t;}float target=p.fadeTo;if(!hide||t<p.testUntil)target=1.f;else if(t-p.idleChangedAt>=seconds*1000LL)target=playing?1.f:0.f;if(target!=p.fadeTo){p.fadeFrom=value;p.fadeTo=target;p.fadeAt=t;}return value;}
struct Backdrop {
 std::shared_ptr<Profile> profile;
 P filter=nullptr,raw=nullptr,blur=nullptr,texture=nullptr,effect=nullptr;
 unsigned w=720,h=240;Matrix relative{};
 std::string cfg,name,skin,context;bool enabled=false,automatic=false,hideIfIdle=false,coverHidden=false;
 int strength=-1,seconds=5;float splitHeight=110,glassHeight=111;float brightness=1,tintOpacity=1,cornerRadius=25;
 P coverTexture=nullptr;unsigned coverRevision=~0u;
 P backgroundSource=nullptr;
};
static P decodeCover(const std::string&bytes){
 if(bytes.empty())return nullptr;
 HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);P texture=nullptr;
 try{com_ptr<IWICImagingFactory> factory;check_hresult(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,__uuidof(IWICImagingFactory),factory.put_void()));
  com_ptr<IWICStream> stream;check_hresult(factory->CreateStream(stream.put()));check_hresult(stream->InitializeFromMemory((BYTE*)bytes.data(),(DWORD)bytes.size()));
  com_ptr<IWICBitmapDecoder> decoder;check_hresult(factory->CreateDecoderFromStream(stream.get(),nullptr,WICDecodeMetadataCacheOnLoad,decoder.put()));com_ptr<IWICBitmapFrameDecode> frame;check_hresult(decoder->GetFrame(0,frame.put()));
  UINT w,h;check_hresult(frame->GetSize(&w,&h));if(!w||!h||w>8192||h>8192)throw hresult_invalid_argument();
  com_ptr<IWICFormatConverter> converter;check_hresult(factory->CreateFormatConverter(converter.put()));check_hresult(converter->Initialize(frame.get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
  std::vector<unsigned char> pixels((size_t)w*h*4);check_hresult(converter->CopyPixels(nullptr,w*4,(UINT)pixels.size(),pixels.data()));const unsigned char*data=pixels.data();obs_enter_graphics();texture=gs_texture_create(w,h,3,1,&data,0);obs_leave_graphics();
 }catch(const hresult_error&e){blog(300,"[omni-mc] Cover decode: %s",to_string(e.message()).c_str());}
 if(SUCCEEDED(apartment))CoUninitialize();return texture;
}
struct FindTransform {P target;Matrix parent,result;bool found=false;unsigned depth=0;};
static bool findTransformItem(P,P item,P pointer)
{
 auto&f=*static_cast<FindTransform*>(pointer);
 if(!obs_sceneitem_visible(item))return true;
 P source=obs_sceneitem_get_source(item);Matrix local,world;
 obs_sceneitem_get_draw_transform(item,&local);matrix4_mul(&world,&local,&f.parent);
 if(source==f.target){f.result=world;f.found=true;return false;}
 if(f.depth<16){P nested=obs_scene_from_source(source);if(!nested)nested=obs_group_from_source(source);
  if(nested){FindTransform child{f.target,world,{},false,f.depth+1};obs_scene_enum_items(nested,findTransformItem,&child);if(child.found){f.result=child.result;f.found=true;return false;}}}
 return true;
}
static bool sourceTransform(P sceneSource,P target,Matrix&result)
{
 matrix4_identity(&result);if(sceneSource==target)return true;
 FindTransform f{};f.target=target;matrix4_identity(&f.parent);
 P scene=obs_scene_from_source(sceneSource);if(scene)obs_scene_enum_items(scene,findTransformItem,&f);
 if(f.found)result=f.result;return f.found;
}
static const char*rawName(P){return "Omni MC — Область фона";}
static P rawCreate(P,P){return nullptr;}
static void rawDestroy(P){}
static unsigned rawWidth(P p){return p?static_cast<Backdrop*>(p)->w:720;}
static unsigned rawHeight(P p){return p?static_cast<Backdrop*>(p)->h:240;}
// The context pointer is supplied through the private source's settings.
static P rawContextCreate(P data,P){return reinterpret_cast<P>((uintptr_t)obs_data_get_int(data,"context"));}
static void rawRender(P pointer,P)
{
 auto*b=static_cast<Backdrop*>(pointer);if(!b||!b->enabled||b->name.empty()||renderingBackdrop)return;
 if(b->name=="@track-cover"){
  if(!b->coverTexture)return;unsigned w=gs_texture_get_width(b->coverTexture),h=gs_texture_get_height(b->coverTexture),contentW=b->w>100?b->w-100:b->w,contentH=b->h>100?b->h-100:b->h;float scale=std::max((float)contentW/w,(float)contentH/h);
  gs_matrix_push();gs_matrix_translate3f(50+((float)contentW-w*scale)/2,50+((float)contentH-h*scale)/2,0);
  P effect=obs_get_base_effect(0);gs_effect_set_texture(gs_effect_get_param_by_name(effect,"image"),b->coverTexture);while(gs_effect_loop(effect,"Draw"))gs_draw_sprite(b->coverTexture,0,(unsigned)(w*scale+.5f),(unsigned)(h*scale+.5f));gs_matrix_pop();return;
 }
 P selected=obs_get_source_by_name(b->name.c_str());if(!selected)return;
 P player=obs_filter_get_parent(b->filter);
 if(selected!=player){
  P scene=b->profile?obs_get_source_by_uuid(b->profile->id.c_str()):obs_frontend_get_current_scene();Matrix playerWorld,backgroundWorld,inverse;
  matrix4_identity(&playerWorld);matrix4_identity(&backgroundWorld);
  bool located=scene&&sourceTransform(scene,player,playerWorld);
  if(scene)sourceTransform(scene,selected,backgroundWorld);
  if(scene)obs_source_release(scene);
  if(located&&matrix4_inv(&inverse,&playerWorld)){
   matrix4_mul(&b->relative,&backgroundWorld,&inverse);
   gs_matrix_push();gs_matrix_mul(&b->relative);
   renderingBackdrop=true;obs_source_video_render(selected);renderingBackdrop=false;
   gs_matrix_pop();
  }
 }
 obs_source_release(selected);
}
static const char*backdropName(P){return "Omni MC — Фон (Composite Blur)";}
static P backdropCreate(P,P source)
{
 auto*b=new Backdrop;b->filter=source;
 P settings=obs_data_create();obs_data_set_int(settings,"context",(long long)(uintptr_t)b);
 b->raw=obs_source_create_private("omni_mc_background_region",nullptr,settings);obs_data_release(settings);
 obs_enter_graphics();b->texture=gs_texrender_create(3,0);
 std::string effectPath=pathUtf8(root/"backdrop.effect");b->effect=gs_effect_create_from_file(effectPath.c_str(),nullptr);obs_leave_graphics();
 return b;
}
static void backdropDestroy(P pointer)
{
 auto*b=static_cast<Backdrop*>(pointer);
 if(b->backgroundSource){P held=obs_weak_source_get_source(b->backgroundSource);if(held){obs_source_dec_showing(held);obs_source_release(held);}obs_weak_source_release(b->backgroundSource);}
 if(b->raw)obs_source_release(b->raw);
 if(b->blur)obs_source_release(b->blur);
 obs_enter_graphics();if(b->texture)gs_texrender_destroy(b->texture);if(b->coverTexture)gs_texture_destroy(b->coverTexture);if(b->effect)gs_effect_destroy(b->effect);obs_leave_graphics();delete b;
}
static void backdropTick(P pointer,float)
{
 auto*b=static_cast<Backdrop*>(pointer);if(b->texture)gs_texrender_reset(b->texture);
 P owner=obs_filter_get_parent(b->filter);if(!owner)return;
 P ownerData=obs_source_get_settings(owner);std::string id=obs_data_get_string(ownerData,"omni_mc_context");obs_data_release(ownerData);
 if(!b->profile||b->context!=id){b->context=id;b->coverRevision=~0u;std::lock_guard lock(mutex);b->profile=id.empty()?nullptr:profileLocked(id);b->cfg.clear();}if(!b->profile)return;
 std::string cfg;{std::lock_guard lock(mutex);cfg=b->profile->cfg;}
 if(cfg!=b->cfg){b->cfg=cfg;P data=obs_data_create_from_json(cfg.c_str());
  b->name=obs_data_get_string(data,"backgroundSource");b->enabled=obs_data_get_bool(data,"blur")&&!b->name.empty();
  b->skin=obs_data_get_string(data,"skin");b->coverHidden=b->skin!="cover"&&b->skin!="vinyl"&&b->skin!="orbit"&&b->skin!="prism"&&b->skin!="eclipse"&&obs_data_has_user_value(data,"showTrackCover")&&!obs_data_get_bool(data,"showTrackCover");auto fontScale=[&](const char*key){auto v=obs_data_get_int(data,key);return (float)std::clamp<long long>(v?v:100,50,200)/100.f;};auto thicknessValue=obs_data_get_int(data,"progressThickness");float thickness=(float)std::clamp<long long>(thicknessValue?thicknessValue:(b->skin=="compact-bar"?3:5),1,12);bool showProgress=!obs_data_has_user_value(data,"progressVisible")||obs_data_get_bool(data,"progressVisible");bool adapt=!obs_data_has_user_value(data,"adaptiveSize")||obs_data_get_bool(data,"adaptiveSize"),shrink=adapt&&!showProgress;b->splitHeight=std::max(shrink?90.f:110.f,(shrink?50.f:70.f)+std::max(0.f,thickness-3.f)+16.f*fontScale("artistScale")+14.f*fontScale("titleScale"));b->glassHeight=(showProgress||!adapt?73.f+thickness:46.f)+16.f*fontScale("artistScale")+16.f*fontScale("titleScale");if(showProgress)b->glassHeight+=thickness-3.f;b->automatic=obs_data_get_bool(data,"auto");b->seconds=(int)std::clamp<long long>(obs_data_get_int(data,"showSeconds")?obs_data_get_int(data,"showSeconds"):5,1,30);
  if(b->skin=="compact-bar")b->coverHidden=false;
  b->hideIfIdle=obs_data_get_bool(data,"hideIfIdle");
  b->brightness=std::clamp((float)obs_data_get_int(data,"brightness")/100.f,.1f,1.5f);
  b->tintOpacity=std::clamp((float)obs_data_get_int(data,"opacity")/100.f,0.f,1.f);
  b->cornerRadius=(float)std::clamp<long long>(obs_data_has_user_value(data,"cornerRadius")?obs_data_get_int(data,"cornerRadius"):25,0,100);
  int radius=(int)std::clamp<long long>(obs_data_get_int(data,"blurStrength"),0,25);obs_data_release(data);
  if(!b->blur&&b->raw){P parameters=obs_data_create();obs_data_set_int(parameters,"blur_algorithm",1);obs_data_set_int(parameters,"blur_type",1);obs_data_set_double(parameters,"radius",radius);obs_data_set_string(parameters,"background","");b->blur=obs_source_create_private("obs_composite_blur","Composite Blur",parameters);obs_data_release(parameters);if(b->blur)obs_source_filter_add(b->raw,b->blur);else blog(100,"[omni-mc] Composite Blur is not available");}
  if(b->blur&&radius!=b->strength){P parameters=obs_data_create();obs_data_set_int(parameters,"blur_algorithm",1);obs_data_set_int(parameters,"blur_type",1);obs_data_set_double(parameters,"radius",radius);obs_data_set_string(parameters,"background","");obs_source_update(b->blur,parameters);obs_data_release(parameters);}
  b->strength=radius;
 }
 P parent=obs_filter_get_parent(b->filter);if(parent){b->w=obs_source_get_width(parent);b->h=obs_source_get_height(parent);}
 P selected=b->enabled&&b->name!="@track-cover"?obs_get_source_by_name(b->name.c_str()):nullptr;
 if(selected==parent){obs_source_release(selected);selected=nullptr;}
 P held=b->backgroundSource?obs_weak_source_get_source(b->backgroundSource):nullptr;
 if(selected!=held){if(held)obs_source_dec_showing(held);if(b->backgroundSource)obs_weak_source_release(b->backgroundSource);b->backgroundSource=selected?obs_source_get_weak_source(selected):nullptr;if(selected)obs_source_inc_showing(selected);}
 if(held)obs_source_release(held);if(selected)obs_source_release(selected);
 if(b->name=="@track-cover"){std::string bytes;unsigned revision;{std::lock_guard lock(mutex);revision=b->profile->coverRevision;if(revision!=b->coverRevision)bytes=b->profile->cover;}
  if(revision!=b->coverRevision){P next=decodeCover(bytes);obs_enter_graphics();if(b->coverTexture)gs_texture_destroy(b->coverTexture);obs_leave_graphics();b->coverTexture=next;b->coverRevision=revision;}
 }
}
static float appearance(Backdrop*b)
{
 long long musicEnd,testEnd;float idle;{std::lock_guard lock(mutex);if(!b->profile)return 0;idle=idleVisibilityLocked(*b->profile);musicEnd=b->profile->overlayUntil;testEnd=b->profile->testUntil;}
 if(!b->automatic)return idle;
 long long end=std::max(musicEnd,testEnd),start=end==testEnd?end-5000:end-std::max(1,b->seconds)*1000;
 long long t=now();if(t<end)return idle*std::clamp((float)(t-start)/500.f,0.f,1.f);return idle*std::clamp(1.f-(float)(t-end)/500.f,0.f,1.f);
}
static void backdropRender(P pointer,P)
{
 if(renderingBackdrop)return; // Exclude the player when rendering its background scene.
 auto*b=static_cast<Backdrop*>(pointer);
 if(!running||!b->profile||!b->enabled||!b->blur||!b->effect||!b->texture||!b->raw||!b->w||!b->h){obs_source_skip_video_filter(b->filter);return;}
 if(gs_texrender_begin(b->texture,b->w,b->h)){
  float clear[4]={0,0,0,0};gs_clear(1,clear,0,0);gs_ortho(0,(float)b->w,0,(float)b->h,-100,100);
  gs_blend_state_push();gs_reset_blend_state();obs_source_video_render(b->raw);gs_blend_state_pop();gs_texrender_end(b->texture);
 }
 P texture=gs_texrender_get_texture(b->texture);if(!texture||!obs_source_process_filter_begin(b->filter,3,0)){obs_source_skip_video_filter(b->filter);return;}
 float logicalWidth=(float)b->w,logicalHeight=(float)b->h;
 float rect[4]={70,72,logicalWidth-140,logicalHeight-144};
 if(b->skin=="compact-bar"||b->coverHidden&&b->skin!="compact"||b->skin=="prism"||b->skin=="cassette"||b->skin=="orbit"||b->skin=="vinyl"||b->skin=="poster"||b->skin=="cinematic"||b->skin=="glass"){rect[0]=70;rect[1]=70;rect[2]=logicalWidth-140;rect[3]=logicalHeight-140;}else if(b->skin=="split"){rect[0]=70;rect[1]=logicalHeight-70-b->splitHeight;rect[2]=logicalWidth-140;rect[3]=b->splitHeight;}
 if(b->skin=="eclipse"){rect[0]=146;rect[1]=74;rect[2]=logicalWidth-216;rect[3]=logicalHeight-148;}
 float progress=appearance(b);std::string direction;{std::lock_guard lock(mutex);P data=obs_data_create_from_json(b->profile->cfg.c_str());direction=obs_data_get_string(data,"direction");obs_data_release(data);}
 float animationHeight=(b->skin=="split"||b->skin=="glass")?logicalHeight-140:rect[3];
 if(direction=="right")rect[0]+=(1-progress)*rect[2]*1.1f;
 else if(direction=="top")rect[1]-=(1-progress)*animationHeight*1.2f;
 else if(direction=="bottom")rect[1]+=(1-progress)*animationHeight*1.2f;
 else if(direction!="fade")rect[0]-=(1-progress)*rect[2]*1.1f;
 float fillTop=-1.f;
 float size[4]={(float)b->w,(float)b->h,1,1};
 gs_effect_set_texture(gs_effect_get_param_by_name(b->effect,"backdrop"),texture);
 gs_effect_set_vec4(gs_effect_get_param_by_name(b->effect,"panel"),rect);
 gs_effect_set_vec4(gs_effect_get_param_by_name(b->effect,"size"),size);
 gs_effect_set_float(gs_effect_get_param_by_name(b->effect,"visibility"),progress);
 gs_effect_set_float(gs_effect_get_param_by_name(b->effect,"brightness"),b->brightness);
 gs_effect_set_float(gs_effect_get_param_by_name(b->effect,"tintOpacity"),b->tintOpacity);
 gs_effect_set_float(gs_effect_get_param_by_name(b->effect,"fillTop"),fillTop);
 gs_effect_set_float(gs_effect_get_param_by_name(b->effect,"radius"),std::min({b->cornerRadius,rect[2]*.5f,rect[3]*.5f}));
 gs_effect_set_float(gs_effect_get_param_by_name(b->effect,"leftRadius"),0.f);
 // The composite shader returns premultiplied RGBA; preserve its edge colors.
 gs_blend_state_push();gs_blend_function(1,5);
 obs_source_process_filter_end(b->filter,b->effect,b->w,b->h);
 gs_blend_state_pop();
}
static bool loadBackdrop(HMODULE lib)
{
#define BLOAD(name) name=reinterpret_cast<decltype(name)>(GetProcAddress(lib,#name));if(!name)return false;
 BLOAD(obs_register_source_s) BLOAD(obs_source_create_private) BLOAD(obs_source_get_filter_by_name) BLOAD(obs_source_filter_add) BLOAD(obs_source_filter_remove) BLOAD(obs_source_enum_filters) BLOAD(obs_source_inc_showing) BLOAD(obs_source_dec_showing) BLOAD(obs_source_get_weak_source) BLOAD(obs_weak_source_get_source) BLOAD(obs_weak_source_release) BLOAD(obs_filter_get_parent) BLOAD(obs_source_skip_video_filter) BLOAD(obs_source_process_filter_begin) BLOAD(obs_source_process_filter_end) BLOAD(obs_data_set_double) BLOAD(obs_data_has_user_value) BLOAD(obs_group_from_source) BLOAD(obs_sceneitem_get_draw_transform) BLOAD(obs_sceneitem_visible) BLOAD(matrix4_mul) BLOAD(matrix4_inv) BLOAD(gs_matrix_push) BLOAD(gs_matrix_pop) BLOAD(gs_matrix_mul) BLOAD(gs_texrender_reset) BLOAD(gs_effect_create_from_file) BLOAD(gs_effect_destroy) BLOAD(gs_effect_get_param_by_name) BLOAD(gs_effect_set_texture) BLOAD(gs_effect_set_float) BLOAD(gs_effect_set_vec4)
 BLOAD(gs_texture_create) BLOAD(gs_texture_destroy) BLOAD(gs_texture_get_width) BLOAD(gs_texture_get_height) BLOAD(gs_draw_sprite) BLOAD(obs_get_base_effect) BLOAD(gs_effect_loop) BLOAD(gs_matrix_translate3f) BLOAD(gs_blend_function)
#undef BLOAD
 SourceInfo raw{};raw.id="omni_mc_background_region";raw.type=0;raw.flags=1|8|1024;raw.name=rawName;raw.create=rawContextCreate;raw.destroy=rawDestroy;raw.width=rawWidth;raw.height=rawHeight;raw.render=rawRender;obs_register_source_s(&raw,sizeof(raw));
 SourceInfo filter{};filter.id="omni_mc_backdrop";filter.type=1;filter.flags=1|8;filter.name=backdropName;filter.create=backdropCreate;filter.destroy=backdropDestroy;filter.tick=backdropTick;filter.render=backdropRender;obs_register_source_s(&filter,sizeof(filter));
 return true;
}
static void attachBackdropSource(P player){P filter=obs_source_get_filter_by_name(player,"Omni MC \u2014 \u0424\u043e\u043d (Composite Blur)");if(filter)obs_source_release(filter);else{filter=obs_source_create("omni_mc_backdrop","Omni MC \u2014 \u0424\u043e\u043d (Composite Blur)",nullptr,nullptr);if(filter){obs_source_filter_add(player,filter);obs_source_release(filter);}}}
static bool validRgbColor(const std::string&value)
{
 if(value.size()!=7||value[0]!='#')return false;for(size_t i=1;i<value.size();++i)if(!((value[i]>='0'&&value[i]<='9')||(value[i]>='a'&&value[i]<='f')||(value[i]>='A'&&value[i]<='F')))return false;return true;
}
static unsigned long long shadowColor(const std::string&value)
{
 if(!validRgbColor(value))return 0xff111111;
 try{unsigned rgb=std::stoul(value.substr(1),nullptr,16);return 0xff000000ull|((rgb>>16)&0xff)|(rgb&0xff00)|((rgb&0xff)<<16);}catch(...){return 0xff111111;}
}
static void syncShadowSource(P player,const std::shared_ptr<Profile>&profile)
{
 if(!player||!profile)return;std::string cfg,coverColor;{std::lock_guard lock(mutex);cfg=profile->cfg;coverColor=profile->shadowCoverColor;}P data=obs_data_create_from_json(cfg.c_str());bool enabled=obs_data_get_bool(data,"shadow");
 P filter=obs_source_get_filter_by_name(player,"Omni MC \u2014 \u0422\u0435\u043d\u044c (Shadow)");
 if(!enabled){if(filter){obs_source_filter_remove(player,filter);obs_source_release(filter);}obs_data_release(data);return;}
 if(!filter){filter=obs_source_get_filter_by_name(player,"Shadow");if(filter&&std::string(obs_source_get_id(filter))=="obs_shadow_filter")obs_source_set_name(filter,"Omni MC \u2014 \u0422\u0435\u043d\u044c (Shadow)");else if(filter){obs_source_release(filter);filter=nullptr;}}
 std::string selectedColor=obs_data_get_bool(data,"coverColors")&&validRgbColor(coverColor)?coverColor:obs_data_get_string(data,"shadowColor");P parameters=obs_data_create();obs_data_set_int(parameters,"glow_position",std::clamp<long long>(obs_data_get_int(data,"shadowPosition")?obs_data_get_int(data,"shadowPosition"):1,1,2));obs_data_set_int(parameters,"blur_type",std::clamp<long long>(obs_data_get_int(data,"shadowBlurType")?obs_data_get_int(data,"shadowBlurType"):2,1,2));obs_data_set_bool(parameters,"fill",obs_data_get_bool(data,"shadowFill"));obs_data_set_bool(parameters,"ignore_source_border",!obs_data_has_user_value(data,"shadowIgnoreBorder")||obs_data_get_bool(data,"shadowIgnoreBorder"));obs_data_set_double(parameters,"glow_size",std::clamp(obs_data_get_double(data,"shadowSize"),0.0,100.0));obs_data_set_double(parameters,"glow_intensity",std::clamp(obs_data_get_double(data,"shadowIntensity"),0.0,200.0));obs_data_set_double(parameters,"glow_offset_angle",std::clamp(obs_data_get_double(data,"shadowAngle"),-180.0,180.0));obs_data_set_double(parameters,"glow_offset_distance",std::clamp(obs_data_get_double(data,"shadowDistance"),0.0,50.0));obs_data_set_int(parameters,"glow_fill_type",std::clamp<long long>(obs_data_get_int(data,"shadowFillType")?obs_data_get_int(data,"shadowFillType"):1,1,2));obs_data_set_int(parameters,"glow_fill_color",(long long)shadowColor(selectedColor));obs_data_set_string(parameters,"glow_fill_source",obs_data_get_string(data,"shadowFillSource"));
 if(filter)obs_source_update(filter,parameters);else{filter=obs_source_create("obs_shadow_filter","Omni MC \u2014 \u0422\u0435\u043d\u044c (Shadow)",parameters,nullptr);if(filter){obs_source_filter_add(player,filter);blog(200,"[omni-mc] Shadow filter attached");}else blog(300,"[omni-mc] Shadow filter plugin is not available");}
 if(filter)obs_source_release(filter);obs_data_release(parameters);obs_data_release(data);
}
static void syncProfileShadow(P pointer){std::unique_ptr<std::string> id(static_cast<std::string*>(pointer));if(!running)return;std::shared_ptr<Profile> profile;{std::lock_guard lock(mutex);profile=profileLocked(*id);}if(!profile)return;P source=obs_get_source_by_name(playerName(profile->id).c_str());if(source){syncShadowSource(source,profile);obs_source_release(source);}}
static bool attachPlayerFilter(P,P source){if(std::string(obs_source_get_id(source))=="browser_source"&&std::string(obs_source_get_name(source)).rfind("Omni MC",0)==0){attachBackdropSource(source);P data=obs_source_get_settings(source);std::string id=obs_data_get_string(data,"omni_mc_context");obs_data_release(data);std::shared_ptr<Profile> profile;{std::lock_guard lock(mutex);profile=profileLocked(id);}if(profile)syncShadowSource(source,profile);}return true;}
static void attachBackdrop(){obs_enum_sources(attachPlayerFilter,nullptr);}
