import * as THREE from 'three';
import { FontLoader } from './vendor/FontLoader.js';
import { TextGeometry } from './vendor/TextGeometry.js';

const palette = { asphalt: '#171d20', marking: '#ede9d9', grass: '#6d963f', lawn: '#83aa4c', stone: '#d6d0b9', hedge: '#426a31', wood: '#886044' };
const linearBytes = hex => { const c = new THREE.Color(hex); return [c.r,c.g,c.b].map(v => Math.round(v*255)); };

// Apply presentation materials to the normalized CAD model, without changing
// the source archive. Road and low-ground regions follow the reference plan.
export function prepareCampusGeometry(geometry) {
  const p = geometry.attributes.position.array, c = geometry.attributes.color.array;
  const index = geometry.index.array, keep = [], road = linearBytes(palette.asphalt), marking = linearBytes(palette.marking);
  const paved = linearBytes('#9b9f95'), lawn = linearBytes(palette.grass);
  for (let k=0;k<index.length;k+=3) {
    const a=index[k]*3,b=index[k+1]*3,d=index[k+2]*3;
    const x=(p[a]+p[b]+p[d])/3, y=(p[a+1]+p[b+1]+p[d+1])/3, z=(p[a+2]+p[b+2]+p[d+2])/3;
    const maxY=Math.max(p[a+1],p[b+1],p[d+1]), minY=Math.min(p[a+1],p[b+1],p[d+1]);
    // Remove the previous gate's pillars/canopies and crossing, then open the
    // north boundary for the new entrance beside the gazebos.
    const oldGate=x>-40&&x<-9&&z>70.7&&z<76&&y>.65&&maxY<5.5;
    const oldCrossing=x>-40&&x<-9&&z>74&&z<84&&y>.23&&maxY<.4;
    const newOpening=x>-25&&x<-9&&z>-36.2&&z<-34.5&&y>.5&&maxY<4;
    if(oldGate||oldCrossing||newOpening) continue;
    keep.push(index[k],index[k+1],index[k+2]);
    const outside=x<-40.66||z>72.28||z<-41.82;
    let rgb;
    if(outside&&maxY<.23&&minY>.12) rgb=road;
    // CAD lane markings are raised above the roadway by a small amount.
    if(outside&&minY>.23&&maxY<.4&&c[a]<8&&c[a+1]<8&&c[a+2]<8) rgb=marking;
    if(!outside&&maxY<.24&&minY>.12) {
      const green=(x>-16&&x<70&&z>55&&z<72.2)||(x>20&&x<70&&z>32.5&&z<44.5)||(x>-8&&x<44&&z>-35.3&&z<-29.3);
      rgb=green?lawn:paved;
    }
    if(rgb) for(const offset of [a,b,d]) {c[offset]=rgb[0];c[offset+1]=rgb[1];c[offset+2]=rgb[2];}
  }
  geometry.setIndex(new THREE.BufferAttribute(new Uint32Array(keep),1));
  geometry.attributes.color.needsUpdate=true;
  geometry.computeVertexNormals();
}

export function createCampusLandscape(fontData) {
  const group=new THREE.Group(); group.name='Keri Campus landscaping and main gate';
  const mats={};
  function material(color){return mats[color]??=new THREE.MeshStandardMaterial({color,roughness:.9});}
  function box(x,y,z,w,h,d,color){const m=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),material(color));m.position.set(x,y,z);m.castShadow=true;group.add(m);return m;}
  function patch(points,color,y=.25){const s=new THREE.Shape();points.forEach(([x,z],i)=>i?s.lineTo(x,-z):s.moveTo(x,-z));s.closePath();const m=new THREE.Mesh(new THREE.ShapeGeometry(s),material(color));m.rotation.x=-Math.PI/2;m.position.y=y;m.receiveShadow=true;group.add(m);return m;}
  function shrub(x,z,size=.6,color=palette.hedge){const m=new THREE.Mesh(new THREE.IcosahedronGeometry(size,1),material(color));const floor=x>18&&x<70&&z>32&&z<45?.85:.25;m.position.set(x,floor+size*.65,z);m.castShadow=true;m.scale.y=.7;group.add(m);}
  function bench(x,z,rotation=0){const b=new THREE.Group();b.position.set(x,.85,z);b.rotation.y=rotation;group.add(b);function part(px,py,pz,w,h,d,color){const m=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),material(color));m.position.set(px,py,pz);b.add(m);}for(const leg of [-1,1]){part(leg,.4,0,.16,.8,.65,'#35473b');}for(let i=0;i<3;i++)part(0,.87,-.3+i*.25,2.8,.12,.18,palette.wood);for(let i=0;i<2;i++)part(0,1.2+i*.25,-.5,2.8,.16,.12,palette.wood);}
  // Broad front lawn and two courtyard islands leave the existing paths clear.
  patch([[-13,58],[69,58],[69,71.5],[-13,71.5]],palette.grass);
  patch([[22,33],[34,33],[39,35],[36,42.5],[22,42.5]],palette.lawn,.86);
  patch([[45,33],[65,33],[68,41.5],[52,43],[46,40]],palette.grass,.86);
  patch([[-7,-34.5],[43,-34.5],[43,-30],[-7,-30]],palette.grass);
  // Tree islands within the parking court.
  for(const [start,end] of [[-27,15],[26,65]]){
    patch([[-32,start],[-26,start],[-26,end],[-32,end]],'#719247');
    for(let z=start+1;z<end;z+=3)shrub(-32,z,.35);
  }
  // Paths and an open seating court form the leisure point.
  patch([[38,32.5],[41,32.5],[44,36],[49,36],[49,39],[43,39],[40,43],[37.5,43],[40.5,37]],palette.stone,.89);
  bench(45,36.5);bench(45,41,Math.PI);bench(32,40,Math.PI/2);bench(57,38,-Math.PI/2);
  for(let x=22;x<=66;x+=2){shrub(x,43,.48);if(x%4===2)shrub(x,42.6,.22,'#c99a9d');}
  for(let x=-10;x<69;x+=3)shrub(x,70,.45);
  // Fuller low-poly canopies give the leisure garden shade and greenery.
  for(const [x,z] of [[26,36],[54,38],[63,35]]){
    const trunk=new THREE.Mesh(new THREE.CylinderGeometry(.18,.28,3.5,7),material('#776047'));
    trunk.position.set(x,2.6,z);trunk.castShadow=true;group.add(trunk);
    for(const [dx,dy,dz,size,color] of [[0,0,0,2.3,'#4c783c'],[-1.3,.4,.4,1.7,'#698c43'],[1.2,.7,-.3,1.8,'#789d4b']]){
      const crown=new THREE.Mesh(new THREE.IcosahedronGeometry(size,1),material(color));crown.position.set(x+dx,4.7+dy,z+dz);crown.castShadow=true;group.add(crown);
    }
  }
  // Readable parking bays, aligned along the tree islands.
  for(const z of [-24,-16,-8,0,8,30,38,46,54,62]){
    patch([[-25,z],[-18,z],[-18,z+.13],[-25,z+.13]],palette.marking,.32);
  }
  // Close the old gate opening on the south side.
  box(-27,1.3,72.25,25,2.2,.6,'#b4b098');
  // Main gate on the opposite boundary, next to the gazebo garden.
  patch([[-25,-42],[-9,-42],[-9,-29],[-25,-29]],'#a6aa9c',.3);
  for(const x of [-24,-10]){box(x,2.75,-35.4,1.2,5.1,1.4,'#d2c6a8');box(x,5.4,-35.4,1.8,.35,1.9,'#384f3b');}
  box(-17,5.15,-35.4,15.2,1.25,1.25,'#214c3d');
  const font=new FontLoader().parse(fontData);
  const textGeometry=new TextGeometry('Keri Campus',{font,size:1.05,depth:.16,curveSegments:5,bevelEnabled:true,bevelThickness:.025,bevelSize:.018,bevelSegments:2});
  textGeometry.computeBoundingBox();textGeometry.translate(-(textGeometry.boundingBox.max.x+textGeometry.boundingBox.min.x)/2,0,0);
  for(const side of [-1,1]){const text=new THREE.Mesh(textGeometry,material('#efd9a0'));text.position.set(-17,4.83,-35.4+side*.72);if(side<0)text.rotation.y=Math.PI;text.castShadow=true;text.name='Keri Campus · extruded gate lettering';group.add(text);}
  // Open metal leaves flank a clear route through the gate.
  for(const side of [-1,1]){
    const leaf=new THREE.Group();leaf.position.set(-17+side*6.35,.3,-35.4);leaf.rotation.y=side*Math.PI/2.8;group.add(leaf);
    for(let i=0;i<9;i++){const bar=new THREE.Mesh(new THREE.BoxGeometry(.08,2.6,.1),material('#354b3d'));bar.position.set(side*i*.55,1.3,0);leaf.add(bar);}
    for(const y of [.45,2.45]){const rail=new THREE.Mesh(new THREE.BoxGeometry(4.5,.12,.12),material('#354b3d'));rail.position.set(side*2.2,y,0);leaf.add(rail);}
  }
  for(let z=-51;z<-42;z+=1.2)patch([[-20,z],[-14,z],[-14,z+.45],[-20,z+.45]],palette.marking,.31);
  for(const [x,z] of [[-26,-34],[-8,-34],[-26,-30],[-8,-30]])shrub(x,z,.85);
  group.userData.gate={x:-17,z:-35.4,label:'Keri Campus',side:'gazebos'};
  return group;
}
