import * as THREE from 'three';
import { OrbitControls } from './vendor/OrbitControls.js';
import { views, places, tourStops } from './campus-data.js';
import { prepareCampusGeometry, createCampusLandscape } from './campus-landscape.js';

const $=id=>document.getElementById(id), canvas=$('scene');
const scene=new THREE.Scene();scene.background=new THREE.Color('#e7ece1');
const camera=new THREE.PerspectiveCamera(40,1,.01,2000);
let renderer;
try{renderer=new THREE.WebGLRenderer({canvas,antialias:true});}catch(e){$('loading').textContent='This device could not start 3D graphics. You can still open the campus references below.';throw e;}
renderer.setPixelRatio(Math.min(devicePixelRatio,2));renderer.outputColorSpace=THREE.SRGBColorSpace;
renderer.shadowMap.enabled=true;renderer.shadowMap.type=THREE.PCFSoftShadowMap;
scene.add(new THREE.HemisphereLight(0xffffff,0x83916e,1.1));
const sun=new THREE.DirectionalLight(0xfff3df,1.4);sun.position.set(-60,100,70);sun.castShadow=true;sun.shadow.mapSize.set(2048,2048);
Object.assign(sun.shadow.camera,{left:-130,right:130,top:130,bottom:-130,near:.5,far:400});sun.shadow.bias=-.0005;sun.shadow.normalBias=.08;scene.add(sun);
const controls=new OrbitControls(camera,canvas);controls.enableDamping=true;controls.maxPolarAngle=Math.PI*.49;controls.minDistance=8;controls.maxDistance=500;controls.autoRotateSpeed=.5;
const reduced=matchMedia('(prefers-reduced-motion: reduce)').matches;
let model,transition,tourTimer,tourIndex=0,selected,dragStart,ready=false;
const markers=[];
const markerCanvas=document.createElement('canvas');markerCanvas.width=markerCanvas.height=64;
const markerContext=markerCanvas.getContext('2d');markerContext.beginPath();markerContext.arc(32,32,23,0,Math.PI*2);markerContext.fillStyle='#fff';markerContext.fill();markerContext.lineWidth=6;markerContext.strokeStyle='#ffffff';markerContext.stroke();
const markerTexture=new THREE.CanvasTexture(markerCanvas);
for(const [key,v] of Object.entries(views)){
  const b=document.createElement('button');b.dataset.view=key;b.textContent=v.label;b.setAttribute('aria-pressed',String(key==='overview'));b.onclick=()=>goView(key);$('views').append(b);
}
places.forEach((p,i)=>{
  const b=document.createElement('button'),number=document.createElement('span');number.textContent=String(i+1).padStart(2,'0');b.append(number,document.createTextNode(p.name));b.onclick=()=>showPlace(i);b.dataset.place=p.view;$('places').append(b);
  const m=new THREE.Sprite(new THREE.SpriteMaterial({map:markerTexture,color:0xd0a356,depthTest:false,sizeAttenuation:false}));m.position.fromArray(p.point);m.renderOrder=10;m.scale.set(.018,.018,1);m.userData.place=i;markers.push(m);
});
function stopTour(){clearInterval(tourTimer);tourTimer=null;$('tour').innerHTML='Start guided tour <span>↗</span>';}
function viewPosition(key){const v=views[key],p=new THREE.Vector3(...v.pos),t=new THREE.Vector3(...v.target);return ['overview','plan'].includes(key)?p.sub(t).multiplyScalar(Math.max(1,.95/camera.aspect)).add(t):p;}
function goView(key,manual=true){
  if(!ready)return;if(manual)stopTour();const v=views[key];controls.autoRotate=false;$('rotate').setAttribute('aria-pressed','false');
  transition={start:performance.now(),from:camera.position.clone(),targetFrom:controls.target.clone(),to:viewPosition(key),target:new THREE.Vector3(...v.target)};
  $('view-title').textContent=v.title;document.querySelectorAll('[data-view]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.view===key)));
  const place=places.find(p=>p.view===key);markers.forEach((m,i)=>{m.material.color.set(places[i]===place?'#376647':'#d0a356');const size=places[i]===place?.024:.018;m.scale.set(size,size,1);});
  document.querySelectorAll('[data-place]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.place===key)));
  if(!manual&&place)displayInfo(place);
}
function displayInfo(place){selected=place;$('info-title').textContent=place.name;$('info-body').textContent=place.text;$('info').hidden=false;}
function showPlace(i){goView(places[i].view);displayInfo(places[i]);}
$('focus').onclick=()=>goView(selected.view);
$('close-info').onclick=()=>$('info').hidden=true;
$('home').onclick=()=>{goView('overview');$('info').hidden=true;};
function zoom(f){stopTour();transition=null;camera.position.sub(controls.target).multiplyScalar(f).clampLength(controls.minDistance,controls.maxDistance).add(controls.target);controls.update();}
$('zoom-in').onclick=()=>zoom(.8);$('zoom-out').onclick=()=>zoom(1.25);
$('rotate').onclick=()=>{stopTour();transition=null;controls.autoRotate=!controls.autoRotate;$('rotate').setAttribute('aria-pressed',String(controls.autoRotate));};
$('tour').onclick=()=>{
  if(tourTimer){stopTour();return;}if(!ready)return;tourIndex=0;goView(tourStops[0],false);$('info').hidden=true;$('tour').textContent='Stop guided tour ■';
  tourTimer=setInterval(()=>{tourIndex++;if(tourIndex>=tourStops.length){stopTour();return;}goView(tourStops[tourIndex],false);if(tourStops[tourIndex]==='overview')$('info').hidden=true;},6500);
};
controls.addEventListener('start',()=>{transition=null;stopTour();});
canvas.addEventListener('pointerdown',e=>dragStart=[e.clientX,e.clientY]);
canvas.addEventListener('pointerup',e=>{
  if(!dragStart||Math.hypot(e.clientX-dragStart[0],e.clientY-dragStart[1])>5)return;
  const r=canvas.getBoundingClientRect(),ray=new THREE.Raycaster();ray.setFromCamera(new THREE.Vector2((e.clientX-r.left)/r.width*2-1,-(e.clientY-r.top)/r.height*2+1),camera);
  const hit=ray.intersectObjects(markers)[0];if(hit)showPlace(hit.object.userData.place);
});
for(const [b,d] of [['reference','reference-dialog'],['help','help-dialog']]){$(b).onclick=()=>$(d).showModal();$(d).querySelector('button').onclick=()=>$(d).close();}
window.addEventListener('keydown',e=>{if(e.key==='Escape'){stopTour();$('info').hidden=true;}});
const mini=$('map').getContext('2d');
function drawMap(){
  mini.clearRect(0,0,180,145);mini.fillStyle='#e6ebdf';mini.fillRect(0,0,180,145);
  if(model){mini.fillStyle='#819277';for(const r of model.map)mini.fillRect(r[0],r[1],Math.max(1,r[2]),Math.max(1,r[3]));}
  mini.fillStyle='#c59b52';for(const p of places){mini.beginPath();mini.arc(90+p.point[0]*.65,72+p.point[2]*.65,2,0,Math.PI*2);mini.fill();}
  const x=90+controls.target.x*.65,y=72+controls.target.z*.65;mini.strokeStyle='#214c3d';mini.lineWidth=2;mini.beginPath();mini.arc(x,y,6,0,Math.PI*2);mini.stroke();
  const delta=camera.position.clone().sub(controls.target).normalize();mini.beginPath();mini.moveTo(x,y);mini.lineTo(x+delta.x*18,y+delta.z*18);mini.stroke();
}
new ResizeObserver(()=>{const w=canvas.clientWidth,h=canvas.clientHeight;renderer.setSize(w,h,false);camera.aspect=w/h;camera.updateProjectionMatrix();}).observe(canvas);
async function load(){
  try{
    const [response,bin,fontResponse]=await Promise.all([fetch('./assets/campus.json'),fetch('./assets/campus.bin.gz'),fetch('./assets/helvetiker_regular.typeface.json')]);
    if(!response.ok||!bin.ok||!fontResponse.ok)throw Error('Campus assets unavailable');
    const [meta,data,font]=await Promise.all([response.json(),new Response(bin.body.pipeThrough(new DecompressionStream('gzip'))).arrayBuffer(),fontResponse.json()]);
    if(data.byteLength!==meta.byteLength)throw Error('Model download incomplete');
    const geometry=new THREE.BufferGeometry();geometry.setAttribute('position',new THREE.BufferAttribute(new Float32Array(data,0,meta.vertices*3),3));
    geometry.setAttribute('color',new THREE.BufferAttribute(new Uint8Array(data,meta.vertices*12,meta.vertices*3),3,true));geometry.setIndex(new THREE.BufferAttribute(new Uint32Array(data,meta.indexOffset,meta.indices),1));
    prepareCampusGeometry(geometry);
    const mesh=new THREE.Mesh(geometry,new THREE.MeshStandardMaterial({vertexColors:true,roughness:.9,side:THREE.DoubleSide}));mesh.name='Original campus CAD · updated road and landscape colors';mesh.castShadow=true;scene.add(mesh);model=meta;
    scene.add(createCampusLandscape(font));
    const ground=new THREE.Mesh(new THREE.PlaneGeometry(650,650),new THREE.MeshStandardMaterial({color:'#e7ece1',roughness:1}));ground.receiveShadow=true;ground.rotation.x=-Math.PI/2;ground.position.y=-.3;scene.add(ground);
    markers.forEach(m=>scene.add(m));ready=true;$('loading').hidden=true;camera.position.copy(viewPosition('overview'));controls.target.fromArray(views.overview.target);controls.update();
  }catch(e){console.error(e);$('loading').textContent='The campus model could not load. Refresh to retry, or open the reference views below.';}
}
load();
renderer.setAnimationLoop(()=>{
  if(transition){const t=reduced?1:Math.min(1,(performance.now()-transition.start)/1400),s=t*t*(3-2*t);camera.position.lerpVectors(transition.from,transition.to,s);controls.target.lerpVectors(transition.targetFrom,transition.target,s);if(t===1)transition=null;}
  controls.update();renderer.render(scene,camera);drawMap();
});
