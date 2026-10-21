'use strict';
window.MusicTimeline={
 position(media,now=Date.now()){
  const duration=Math.max(0,Number(media.duration)||0);
  const elapsed=media.playing&&media.updated?Math.max(0,(now-Number(media.updated))/1000):0;
  return Math.max(0,Math.min(duration,(Number(media.position)||0)+elapsed*(Number(media.rate)||1)));
 },
 time(seconds){const n=Math.max(0,Math.floor(Number(seconds)||0));return Math.floor(n/60)+':'+String(n%60).padStart(2,'0');},
 createProgress(){
  let key=null,scope=null,lastPosition=0,lastDuration=0,value=0,resetAt=null,resetFrom=0,ended=false,mode=null;
  const sample=(media,now=Date.now(),options={})=>{
   const nextMode=["none","smooth","scatter","wick"].includes(options.mode)?options.mode:"smooth";sample.effect=null;
   const nextKey=media.coverKey||[media.session,media.title,media.artist].join('|');
   const duration=Math.max(0,Number(media.duration)||0),position=this.position(media,now);
   const changed=key!==null&&key!==nextKey;
   const nearEnd=lastDuration>0&&lastPosition>=lastDuration-1.5;
   const repeated=!changed&&nearEnd&&position<2&&lastPosition-position>1;
   const finished=duration>0&&position>=duration-.02&&media.playing;
   if(scope!==options.scope||options.immediate||mode!==null&&mode!==nextMode){resetAt=null;ended=false;key=null;value=duration?position/duration:0;}
   else if((changed&&value>0)||repeated||(finished&&!ended)){
    resetFrom=value;resetAt=nextMode==="none"?null:now;ended=finished&&!changed;
   }
   if(changed||repeated)ended=false;
   if(ended&&position<duration-2)ended=false;
   if(resetAt!==null){const t=Math.min(1,Math.max(0,(now-resetAt)/(nextMode==="smooth"?650:1000)));value=nextMode==="scatter"?0:resetFrom*(nextMode==="wick"?1-t:Math.pow(1-t,3));if(t<1)sample.effect={mode:nextMode,t,from:resetFrom,value};if(t===1)resetAt=null;}
   else value=ended?0:duration?position/duration:0;
   key=nextKey;scope=options.scope;mode=nextMode;lastPosition=position;lastDuration=duration;
   return Math.max(0,Math.min(1,value));
  };
  return sample;
 }
};
window.MusicTimeline.createEffects=function(bar,artwork=null){
 let canvas=null;
 const noise=n=>{const v=Math.sin(n*127.1+311.7)*43758.5453;return v-Math.floor(v);};
 return (effect,color)=>{
  if(!effect||!['scatter','wick'].includes(effect.mode)){if(canvas)canvas.hidden=true;return;}
  if(!canvas){canvas=document.createElement('canvas');Object.assign(canvas.style,{position:'fixed',left:'0',top:'0',pointerEvents:'none',zIndex:'100'});document.body.append(canvas);}
  canvas.hidden=false;canvas.width=innerWidth;canvas.height=innerHeight;
  const ctx=canvas.getContext('2d'),rect=bar.parentElement.getBoundingClientRect(),t=effect.t;
  if(!rect.width||!rect.height){canvas.hidden=true;return;}
  const points=[{x:rect.left,y:rect.top+rect.height/2,w:rect.width,height:rect.height}];
  if(artwork&&document.getElementById('player').className==='orbit'){
   const r=artwork.getBoundingClientRect();points.push({x:r.left+r.width/2,y:r.top+r.height/2,r:r.width/2});
  }
  const locate=(p,f)=>p.r?{x:p.x+p.r*Math.cos(Math.PI+2*Math.PI*f),y:p.y+p.r*Math.sin(Math.PI+2*Math.PI*f)}:{x:p.x+p.w*f,y:p.y};
  for(const p of points){
   if(effect.mode==='scatter'){
    const count=Math.max(24,Math.min(140,Math.ceil((p.w||2*Math.PI*p.r)*effect.from/3)));
    ctx.fillStyle=color;
    for(let i=0;i<count;i++){
     const delay=noise(i+47)*.55,age=Math.max(0,Math.min(1,(t-delay)/(1-delay)));
     const origin=locate(p,effect.from*(i+.5)/count),angle=noise(i+1)*Math.PI*2;
     const travel=age*age,speed=12+noise(i+3)*24;
     const fade=Math.max(0,(age-.6)/.4);ctx.globalAlpha=1-fade*fade*(3-2*fade);
     const height=(p.height||3)*(1-.35*age),width=p.r?height:p.w*effect.from/count*(1-.65*age)+.5*(1-age);
     ctx.fillRect(origin.x+Math.cos(angle)*speed*travel-width/2,origin.y+Math.sin(angle)*speed*travel+12*travel-height/2,width,height);
    }
   }else{
    const tip=locate(p,effect.value);ctx.globalAlpha=Math.min(1,(1-t)*6);
    ctx.fillStyle='#fff5c2';ctx.shadowColor='#ff962c';ctx.shadowBlur=10;ctx.beginPath();ctx.arc(tip.x,tip.y,3,0,Math.PI*2);ctx.fill();ctx.shadowBlur=0;
    for(let i=0;i<14;i++){const age=(t*5+noise(i))%1,angle=noise(i+21)*Math.PI*2,distance=age*(8+noise(i+31)*20);ctx.globalAlpha=(1-age)*Math.min(1,(1-t)*6);ctx.fillStyle=i%2?'#ffb449':'#ff6634';ctx.fillRect(tip.x+Math.cos(angle)*distance,tip.y+Math.sin(angle)*distance-age*10,1.5,1.5);}
   }
  }
 };
};
