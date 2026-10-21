'use strict';
const $=id=>document.getElementById(id);
const query=new URLSearchParams(location.search),contextId=query.get('context')||'',contextQuery=contextId?'?context='+encodeURIComponent(contextId):'',sourcePadding=query.get('padding')==='50'?50:0;
const defaultCoverScale=skin=>skin==='compact'||skin==='prism'?80:skin==='cassette'?50:['cosmos','gargantua'].includes(skin)?110:100;
const defaultProgressThickness=skin=>skin==='compact-bar'?3:5;
let key='',busy=false,state=null;
let measuredLayout='';
function ensureCosmosStars(player){let field=player.querySelector('.cosmos-star-field');if(field)return;field=document.createElement('div');field.className='cosmos-star-field';for(let i=0;i<58;i++){const star=document.createElement('i'),size=.7+Math.random()*2.5;star.style.setProperty('--star-x',(2+Math.random()*96)+'%');star.style.setProperty('--star-y',(3+Math.random()*94)+'%');star.style.setProperty('--star-size',size+'px');star.style.setProperty('--star-opacity',(.24+Math.random()*.7).toFixed(2));star.style.setProperty('--star-duration',(2.2+Math.random()*5.5)+'s');star.style.setProperty('--star-delay',(-Math.random()*7)+'s');star.style.setProperty('--star-drift-x',(-7+Math.random()*14)+'px');star.style.setProperty('--star-drift-y',(-5+Math.random()*10)+'px');if(Math.random()<.18)star.className='accent';field.append(star);}player.prepend(field);}
let gargantuaVortex=null;
function ensureGargantuaVortex(player){
 if(gargantuaVortex?.canvas.isConnected)return gargantuaVortex;
 const canvas=document.createElement('canvas');canvas.className='gargantua-vortex';canvas.setAttribute('aria-hidden','true');player.insertBefore(canvas,player.querySelector('.artwork'));
 const particles=Array.from({length:96},()=>({angle:Math.random()*Math.PI*2,radius:0,depth:.25+Math.random()*.75,sink:16+Math.random()*25,spin:.75+Math.random()*.7,phase:Math.random()*Math.PI*2}));
 gargantuaVortex={canvas,ctx:canvas.getContext('2d'),particles,width:0,height:0,last:0,key:'',burstUntil:0};return gargantuaVortex;
}
function drawGargantuaVortex(player,at,media){
 if(player.className!=='gargantua')return;
 const vortex=ensureGargantuaVortex(player),canvas=vortex.canvas,ctx=vortex.ctx,width=player.clientWidth,height=player.clientHeight,dpr=Math.min(2,devicePixelRatio||1),maxRadius=Math.hypot(width,height)*.82;
 if(!ctx||width<1||height<1)return;
 if(vortex.width!==width||vortex.height!==height){vortex.width=width;vortex.height=height;canvas.width=Math.round(width*dpr);canvas.height=Math.round(height*dpr);canvas.style.width=width+'px';canvas.style.height=height+'px';ctx.setTransform(dpr,0,0,dpr,0,0);vortex.last=at;for(const particle of vortex.particles)particle.radius=maxRadius*(.12+Math.random()*.9);}
 const artwork=document.querySelector('.artwork'),playerRect=player.getBoundingClientRect(),artRect=artwork.getBoundingClientRect(),centerX=artRect.left-playerRect.left+artRect.width/2,centerY=artRect.top-playerRect.top+artRect.height/2;
 const coverKey=media.coverKey||'';if(vortex.key&&coverKey&&coverKey!==vortex.key)vortex.burstUntil=at+1900;vortex.key=coverKey;
 const delta=Math.min(.05,Math.max(0,(at-vortex.last)/1000));vortex.last=at;const burst=Math.max(0,Math.min(1,(vortex.burstUntil-at)/1900)),force=(media.playing?1:.68)+burst*1.35,tilt=-24*Math.PI/180,cos=Math.cos(tilt),sin=Math.sin(tilt),point=particle=>{const x=Math.cos(particle.angle)*particle.radius,y=Math.sin(particle.angle)*particle.radius*.5;return{x:centerX+x*cos-y*sin,y:centerY+x*sin+y*cos};};
 ctx.clearRect(0,0,width,height);ctx.save();ctx.globalCompositeOperation='lighter';ctx.lineCap='round';const accent=player.style.getPropertyValue('--accent')||'#ed4169';
 for(const particle of vortex.particles){
  if(particle.radius<34){particle.radius=maxRadius*(.72+Math.random()*.34);particle.angle=Math.random()*Math.PI*2;particle.depth=.25+Math.random()*.75;particle.sink=16+Math.random()*25;particle.spin=.75+Math.random()*.7;particle.phase=Math.random()*Math.PI*2;}
  const previous=point(particle),pull=1-particle.radius/maxRadius;particle.radius-=particle.sink*(1+pull*pull*5)*delta*force;particle.angle+=particle.spin*(.35+pull*pull*3.2)*delta*force;const current=point(particle),dx=current.x-previous.x,dy=current.y-previous.y,outer=Math.min(1,(maxRadius-particle.radius)/(maxRadius*.18)),inner=Math.min(1,Math.max(0,(particle.radius-34)/72)),twinkle=.72+.28*Math.sin(at*.003+particle.phase),alpha=(.12+particle.depth*.48)*outer*inner*twinkle*(1+burst*.55);
  if(alpha<=.01)continue;const trail=3+particle.depth*7+burst*4,gradient=ctx.createLinearGradient(current.x-dx*trail,current.y-dy*trail,current.x,current.y);gradient.addColorStop(0,'transparent');gradient.addColorStop(.65,particle.depth>.72?'rgba(255,137,43,.32)':'rgba(255,255,255,.18)');gradient.addColorStop(1,particle.depth>.48?accent:'#fff4cf');ctx.globalAlpha=Math.min(1,alpha);ctx.strokeStyle=gradient;ctx.lineWidth=.45+particle.depth*1.35+burst*.45;ctx.shadowColor=particle.depth>.5?accent:'#ffb45a';ctx.shadowBlur=2+particle.depth*5+burst*3;ctx.beginPath();ctx.moveTo(current.x-dx*trail,current.y-dy*trail);ctx.lineTo(current.x,current.y);ctx.stroke();
 }
 ctx.restore();
}
const setText=(id,value)=>{const e=$(id);if(e.textContent!==value)e.textContent=value;};
const styleValues=new WeakMap();
const setStyle=(e,name,value)=>{const text=String(value),values=styleValues.get(e)||{};if(values[name]!==text){e.style[name]=text;values[name]=text;styleValues.set(e,values);}};
let orbitAngle=-Math.PI/2,cosmosOrbitA=-Math.PI*.75,cosmosOrbitB=-Math.PI*.25,orbitFrameAt=0;
let armAngle=-30,armFrom=-30,armAt=0,armDuration=0,armPlaying=null,armKey='',armPosition=0;
function tonearmTrackAngle(position,duration){
 const fraction=duration>0?Math.max(0,Math.min(1,position/duration)):0;
 const radius=112-(112-46)*fraction;
 // Pivot (270,55), spindle (150,150), needle tip after its 35-degree cartridge rotation.
 const tipX=-90.898952,tipY=146.563889,centerX=-120,centerY=95;
 const armLength=Math.hypot(tipX,tipY),pivotDistance=Math.hypot(centerX,centerY);
 const cosine=Math.max(-1,Math.min(1,(armLength*armLength+pivotDistance*pivotDistance-radius*radius)/(2*armLength*pivotDistance)));
 return (Math.atan2(centerY,centerX)-Math.atan2(tipY,tipX)-Math.acos(cosine))*180/Math.PI;
}
function moveTonearm(media,position,at){
 const playing=!!media.playing,target=playing?tonearmTrackAngle(position,media.duration):-30;
 const changed=playing!==armPlaying,newTrack=media.coverKey!==armKey,seek=Math.abs(position-armPosition)>2;
 if(changed||newTrack||seek){armFrom=armAngle;armAt=at;armDuration=changed?(playing?180:650):newTrack?600:250;}
 armPlaying=playing;armKey=media.coverKey;armPosition=position;
 const t=armDuration?Math.max(0,Math.min(1,(at-armAt)/armDuration)):1,ease=t*t*(3-2*t);armAngle=armFrom+(target-armFrom)*ease;
 $('player').style.setProperty('--tonearm-angle',armAngle+'deg');
}
let colorComplete=false;
let coverPalette=null,colorTarget='',colorFrom=null,colorTo=null,colorAt=0,currentColors=null,shadowColorTarget='';
const rgbHex=value=>[1,3,5].map(i=>parseInt(value.slice(i,i+2),16));
const colorHex=rgb=>'#'+rgb.map(c=>Math.round(c).toString(16).padStart(2,'0')).join('');
function coverColors(image){
 try{
  const canvas=document.createElement('canvas');canvas.width=canvas.height=32;const ctx=canvas.getContext('2d',{willReadFrequently:true});ctx.drawImage(image,0,0,32,32);
  const pixels=ctx.getImageData(0,0,32,32).data,buckets=new Map();let sum=[0,0,0],total=0;
  for(let i=0;i<pixels.length;i+=4){if(pixels[i+3]<128)continue;const rgb=[pixels[i],pixels[i+1],pixels[i+2]],max=Math.max(...rgb),min=Math.min(...rgb);sum=sum.map((v,j)=>v+rgb[j]);total++;if(max<30||min>230)continue;const id=rgb.map(c=>c>>4).join(','),weight=1+(max-min)/255*3,b=buckets.get(id)||{weight:0,sum:[0,0,0]};b.weight+=weight;b.sum=b.sum.map((v,j)=>v+rgb[j]*weight);buckets.set(id,b);}
  if(!total)return null;
  const best=[...buckets.values()].sort((a,b)=>b.weight-a.weight)[0],rgb=best?best.sum.map(v=>v/best.weight):sum.map(v=>v/total),normalized=rgb.map(v=>v/255),max=Math.max(...normalized),min=Math.min(...normalized),delta=max-min;
  let hue=0;if(delta){const index=normalized.indexOf(max);hue=(index===0?(normalized[1]-normalized[2])/delta:index===1?(normalized[2]-normalized[0])/delta+2:(normalized[0]-normalized[1])/delta+4)/6;if(hue<0)hue++;}
  const saturation=delta<.06?0:Math.min(.8,Math.max(.45,delta/Math.max(.01,1-Math.abs(max+min-1))));
  const hsl=(sat,light)=>{const a=sat*Math.min(light,1-light);return [0,8,4].map(n=>{const k=(n+hue*12)%12;return 255*(light-a*Math.max(-1,Math.min(k-3,9-k,1)));});};
  return {background:hsl(saturation*.65,.12),accent:hsl(saturation,.66)};
 }catch{return null;}
}
function paintColors(at){
 if(!state)return;const s=state.settings,automatic=!!s.coverColors&&!!coverPalette,background=automatic?coverPalette.background:rgbHex(s.backgroundColor||'#285154'),accent=automatic?coverPalette.accent:rgbHex(s.barColor||'#ed4169'),signature=[colorHex(background),colorHex(accent)].join(':');
 if(signature===colorTarget&&colorComplete)return;
 if(signature!==colorTarget){colorComplete=false;colorTarget=signature;colorFrom=currentColors||{background,accent};colorTo={background,accent};colorAt=at;}
 if(s.coverColors){const nextShadowColor=colorHex(accent);if(nextShadowColor!==shadowColorTarget){shadowColorTarget=nextShadowColor;fetch('/shadow-color'+contextQuery,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({color:nextShadowColor})}).then(response=>{if(!response.ok&&shadowColorTarget===nextShadowColor)shadowColorTarget='';}).catch(()=>{if(shadowColorTarget===nextShadowColor)shadowColorTarget='';});}}else shadowColorTarget='';
 const t=Math.min(1,Math.max(0,(at-colorAt)/650)),ease=t*t*(3-2*t),mix=(a,b)=>a.map((c,i)=>c+(b[i]-c)*ease);currentColors={background:mix(colorFrom.background,colorTo.background),accent:mix(colorFrom.accent,colorTo.accent)};
 const bg=colorHex(currentColors.background),bar=colorHex(currentColors.accent),p=$('player'),tintOpacity=Math.min(100,Math.max(0,Number(s.opacity)||0))/100,compactFilledText=colorHex(currentColors.background.map(value=>value*tintOpacity));p.dataset.coverColors=String(automatic);p.style.setProperty('--accent',bar);p.style.setProperty('--compact-filled-text',compactFilledText);p.style.setProperty('--prism-decoration-opacity',tintOpacity*.13);p.style.setProperty('--button-surface','rgba('+currentColors.accent.map(Math.round).join(',')+',.14)');
 const luminance=currentColors.accent.map(c=>{c/=255;return c<=.04045?c/12.92:Math.pow((c+.055)/1.055,2.4);}).reduce((v,c,i)=>v+c*[.2126,.7152,.0722][i],0);p.style.setProperty('--button-ink',luminance>.179?'#10131b':'#ffffff');$('tint').style.background=bg;$('bar').style.background=bar;colorComplete=t===1;
}
const visualProgress=MusicTimeline.createProgress();
const progressEffects=MusicTimeline.createEffects($("bar"),document.querySelector(".artwork"));
const fmt=n=>Math.floor(n/60)+':'+String(Math.floor(n%60)).padStart(2,'0');
let compactMarqueeKey='',compactMarqueeAt=0;
function compactBarMarquee(player,at){
 const elements=[$('artist'),$('title')];
 if(player.className!=='compact-bar'){compactMarqueeKey='';for(const element of elements)setStyle(element,'transform','');return;}
 const overflow=Math.max(0,elements.reduce((width,element)=>width+element.offsetWidth,0)-player.clientWidth),nextKey=elements.map(element=>element.textContent).join('\u0000')+'\u0000'+player.clientWidth+'\u0000'+Math.round(overflow);
 if(nextKey!==compactMarqueeKey){compactMarqueeKey=nextKey;compactMarqueeAt=at;}
 let offset=0;
 if(overflow>1){
  const hold=1200,travel=Math.max(4000,Math.min(18000,overflow/35*1000)),cycle=2*(hold+travel),elapsed=(at-compactMarqueeAt)%cycle,ease=value=>value*value*(3-2*value);
  if(elapsed<hold)offset=0;
  else if(elapsed<hold+travel)offset=-overflow*ease((elapsed-hold)/travel);
  else if(elapsed<2*hold+travel)offset=-overflow;
  else offset=-overflow*(1-ease((elapsed-2*hold-travel)/travel));
 }
 const transform='translate3d('+offset+'px,0,0)';for(const element of elements)setStyle(element,'transform',transform);
}
function compactBarTextProgress(player,progress){if(player.className!=='compact-bar')return;const playerRect=player.getBoundingClientRect(),fill=player.clientWidth*progress;for(const element of [$('artist'),$('title')]){const rect=element.getBoundingClientRect(),local=Math.max(0,Math.min(1,(fill-(rect.left-playerRect.left))/Math.max(1,rect.width)));element.style.setProperty('--text-fill',local*100+'%');}}
$('cover').onload=()=>{if($('player').className==='neon'){$('cover').classList.remove('neon-reveal');void $('cover').offsetWidth;$('cover').classList.add('neon-reveal');}if($('player').className==='eclipse'){$('cover').classList.remove('eclipse-reveal');void $('cover').offsetWidth;$('cover').classList.add('eclipse-reveal');}if(['vector','pulse','cosmos','gargantua'].includes($('player').className)){const artwork=document.querySelector('.artwork'),reveal=$('player').className+'-reveal';artwork.classList.remove(reveal);void artwork.offsetWidth;artwork.classList.add(reveal);}$('cover').hidden=false;$('placeholder').style.display='none';coverPalette=coverColors($('cover'));};
$('cover').onerror=()=>{$('cover').hidden=true;$('placeholder').style.display='grid';coverPalette=null;};
async function update(){
 if(busy)return;busy=true;
 try{
  const response=await fetch('/state'+contextQuery);if(!response.ok)throw Error();
  state=await response.json();const {media:m,settings:s}=state;
  const widthScale=Math.max(50,Math.min(150,Number(s.widgetWidthScale)||100))/100,heightScale=Math.max(50,Math.min(150,Number(s.widgetHeightScale)||100))/100;const widget=$('widget');widget.style.left=sourcePadding+'px';widget.style.top=sourcePadding+'px';widget.style.width=Math.max(0,innerWidth-sourcePadding*2)+'px';widget.style.height=Math.max(0,innerHeight-sourcePadding*2)+'px';widget.style.removeProperty('transform');
  const p=$('player');p.className=!s.skin||s.skin==='overlay'||s.skin==='spectrum'?'compact':s.skin;p.dataset.showTrackCover=String(p.className==='cover'||s.showTrackCover!==false);p.dataset.progressVisible=String(s.progressVisible!==false);p.dataset.showPlayerButtons=String(s.showPlayerButtons!==false);p.style.borderRadius=Math.max(0,Math.min(100,s.cornerRadius===undefined?25:Number(s.cornerRadius)||0))+'px';p.style.setProperty('--corner-radius',p.style.borderRadius);p.style.setProperty('--progress-thickness',Math.max(1,Math.min(12,Number(s.progressThickness)||defaultProgressThickness(p.className)))+'px');const coverScale=Math.max(50,Math.min(150,Number(s.coverScale??defaultCoverScale(p.className))||defaultCoverScale(p.className))),coverSize=150*coverScale/100;p.style.setProperty('--cover-size',coverSize+'px');p.style.setProperty('--prism-cover-overhang',coverSize*.049525+'px');p.dataset.playing=String(!!m.playing);p.dataset.blur=String(!!s.blur&&!!s.backgroundSource);
  if(['cosmos','gargantua'].includes(p.className))ensureCosmosStars(p);if(p.className==='gargantua')ensureGargantuaVortex(p);
  p.style.fontFamily=s.fontFamily||'';const selectedWeight=Math.max(0,Math.min(900,Number(s.fontWeight)||0));for(const element of [$('artist'),$('title'),document.querySelector('.footer')])element.style.fontWeight=selectedWeight?String(selectedWeight):'';
  p.style.filter='brightness('+Math.min(150,Math.max(10,Number(s.brightness)||96))+'%)';
  $('tint').style.opacity=Math.min(100,Math.max(0,Number(s.opacity)||0))/100;
  setText('artist',m.artist||'ИСПОЛНИТЕЛЬ');setText('title',(p.className==='compact-bar'?'— ':'')+(m.title||'Нет названия'));
  for(const [id,scale] of [['title',s.titleScale],['artist',s.artistScale]]){
   const text=$(id);text.style.removeProperty('font-size');text.style.removeProperty('line-height');
   const base=getComputedStyle(text),factor=Math.min(200,Math.max(50,Number(scale)||100))/100;
   const size=parseFloat(base.fontSize),line=parseFloat(base.lineHeight);
   text.style.setProperty('font-size',size*factor+'px','important');
   text.style.setProperty('line-height',(Number.isFinite(line)?line*factor:size*factor*1.25)+'px','important');
  }
  const footer=document.querySelector('.footer');
  footer.style.removeProperty('font-size');footer.style.removeProperty('line-height');
  const timeStyle=getComputedStyle(footer),timeDelta=Math.max(1,Math.min(12,Number(s.progressThickness)||defaultProgressThickness(p.className)))-3;
  footer.style.fontSize=(parseFloat(timeStyle.fontSize)+timeDelta)+'px';
  footer.style.lineHeight=((parseFloat(timeStyle.lineHeight)||parseFloat(timeStyle.fontSize)*1.35)+timeDelta)+'px';
  const meta=document.querySelector('.meta'),a=Math.min(200,Math.max(50,Number(s.artistScale)||100))/100,t=Math.min(200,Math.max(50,Number(s.titleScale)||100))/100;
  meta.style.removeProperty('transform');meta.style.removeProperty('width');meta.style.removeProperty('margin-top');meta.style.removeProperty('margin-bottom');
  const shrink=s.adaptiveSize!==false&&s.progressVisible===false;
  if(p.className==='cassette'){const coverSize=150*coverScale/100,labelHeight=Math.max(s.showTrackCover===false?0:coverSize,$('artist').offsetHeight+$('title').offsetHeight+4);p.style.setProperty('--cassette-label-height',labelHeight+'px');}
  p.style.setProperty('--split-height',Math.max(shrink?90:110,(shrink?50:70)+16*a+14*t+Math.max(0,(Number(s.progressThickness)||defaultProgressThickness(p.className))-3))+'px');
  meta.style.minHeight=p.dataset.showTrackCover!=='false'&&p.className==='glass'&&s.adaptiveSize===false&&s.progressVisible===false?(73+Math.max(1,Math.min(12,Number(s.progressThickness)||defaultProgressThickness(p.className)))+16*a+16*t)+'px':'';
   const coverHidden=p.className!=='compact-bar'&&!['vinyl','orbit','prism','eclipse'].includes(p.className)&&p.dataset.showTrackCover==='false';
  if(coverHidden){
   const measure=document.createElement('canvas').getContext('2d');
   const textWidth=Math.max(...['artist','title'].map(id=>{const style=getComputedStyle($(id));measure.font=style.fontWeight+' '+style.fontSize+' '+style.fontFamily;return measure.measureText($(id).textContent).width;}));
   const style=getComputedStyle(p),paddingX=parseFloat(style.paddingLeft)+parseFloat(style.paddingRight),paddingY=parseFloat(style.paddingTop)+parseFloat(style.paddingBottom),outerY=p.className==='compact'?44:40;
   const width=Math.ceil(Math.max(p.className==='cassette'?520:270,Math.min(900,40+paddingX+textWidth+12)));
   p.style.width=(Math.round(width*widthScale)-40)+'px';p.style.height='auto';
   const height=Math.max(100,Math.ceil(outerY+paddingY+meta.offsetHeight));
   p.style.height=(Math.max(100,Math.round(height*heightScale))-outerY)+'px';
   const signature=[p.className,width,height,s.progressVisible,'no-cover'].join(':');
   if(measuredLayout!==signature){measuredLayout=signature;fetch('/layout'+contextQuery,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({skin:p.className,width,height})}).then(r=>{if(!r.ok)measuredLayout='';}).catch(()=>{measuredLayout='';});}
  }else{p.style.removeProperty('width');p.style.removeProperty('height');measuredLayout='';}
  if(['prism','orbit','eclipse','neon'].includes(p.className)){const style=getComputedStyle(p),available=p.clientHeight-parseFloat(style.paddingTop)-parseFloat(style.paddingBottom);p.style.setProperty('--art-size',Math.max(24,Math.min(p.className==='neon'?128:p.className==='eclipse'?224:p.className==='orbit'?176:84,available))+'px');meta.style.transform='';meta.style.transformOrigin='left center';meta.style.width='';const ratio=Math.min(1,(p.clientHeight-(p.className==='orbit'?56:32))/meta.offsetHeight);if(ratio<1){meta.style.transform='scale('+ratio+')';meta.style.width=(100/ratio)+'%';}}
  if(p.className==='vinyl'){const scale=Math.min(p.clientWidth/320,p.clientHeight/340);p.style.setProperty('--disc-size',Math.max(60,238*scale)+'px');}
  if(p.className==='compact'){const space=p.clientHeight-30;meta.style.transform='';meta.style.transformOrigin='left center';const ratio=Math.min(1,space/meta.offsetHeight);meta.style.transform=ratio<1?'scale('+ratio+')':'';meta.style.width=ratio<1?(100/ratio)+'%':'';}
  if(p.className==='vinyl'){
   const style=getComputedStyle(p),disc=parseFloat(style.getPropertyValue('--disc-size'))||238,available=Math.max(1,p.clientHeight-parseFloat(style.paddingTop)-parseFloat(style.paddingBottom)-disc-14),height=meta.offsetHeight,ratio=Math.min(1,available/Math.max(1,height));
   if(ratio<1){const compensation=(height*ratio-height)/2;meta.style.transform='scale('+ratio+')';meta.style.transformOrigin='center center';meta.style.marginTop=(14+compensation)+'px';meta.style.marginBottom=compensation+'px';}
  }
  if(p.className==='vinyl'){const art=document.querySelector('.artwork'),arm=document.querySelector('.tonearm'),size=art.clientWidth,scale=size/238;arm.style.width=(300*scale)+'px';arm.style.height=(300*scale)+'px';arm.style.left=(art.offsetLeft-31*scale)+'px';arm.style.top=(art.offsetTop-31*scale)+'px';}
  if(coverHidden){
   const style=getComputedStyle(p),available=Math.max(1,p.clientHeight-parseFloat(style.paddingTop)-parseFloat(style.paddingBottom)),ratio=Math.min(1,available/Math.max(1,meta.offsetHeight));
   meta.style.transform=ratio<1?'scale('+ratio+')':'';meta.style.transformOrigin='center center';meta.style.width='100%';
  }
  p.style.setProperty('--meta-height',meta.offsetHeight+'px');
  if(p.className==='cassette'){p.style.setProperty('--cassette-width',p.clientWidth+'px');p.style.setProperty('--cassette-height',p.clientHeight+'px');}
  const artwork=document.querySelector('.artwork');
  if(p.className==='glass'&&p.dataset.blur==='true'){
   artwork.style.clipPath='inset(0 0 '+meta.offsetHeight+'px 0)';
  }else artwork.style.removeProperty('clip-path');
   const coverToken=m.coverKey+'|'+(m.coverRevision||0);if(key!==coverToken){key=coverToken;$('cover').hidden=true;$('placeholder').style.display='grid';if(m.coverKey)$('cover').src='/cover?key='+encodeURIComponent(m.coverKey)+'&revision='+(m.coverRevision||0)+(contextId?'&context='+encodeURIComponent(contextId):'');else coverPalette=null;}
 }catch{}finally{busy=false;}
}
function animate(){
 const frameAt=performance.now(),frameDelta=orbitFrameAt?Math.min(100,frameAt-orbitFrameAt):0;orbitFrameAt=frameAt;
 paintColors(frameAt);
 if(state){
  const {settings:s,testUntil=0,overlayUntil=0}=state;
  let progress=1;
  if(s.auto){const end=Math.max(testUntil,overlayUntil),seconds=Math.max(1,Math.min(30,Number(s.showSeconds)||1)),start=end===testUntil?end-5000:end-seconds*1000,t=Date.now();progress=t<end?Math.max(0,Math.min(1,(t-start)/500)):Math.max(0,Math.min(1,1-(t-end)/500));}
  if(state.idleFade){const {from,to,at}=state.idleFade;progress*=from+(to-from)*Math.max(0,Math.min(1,(Date.now()-at)/500));}
  const m=state.media,n=MusicTimeline.position(m),trackProgress=visualProgress(m,Date.now(),{mode:s.progressAnimation});setStyle($('bar'),'width',(100*trackProgress)+'%');compactBarMarquee($('player'),frameAt);compactBarTextProgress($('player'),trackProgress);setText('position',m.duration?MusicTimeline.time(n):'—');setText('duration',m.duration?MusicTimeline.time(m.duration):'—');
  const p=$('player');setStyle(p,'opacity',progress);
  drawGargantuaVortex(p,frameAt,m);
  if(p.className==='vinyl')moveTonearm(m,n,frameAt);
  if(p.className==='orbit'){
   if(m.playing)orbitAngle=(orbitAngle+frameDelta*Math.PI*2/12000)%(Math.PI*2);
   const radius=(document.querySelector('.artwork').clientWidth+16)/2;
   const x=radius*Math.cos(orbitAngle),y=radius*.78*Math.sin(orbitAngle),tilt=-Math.PI/6;
   p.style.setProperty('--satellite-x',(x*Math.cos(tilt)-y*Math.sin(tilt))+'px');
   p.style.setProperty('--satellite-y',(x*Math.sin(tilt)+y*Math.cos(tilt))+'px');
  }
  if(p.className==='cosmos'){
   if(m.playing){cosmosOrbitA=(cosmosOrbitA+frameDelta*Math.PI*2/9000)%(Math.PI*2);cosmosOrbitB=(cosmosOrbitB-frameDelta*Math.PI*2/11500)%(Math.PI*2);}
   const size=document.querySelector('.artwork').clientWidth,place=(angle,radius,tilt,prefix)=>{const x=radius*Math.cos(angle),y=radius*.58*Math.sin(angle);p.style.setProperty('--cosmos-'+prefix+'-x',(x*Math.cos(tilt)-y*Math.sin(tilt))+'px');p.style.setProperty('--cosmos-'+prefix+'-y',(x*Math.sin(tilt)+y*Math.cos(tilt))+'px');};
   place(cosmosOrbitA,(size+32)/2,-Math.PI/5,'a');place(cosmosOrbitB,(size+58)/2,Math.PI/5,'b');
  }
  if(p.className==='neon'){
   const wire=document.querySelector('.neon-wire'),w=Math.max(1,wire.parentElement.clientWidth),h=24,r=Math.min(16,w/2),d='M 0 20 H '+(w-r)+' Q '+w+' 20 '+w+' '+(20-r)+' V 0';
   if(wire.getAttribute('viewBox')!=='0 0 '+w+' '+h){wire.setAttribute('viewBox','0 0 '+w+' '+h);for(const path of wire.querySelectorAll('path'))path.setAttribute('d',d);}
   setStyle(wire.querySelector('.neon-wire-front'),'strokeDashoffset',1-trackProgress);
  }
  p.style.setProperty('--track-progress',(100*trackProgress)+'%');
  progressEffects(s.progressVisible===false?null:visualProgress.effect,currentColors?colorHex(currentColors.accent):s.barColor||'#ed4169');
  setStyle(p,'transform',s.direction==='right'?'translateX('+110*(1-progress)+'%)':s.direction==='top'?'translateY('+(-120*(1-progress))+'%)':s.direction==='bottom'?'translateY('+120*(1-progress)+'%)':s.direction==='fade'?'translate(0,0)':'translateX('+(-110*(1-progress))+'%)');
 }
 requestAnimationFrame(animate);
}
update();setInterval(update,250);animate();

for(const button of document.querySelectorAll('.transport button'))button.onclick=()=>fetch('/'+button.dataset.command+contextQuery,{method:'POST',headers:{'Content-Type':'application/json'},body:'{}'}).catch(()=>{});
