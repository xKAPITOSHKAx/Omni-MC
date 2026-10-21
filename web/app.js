'use strict';
const initialSessionSelect=document.getElementById('session');initialSessionSelect.replaceChildren(new Option('Ничего не выбрано','@none',true,true),new Option('Автоматически',''));
document.getElementById('showSeconds').value='1';
const skinSelect=document.getElementById('skin'),defaultSkinOption=[...skinSelect.options].find(option=>option.value==='compact');defaultSkinOption.textContent='1. По умолчанию';skinSelect.add(new Option('2. Компактный','compact-bar'),1);
const $=id=>document.getElementById(id),ids=['showTrackCover','coverScale','coverColors','widgetWidthScale','widgetHeightScale','cornerRadius','showPlayerButtons','adaptiveSize','progressThickness','progressVisible','progressAnimation','fontFamily','fontWeight','session','skin','blur','backgroundSource','blurStrength','shadow','shadowPosition','shadowBlurType','shadowFill','shadowIgnoreBorder','shadowSize','shadowIntensity','shadowAngle','shadowDistance','shadowFillType','shadowColor','shadowFillSource','backgroundColor','opacity','brightness','barColor','titleScale','artistScale','auto','hideIfIdle','showSeconds','direction'];
const setText=(id,value)=>{const e=$(id);if(e.textContent!==value)e.textContent=value;};
const styleValues=new WeakMap();
const setStyle=(e,name,value)=>{const text=String(value),values=styleValues.get(e)||{};if(values[name]!==text){e.style[name]=text;values[name]=text;styleValues.set(e,values);}};
let loaded=false,settings={},lastCover='',saving=Promise.resolve(),pending=false,mediaState=null,sessionApps={},sceneContext='';
const defaultCoverScale=skin=>skin==='compact'||skin==='prism'?80:skin==='cassette'?50:['cosmos','gargantua'].includes(skin)?110:100;
const defaultProgressThickness=skin=>skin==='compact-bar'?3:5;
const customSelects=new Map();
let openedSelect=null;
function selectOption(select){return select.options[select.selectedIndex]||select.options[0];}
function positionSelectMenu(component){
 const rect=component.button.getBoundingClientRect(),gap=6,edge=10,below=innerHeight-rect.bottom-gap,above=rect.top-gap;
 const menuHeight=Math.min(component.menu.scrollHeight,280),openAbove=below<Math.min(menuHeight,180)&&above>below;
 component.menu.classList.toggle('select-menu-above',openAbove);
 component.menu.style.left=Math.max(edge,Math.min(rect.left,innerWidth-rect.width-edge))+'px';
 component.menu.style.width=Math.min(rect.width,innerWidth-edge*2)+'px';
 component.menu.style.maxHeight=Math.max(96,Math.min(280,openAbove?above-edge:below-edge))+'px';
 component.menu.style.top=(openAbove?Math.max(edge,rect.top-gap-Math.min(menuHeight,above-edge)):rect.bottom+gap)+'px';
}
function closeSelect(focus=false){
 if(!openedSelect)return;
 const component=openedSelect;openedSelect=null;
 component.shell.classList.remove('is-open');component.button.setAttribute('aria-expanded','false');component.menu.hidden=true;
 if(focus)component.button.focus();
}
function setActiveOption(component,index,scroll=true){
 const items=[...component.menu.querySelectorAll('.select-option:not(:disabled):not([hidden])')];if(!items.length)return;
 const current=items.findIndex(item=>Number(item.dataset.index)===index),next=items[current<0?0:current];
 for(const item of items)item.classList.toggle('is-active',item===next);
 component.active=Number(next.dataset.index);if(scroll)next.scrollIntoView({block:'nearest'});
}
function renderSelectOptions(component){
 const {select,menu}=component;menu.replaceChildren();
 if(select.id==='fontFamily'){const search=document.createElement('input');search.type='search';search.className='select-search';search.placeholder='Поиск шрифта';search.setAttribute('aria-label','Поиск шрифта');search.oninput=()=>{const query=search.value.trim().toLocaleLowerCase('ru');for(const item of menu.querySelectorAll('.select-option'))item.hidden=!!query&&!item.textContent.toLocaleLowerCase('ru').includes(query);const first=menu.querySelector('.select-option:not(:disabled):not([hidden])');for(const item of menu.querySelectorAll('.select-option'))item.classList.remove('is-active');if(first){first.classList.add('is-active');component.active=Number(first.dataset.index);}};menu.append(search);}
 [...select.options].forEach((option,index)=>{const item=document.createElement('button');item.type='button';item.className='select-option';item.dataset.index=index;item.textContent=option.text;if(select.id==='fontFamily'&&option.value)item.style.fontFamily=option.value;item.disabled=option.disabled;item.setAttribute('role','option');item.setAttribute('aria-selected',String(index===select.selectedIndex));if(index===select.selectedIndex)item.classList.add('is-selected','is-active');menu.append(item);});
 component.active=Math.max(0,select.selectedIndex);
}
function syncCustomSelect(select){
 const component=customSelects.get(select);if(!component)return;
 const option=selectOption(select);component.value.textContent=option?.text||'Выберите значение';component.value.style.fontFamily=select.id==='fontFamily'&&option?.value?option.value:'';component.button.disabled=select.disabled;
 if(openedSelect===component){renderSelectOptions(component);positionSelectMenu(component);}
}
function syncCustomSelects(){for(const select of customSelects.keys())syncCustomSelect(select);}
function openCustomSelect(component){
 if(openedSelect&&openedSelect!==component)closeSelect();
 openedSelect=component;renderSelectOptions(component);component.menu.hidden=false;component.shell.classList.add('is-open');component.button.setAttribute('aria-expanded','true');positionSelectMenu(component);setActiveOption(component,component.select.selectedIndex);
 if(component.select.id==='fontFamily')component.menu.querySelector('.select-search')?.focus();
}
function enhanceSelect(select,index){
 const shell=document.createElement('span');shell.className='custom-select custom-select-'+select.id;
 const button=document.createElement('button');button.type='button';button.className='select-button';button.setAttribute('aria-haspopup','listbox');button.setAttribute('aria-expanded','false');button.setAttribute('aria-controls','select-menu-'+index);
 const value=document.createElement('span');value.className='select-value';button.append(value);
 const menu=document.createElement('span');menu.id='select-menu-'+index;menu.className='select-menu';menu.setAttribute('role','listbox');menu.hidden=true;document.body.append(menu);
 select.parentNode.insertBefore(shell,select);shell.append(select,button);select.classList.add('native-select');select.tabIndex=-1;
 const component={select,shell,button,value,menu,active:0};customSelects.set(select,component);syncCustomSelect(select);
 button.onclick=event=>{event.preventDefault();openedSelect===component?closeSelect(true):openCustomSelect(component);};
 button.onkeydown=event=>{
  const keys=['ArrowDown','ArrowUp','Home','End','Enter',' ','Escape'];if(!keys.includes(event.key))return;event.preventDefault();
  if(event.key==='Escape'){closeSelect(true);return;}if(!openedSelect){openCustomSelect(component);if(event.key==='Enter'||event.key===' ')return;}
  const enabled=[...select.options].map((option,i)=>option.disabled?-1:i).filter(i=>i>=0);let at=enabled.indexOf(component.active);
  if(event.key==='ArrowDown')at=Math.min(enabled.length-1,Math.max(0,at+1));else if(event.key==='ArrowUp')at=Math.max(0,at<0?enabled.length-1:at-1);else if(event.key==='Home')at=0;else if(event.key==='End')at=enabled.length-1;else if(event.key==='Enter'||event.key===' '){const item=component.menu.querySelector('.select-option.is-active');if(item)item.click();return;}setActiveOption(component,enabled[at]);
 };
 menu.onclick=event=>{const item=event.target.closest('.select-option');if(!item||item.disabled)return;select.selectedIndex=Number(item.dataset.index);syncCustomSelect(select);select.dispatchEvent(new Event('change',{bubbles:true}));closeSelect(true);};
 menu.onmousemove=event=>{const item=event.target.closest('.select-option');if(item&&!item.disabled)setActiveOption(component,Number(item.dataset.index),false);};
 select.addEventListener('change',()=>syncCustomSelect(select));select.addEventListener('focus',()=>button.focus());
 new MutationObserver(()=>syncCustomSelect(select)).observe(select,{childList:true,subtree:true,attributes:true});
}
$('fontWeight').options[0].textContent='Как в шаблоне';
for(const fold of [
 {title:'ФОН ПЛЕЕРА',label:'ФОН ПЛЕЕРА',key:'background'},
 {title:'НАСТРОЙКА СКИНА',label:'НАСТРОЙКА ШАБЛОНА',key:'template'},
 {title:'АВТОПОКАЗ ПЛЕЕРА',label:'АВТОПОКАЗ ПЛЕЕРА',key:'autodisplay'}
]){
 const heading=[...document.querySelectorAll('h2')].find(element=>element.textContent.trim()===fold.title),content=heading?.nextElementSibling;
 if(!heading||!content)continue;
 heading.classList.add('settings-fold-heading','settings-fold-'+fold.key);
 const button=document.createElement('button');button.type='button';button.className='settings-fold-button';button.textContent=fold.label;heading.replaceChildren(button);
 const storageKey='omni-mc-fold-'+fold.key;
 const setOpen=open=>{content.hidden=!open;button.setAttribute('aria-expanded',String(open));heading.classList.toggle('is-open',open);};
 let open=false;try{open=localStorage.getItem(storageKey)==='open';}catch{}
 setOpen(open);button.onclick=()=>{const next=button.getAttribute('aria-expanded')!=='true';setOpen(next);try{localStorage.setItem(storageKey,next?'open':'closed');}catch{}};
}
[...document.querySelectorAll('select')].forEach(enhanceSelect);
async function loadSystemFonts(){try{const response=await fetch('/fonts');if(!response.ok)throw Error();const fonts=await response.json(),select=$('fontFamily'),value=select.value;select.replaceChildren(new Option('Системный шрифт',''));for(const font of fonts)select.add(new Option(font,font));if(value&&!fonts.includes(value))select.add(new Option(value+' (недоступен)',value));select.value=value;syncCustomSelect(select);}catch{$('message').textContent='Не удалось получить список шрифтов Windows.';}}
loadSystemFonts();
document.addEventListener('pointerdown',event=>{if(openedSelect&&!openedSelect.shell.contains(event.target)&&!openedSelect.menu.contains(event.target))closeSelect();});
addEventListener('resize',()=>{if(openedSelect)positionSelectMenu(openedSelect);});addEventListener('scroll',()=>{if(openedSelect)positionSelectMenu(openedSelect);},true);
function setCompactPanel(compact){
 document.body.classList.toggle('compact-panel',compact);
 const button=$('compactPanel');
 button.setAttribute('aria-pressed',String(compact));
 button.title=compact?'Показать настройки':'Скрыть настройки';
 button.setAttribute('aria-label',button.title);
}
try{setCompactPanel(localStorage.getItem('omni-mc-compact-panel')==='true');}catch{setCompactPanel(false);}
$('compactPanel').addEventListener('click',()=>{
 const compact=!document.body.classList.contains('compact-panel');
 setCompactPanel(compact);
 try{localStorage.setItem('omni-mc-compact-panel',String(compact));}catch{}
 window.scrollTo(0,0);
});
let interfaceAccent='#ef4b6b';
function applyInterfaceColor(value){
 if(!/^#[0-9a-f]{6}$/i.test(value))return;
 interfaceAccent=value;
 const rgb=[1,3,5].map(i=>parseInt(value.slice(i,i+2),16));
 const mix=(target,factor)=>'#'+rgb.map(c=>Math.round(c+(target-c)*factor).toString(16).padStart(2,'0')).join('');
 const root=document.documentElement;
 root.style.setProperty('--interface-accent',value);
 root.style.setProperty('--interface-light',mix(255,.35));
 root.style.setProperty('--interface-dark',mix(0,.25));
 root.style.setProperty('--interface-ink',rgb[0]*.299+rgb[1]*.587+rgb[2]*.114>160?'#171a20':'#ffffff');
 $('interfaceColor').value=value;
}
try{applyInterfaceColor(localStorage.getItem('omni-mc-interface-color')||interfaceAccent);}catch{applyInterfaceColor(interfaceAccent);}
$('interfaceColor').addEventListener('input',event=>{applyInterfaceColor(event.target.value);try{localStorage.setItem('omni-mc-interface-color',interfaceAccent);}catch{}});
let draggingSeek=null,previewSeek=null;
let volumeTimer,volumePreview=null,volumeDragging=false,volumeSaving=Promise.resolve();
function renderVolume(value,muted){$('volume').value=value;$('volume').style.setProperty('--fill',value+'%');document.querySelector('output[for="volume"]').value=value+'%';$('mute').setAttribute('aria-pressed',String(!!muted));$('mute').title=muted?'Включить звук':'Выключить звук';$('mute').setAttribute('aria-label',$('mute').title);}
function updateVolume(media,context){const available=!!media.volumeAvailable;$('volume').disabled=!available;$('mute').disabled=!available;$('volume').title=available?'Громкость выбранного приложения':'Звук выбранного приложения пока недоступен';if(volumePreview&&(volumePreview.session!==media.session||volumePreview.context!==context||Date.now()>volumePreview.expires))volumePreview=null;if(!volumeDragging||!available)renderVolume(volumePreview?.volume??media.volume??100,volumePreview?.muted??media.muted??false);}
function sendVolume(body,context){volumeSaving=volumeSaving.catch(()=>{}).then(()=>post('/volume',body,context)).catch(()=>{volumePreview=null;$('message').textContent='Не удалось изменить громкость приложения.';});}
$('volume').addEventListener('pointerdown',()=>{volumeDragging=true;});
for(const event of ['pointerup','pointercancel','lostpointercapture'])$('volume').addEventListener(event,()=>{volumeDragging=false;});
$('volume').addEventListener('input',()=>{if(!mediaState?.volumeAvailable)return;const volume=Number($('volume').value),context=sceneContext,session=mediaState.session;volumePreview={volume,muted:volumePreview?.muted??mediaState.muted,session,context,expires:Date.now()+2500};renderVolume(volume,volumePreview.muted);clearTimeout(volumeTimer);volumeTimer=setTimeout(()=>sendVolume({session,volume},context),100);});
$('mute').onclick=()=>{if(!mediaState?.volumeAvailable)return;const muted=!(volumePreview?.muted??mediaState.muted),volume=Number($('volume').value);volumePreview={muted,volume,session:mediaState.session,context:sceneContext,expires:Date.now()+2500};renderVolume(volume,muted);sendVolume({session:mediaState.session,muted},sceneContext);};
const visualProgress=MusicTimeline.createProgress();
const progressEffects=MusicTimeline.createEffects($("progress"));
const seekBar=$('progress').parentElement;
seekBar.tabIndex=0;seekBar.setAttribute('role','slider');seekBar.setAttribute('aria-label','Перемотка трека');
function seekPoint(event){const rect=seekBar.getBoundingClientRect();return Math.max(0,Math.min(1,(event.clientX-rect.left)/rect.width))*draggingSeek.duration;}
function commitSeek(seek){if(seek.context!==sceneContext||seek.key!==mediaState?.coverKey)return;previewSeek={...seek,expires:Date.now()+2500};post('/seek',{positionMs:Math.round(seek.position*1000),coverKey:seek.key},seek.context).catch(()=>{previewSeek=null;$('message').textContent='Не удалось перемотать трек.';});}
seekBar.addEventListener('pointerdown',event=>{if(event.button!==0||!mediaState?.duration||!mediaState.coverKey)return;event.preventDefault();draggingSeek={context:sceneContext,key:mediaState.coverKey,duration:mediaState.duration,position:0,pointer:event.pointerId};draggingSeek.position=seekPoint(event);seekBar.setPointerCapture(event.pointerId);seekBar.classList.add('seeking');});
seekBar.addEventListener('pointermove',event=>{if(draggingSeek?.pointer===event.pointerId)draggingSeek.position=seekPoint(event);});
seekBar.addEventListener('pointerup',event=>{if(draggingSeek?.pointer!==event.pointerId)return;draggingSeek.position=seekPoint(event);const seek=draggingSeek;draggingSeek=null;seekBar.classList.remove('seeking');commitSeek(seek);});
for(const event of ['pointercancel','lostpointercapture'])seekBar.addEventListener(event,()=>{draggingSeek=null;seekBar.classList.remove('seeking');});
seekBar.addEventListener('keydown',event=>{if(!mediaState?.duration||!mediaState.coverKey||!['ArrowLeft','ArrowRight','Home','End'].includes(event.key))return;event.preventDefault();const current=previewSeek?.position??MusicTimeline.position(mediaState),position=event.key==='Home'?0:event.key==='End'?mediaState.duration:Math.max(0,Math.min(mediaState.duration,current+(event.key==='ArrowRight'?5:-5)));commitSeek({context:sceneContext,key:mediaState.coverKey,position,duration:mediaState.duration});});
async function post(path,body,context=sceneContext){const r=await fetch(path+(context?'?context='+encodeURIComponent(context):''),{method:'POST',headers:{'Content-Type':'application/json'},body:body?JSON.stringify(body):'{}'});if(!r.ok)throw Error('Ошибка '+r.status);return r;}
function updatePlayerButtonsSetting(){const skin=$('skin').value,coverOnly=skin==='cover',coverless=skin==='compact-bar';$('trackCoverSetting').hidden=coverOnly||coverless;$('showTrackCover').disabled=coverOnly||coverless;const scalableCover=['compact','cover','prism','cassette','neon','vector','pulse','cosmos','gargantua'].includes(skin);$('coverScaleSetting').hidden=!scalableCover;$('coverScale').disabled=!scalableCover;const supported=['poster','cinematic','split','orbit','cassette','prism','eclipse','neon'].includes(skin);$('playerButtonsSetting').hidden=!supported;$('showPlayerButtons').disabled=!supported;for(const id of ['backgroundColor','barColor','shadowColor']){$(id).disabled=!!settings.coverColors;$(id).closest('label').classList.toggle('color-unavailable',!!settings.coverColors);}}
function updateShadowSettings(){const enabled=$('shadow').checked,inner=$('shadowPosition').value==='2',source=$('shadowFillType').value==='2';$('shadowControls').classList.toggle('is-disabled',!enabled);for(const element of $('shadowControls').querySelectorAll('input,select,button'))element.disabled=!enabled;$('shadowColor').disabled=!enabled||!!settings.coverColors;$('shadowColorSetting').classList.toggle('color-unavailable',!!settings.coverColors);$('shadowFillSetting').hidden=inner;$('shadowIgnoreBorderSetting').hidden=!inner;$('shadowColorSetting').hidden=source;$('shadowFillSourceSetting').hidden=!source;syncCustomSelects();}
function outputs(){for(const id of ['coverScale','widgetWidthScale','widgetHeightScale','cornerRadius','progressThickness','blurStrength','shadowSize','shadowIntensity','shadowAngle','shadowDistance','opacity','brightness','titleScale','artistScale','showSeconds']){const e=$(id),suffix=id==='showSeconds'?' сек.':['cornerRadius','progressThickness','shadowSize','shadowDistance'].includes(id)?' px':id==='shadowAngle'?'°':id==='blurStrength'?'':'%';document.querySelector('output[for="'+id+'"]').value=e.value+suffix;e.style.setProperty('--fill',100*(e.value-e.min)/(e.max-e.min)+'%');}}
function collect(){const s={};for(const id of ids){const e=$(id);s[id]=e.type==='checkbox'?e.checked:e.type==='range'||id==='showSeconds'||['fontWeight','shadowPosition','shadowBlurType','shadowFillType'].includes(id)?Number(e.value):e.value;}return s;}
function save(changedId){const next=collect();if(changedId==='skin'&&next.skin!==settings.skin){clearTimeout(fontSaveTimer);for(const [id,value] of Object.entries({coverScale:defaultCoverScale(next.skin),widgetWidthScale:100,widgetHeightScale:100,titleScale:100,artistScale:100,progressThickness:defaultProgressThickness(next.skin)})){next[id]=value;$(id).value=value;}}for(const id of ["progressVisible","adaptiveSize","showPlayerButtons","coverColors","showTrackCover"]){if(changedId!==id&&typeof settings[id]==="boolean"){next[id]=settings[id];$(id).checked=settings[id];}}settings=next;updatePlayerButtonsSetting();updateShadowSettings();const snapshot={...settings},context=sceneContext;saving=saving.catch(()=>{}).then(()=>post('/settings',snapshot,context)).catch(()=>{$('message').textContent='Не удалось сохранить настройки. Проверьте, что OBS работает.';});outputs();}
function applicationName(id){
 if(!id)return '—';
 const resolved=sessionApps[id]||id,key=resolved.toLowerCase();
 if(key.includes('yandex')&&key.includes('music'))return 'Яндекс Музыка';
 if(key.includes('spotify'))return 'Spotify';
 if(key==='chrome'||key.includes('chrome.exe'))return 'Google Chrome';
 if(key==='msedge'||key.includes('msedge.exe'))return 'Microsoft Edge';
 if(key.includes('firefox'))return 'Mozilla Firefox';
 if(key.includes('zunemusic')||key.includes('mediaplayer')||key==='wmplayer.exe')return 'Проигрыватель Windows';
 return resolved;
}
function options(e,values,first){const background=e.id==='backgroundSource',session=e.id==='session',fixed='@track-cover',label=session?applicationName:v=>background&&v===fixed?'Обложка трека':v;const value=e.value;const signature=JSON.stringify(values);if(e.dataset.options===signature)return;e.dataset.options=signature;e.replaceChildren(...(background?[new Option('Обложка трека',fixed),new Option(first,'')]:session?[new Option('Ничего не выбрано','@none'),new Option('Автоматически','')]:[new Option(first,'')]));for(const v of values)if(!background||v!==fixed)e.add(new Option(label(v),v));if(value&&!values.includes(value)&&value!=='@none'&&!(background&&value===fixed))e.add(new Option(label(value)+' (недоступен)',value));e.value=value;}
const time=n=>Math.floor(n/60)+':'+String(Math.floor(n%60)).padStart(2,'0');
async function update(){if(pending)return;pending=true;try{const r=await fetch('/state');if(!r.ok)throw Error();const data=await r.json(),m=data.media;mediaState=m;sessionApps=m.sessionApps||{};updateVolume(m,data.context||'');if(data.context!==sceneContext){sceneContext=data.context||'';lastCover='';loaded=false;clearTimeout(fontSaveTimer);}if(!loaded){settings={showTrackCover:true,coverScale:defaultCoverScale(data.settings?.skin),coverColors:false,widgetWidthScale:100,widgetHeightScale:100,cornerRadius:25,showPlayerButtons:true,adaptiveSize:true,progressVisible:true,progressThickness:defaultProgressThickness(data.settings?.skin),progressAnimation:"smooth",shadow:false,shadowPosition:1,shadowBlurType:2,shadowFill:false,shadowIgnoreBorder:true,shadowSize:30,shadowIntensity:100,shadowAngle:0,shadowDistance:30,shadowFillType:1,shadowColor:'#111111',shadowFillSource:'',titleScale:100,artistScale:100,...data.settings};const removedSkin=!settings.skin||settings.skin==='overlay'||settings.skin==='spectrum';if(removedSkin)settings.skin='compact';for(const id of ids){const e=$(id);if(e.type==='checkbox')e.checked=!!settings[id];else if(settings[id]!==undefined){if(e.tagName==='SELECT'&&settings[id]&&![...e.options].some(o=>o.value===String(settings[id])))e.add(new Option(settings[id],settings[id]));e.value=settings[id];}}syncCustomSelects();loaded=true;outputs();updatePlayerButtonsSetting();updateShadowSettings();if(removedSkin)save();}options($('session'),m.sessions||[],'Автоматически');$('artist').textContent=m.artist||'ИСПОЛНИТЕЛЬ';$('title').textContent=m.title||'Нет названия';$('application').textContent='Приложение: '+applicationName(m.session);$('status').textContent=m.playing?'\u0418\u0413\u0420\u0410\u0415\u0422':'\u041d\u0415 \u0418\u0413\u0420\u0410\u0415\u0422';$('status').className='badge '+(m.playing?'playing':'not-playing');$('play').dataset.playing=String(!!m.playing);$('play').title=m.playing?'\u041f\u0430\u0443\u0437\u0430':'\u0412\u043e\u0441\u043f\u0440\u043e\u0438\u0437\u0432\u0435\u0441\u0442\u0438';$('play').setAttribute('aria-label',$('play').title);const coverToken=m.coverKey+'|'+(m.coverRevision||0);if(lastCover!==coverToken){lastCover=coverToken;$('cover').style.display='none';$('noCover').style.display='block';if(m.coverKey)$('cover').src='/cover?key='+encodeURIComponent(m.coverKey)+'&revision='+(m.coverRevision||0)+(sceneContext?'&context='+encodeURIComponent(sceneContext):'');}if(m.error)$('sessionHint').textContent=m.error;else $('sessionHint').textContent=m.sessions?.length?'Выберите приложение, из которого играет музыка.':'Запустите музыку в приложении, чтобы оно появилось в списке.';}catch{$('message').textContent='Нет соединения с плагином. Запустите OBS с установленным Omni MC.';}finally{pending=false;}}
$('cover').onload=()=>{$('cover').style.display='block';$('noCover').style.display='none';};
for(const id of ids)$(id).addEventListener('change',()=>{const element=$(id);if(element.checked&&id==='auto')$('hideIfIdle').checked=false;else if(element.checked&&id==='hideIfIdle')$('auto').checked=false;if(loaded)save(id);});for(const id of ['widgetWidthScale','widgetHeightScale','cornerRadius','progressThickness','blurStrength','shadowSize','shadowIntensity','shadowAngle','shadowDistance','opacity','brightness','titleScale','artistScale','showSeconds'])$(id).addEventListener('input',outputs);
const helpIcon=document.querySelector('.help-icon'),helpTooltip=$('showSecondsHelp'),showHelp=()=>helpTooltip.classList.add('is-visible'),hideHelp=()=>helpTooltip.classList.remove('is-visible');helpIcon.addEventListener('pointerenter',showHelp);helpIcon.addEventListener('pointerleave',hideHelp);helpIcon.addEventListener('focus',showHelp);helpIcon.addEventListener('blur',hideHelp);
for(const b of document.querySelectorAll('[data-command]'))b.onclick=()=>post('/'+b.dataset.command).catch(()=>{$('message').textContent='Команда управления не отправлена.';});
$('add').onclick=async()=>{const context=sceneContext;try{await saving;await post('/add',null,context);$('message').textContent='Команда добавления отправлена. Источник «Omni MC — Плеер» появится в текущей сцене OBS.';}catch{$('message').textContent='Не удалось добавить плеер.';}};
$('test').onclick=()=>post('/test').catch(()=>{});
$('reset').onclick=()=>{for(const [id,value] of Object.entries({coverScale:defaultCoverScale($('skin').value),widgetWidthScale:100,widgetHeightScale:100,cornerRadius:25,progressThickness:defaultProgressThickness($('skin').value),backgroundColor:'#285154',opacity:42,brightness:96,barColor:'#ed4169',fontFamily:'',fontWeight:0,titleScale:100,artistScale:100})){if(settings.coverColors&&['backgroundColor','barColor'].includes(id))continue;$(id).value=value;}syncCustomSelects();save();};
async function sources(){try{const r=await fetch('/sources'),values=await r.json();options($('backgroundSource'),values,'Прозрачный фон');options($('shadowFillSource'),values,'Не выбран');}catch{}}
update();sources();setInterval(update,1000);setInterval(sources,3000);

function animateProgress(){if(mediaState){const m=mediaState;let n=MusicTimeline.position(m);if(previewSeek&&(previewSeek.context!==sceneContext||previewSeek.key!==m.coverKey||Date.now()>previewSeek.expires||Math.abs(n-previewSeek.position)<1))previewSeek=null;const seek=draggingSeek||previewSeek;if(seek&&seek.context===sceneContext&&seek.key===m.coverKey)n=seek.position;const trackProgress=visualProgress({...m,position:n,updated:Date.now()},Date.now(),{scope:sceneContext,immediate:!!seek,mode:settings.progressAnimation});seekBar.setAttribute('aria-valuemin','0');seekBar.setAttribute('aria-valuemax',String(m.duration||0));seekBar.setAttribute('aria-valuenow',String(Math.round(n)));seekBar.style.setProperty('--seek-position',(100*trackProgress)+'%');setText('position',m.duration?MusicTimeline.time(n):'—');setText('duration',m.duration?MusicTimeline.time(m.duration):'—');setStyle($('progress'),'width',(100*trackProgress)+'%');progressEffects(visualProgress.effect,interfaceAccent);}requestAnimationFrame(animateProgress);}animateProgress();

let fontSaveTimer;
for(const id of ['coverScale','widgetWidthScale','widgetHeightScale','cornerRadius','titleScale','artistScale','shadowSize','shadowIntensity','shadowAngle','shadowDistance'])$(id).addEventListener('input',()=>{clearTimeout(fontSaveTimer);fontSaveTimer=setTimeout(()=>{if(loaded)save();},150);});

const volumePanelElement=document.querySelector('.preview .volume-control'),volumeToggleButton=document.createElement('button');
volumePanelElement.id='volumePanel';volumePanelElement.hidden=true;volumePanelElement.setAttribute('aria-hidden','true');
volumeToggleButton.id='volumeToggle';volumeToggleButton.type='button';volumeToggleButton.className='track-button volume-toggle';volumeToggleButton.title='\u0413\u0440\u043e\u043c\u043a\u043e\u0441\u0442\u044c';volumeToggleButton.setAttribute('aria-label',volumeToggleButton.title);volumeToggleButton.setAttribute('aria-expanded','false');volumeToggleButton.setAttribute('aria-controls','volumePanel');volumeToggleButton.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true"><path class="volume-body" d="M4 9h4l5-4v14l-5-4H4z"/><path class="sound-waves" d="M17 8a6 6 0 0 1 0 8m3-11a10 10 0 0 1 0 14"/><path class="sound-muted" d="m17 9 5 6m0-6-5 6"/></svg>';
document.querySelector('.preview .controls').append(volumeToggleButton);
let volumePanelCloseTimer;
function setVolumePanel(open){clearTimeout(volumePanelCloseTimer);volumeToggleButton.setAttribute('aria-expanded',String(open));volumePanelElement.setAttribute('aria-hidden',String(!open));if(open){volumePanelElement.hidden=false;requestAnimationFrame(()=>volumePanelElement.classList.add('is-open'));}else{volumePanelElement.classList.remove('is-open');volumePanelCloseTimer=setTimeout(()=>{if(volumeToggleButton.getAttribute('aria-expanded')==='false')volumePanelElement.hidden=true;},180);}}
volumeToggleButton.onclick=()=>setVolumePanel(volumeToggleButton.getAttribute('aria-expanded')!=='true');
const renderVolumeBase=renderVolume;renderVolume=function(value,muted){renderVolumeBase(value,muted);volumeToggleButton.classList.toggle('is-muted',!!muted);volumeToggleButton.disabled=$('volume').disabled;};
