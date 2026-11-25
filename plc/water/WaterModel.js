/* Discrete educational process model. No live PLC connection. */
(function(global){
  const freshMixer=()=>({tank:0,pond:0,pump:false,valve:false,mixer:false,phase:0});
  const freshReservoir=()=>({level:0,chemical:0,truck:0,grass:0,position:0,pump:false,v1:false,v2:false,v3:false,phase:0});
  function mixerStep(s){
    if(s.pump) s.tank=Math.min(150,s.tank+15);
    if(s.valve){ const n=Math.min(10,s.tank,150-s.pond);s.tank-=n;s.pond+=n; }
    s.mixer=s.pond>=120;s.phase=s.mixer?(s.phase+1)%4:0;return s;
  }
  function reservoirStep(s){
    if(s.pump){s.level=Math.min(120,s.level+20);if(s.level>=120)s.pump=false;}
    if(s.v1){const n=Math.min(10,s.level);s.level-=n;s.grass+=n;}
    if(s.v2){const n=Math.min(10,s.level,100-s.chemical);s.level-=n;s.chemical+=n;}
    if(s.v3){const n=Math.min(10,s.level,50-s.truck);s.level-=n;s.truck+=n;}
    if(s.truck>=50){s.v3=false;s.position=Math.min(10,s.position+1);}
    s.phase=(s.phase+1)%4;return s;
  }
  const model={freshMixer,freshReservoir,mixerStep,reservoirStep};
  if(typeof module!=="undefined")module.exports=model;else global.WaterModel=model;
})(typeof globalThis!=="undefined"?globalThis:this);
