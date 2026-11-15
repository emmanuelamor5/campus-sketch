"""Convert a SketchUp DXF zip to static browser geometry; see web/README.md."""
import sys,io,zipfile,json,collections
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'.tools'))
import ezdxf
import numpy as np
from ezdxf.math import Matrix44
from ezdxf.render import MeshBuilder
from ezdxf.colors import aci2rgb
from PIL import Image

def read_source(path):
    with zipfile.ZipFile(path) as archive:
        name=next(n for n in archive.namelist() if n.lower().endswith('.dxf'))
        lines=archive.read(name).decode('utf-8',errors='replace').replace('\r\r\n','\n').splitlines()
    clean=[]
    for i in range(0,len(lines)-1,2):
        if lines[i].strip()!='310':clean.extend(lines[i:i+2])
    return ezdxf.read(io.StringIO('\n'.join(clean)+'\n'))

def convert(path):
    d=read_source(path);print('DXF loaded',flush=True)
    chunks=[];colors=[];cache={};counts=collections.Counter();skipped=collections.Counter()
    material_colors={}
    with zipfile.ZipFile(path) as archive:
        for name, material in d.materials:
            filename=material.dxf.get('diffuse_map_file_name','')
            named={'_Color_000_':(22,25,23),'_Gray_':(77,80,76),'_Khaki_':(200,202,184),'_Light_Oliv_':(115,139,69)}
            if name in named:material_colors[material.dxf.handle]=named[name]
            if filename in archive.namelist() and any(word in name.lower() for word in ['asphalt','roof','grass','concrete','brick','stucco']):
                material_colors[material.dxf.handle]=tuple(np.array(Image.open(io.BytesIO(archive.read(filename))).convert('RGB').resize((1,1)))[0,0])
    def style(e,inherited):
        rgb=e.rgb
        handle=e.dxf.get('material_handle','45')
        if handle in material_colors:return material_colors[handle]
        if rgb is not None:return rgb
        if inherited is not None:return inherited
        layer=d.layers.get(e.dxf.layer)
        if layer.rgb is not None:return layer.rgb
        aci=e.dxf.get('color',256)
        if aci in (0,256):aci=layer.dxf.color
        return aci2rgb(abs(aci) if 0<abs(aci)<256 else 7)
    def walk(entities,matrix,depth=0,inherited=None):
        if depth>32:raise ValueError('Excessively nested CAD blocks')
        for e in entities:
            kind=e.dxftype()
            if kind=='INSERT':
                walk(e.block(),e.matrix44() @ matrix,depth+1,style(e,inherited));continue
            if kind not in ('MESH','POLYLINE','3DFACE') or (kind=='POLYLINE' and not(e.is_poly_face_mesh or e.is_polygon_mesh)):
                skipped[kind]+=1;continue
            rgb=style(e,inherited)
            key=(e.dxf.handle,tuple(rgb))
            if key not in cache:
                if kind=='MESH':verts=np.asarray(e.vertices,dtype=float);faces=e.faces
                elif kind=='POLYLINE':
                    mesh=MeshBuilder.from_polyface(e);verts=np.array([tuple(v) for v in mesh.vertices]);faces=mesh.faces
                else:verts=np.array([tuple(v) for v in e.wcs_vertices()]);faces=[list(range(len(verts)))];
                indices=[]
                for face in faces:
                    for k in range(1,len(face)-1):indices.extend((face[0],face[k],face[k+1]))
                if not indices:cache[key]=None;continue
                points=verts[indices].reshape(-1,3,3)
                good=np.linalg.norm(np.cross(points[:,1]-points[:,0],points[:,2]-points[:,0]),axis=1)>1e-10
                points=points[good].reshape(-1,3)
                color=np.array(rgb,dtype=np.float32)/255
                color=np.where(color<=.04045,color/12.92,((color+.055)/1.055)**2.4)
                cache[key]=(points,color)
            item=cache[key]
            if item is None:continue
            points,color=item
            if not len(points):continue
            # ezdxf's row-vector matrix composes local block transforms outward.
            mat=np.array(list(matrix)).reshape(4,4)
            transformed=(points@mat[:3,:3]+mat[3,:3]).astype('<f4')
            chunks.append(transformed);colors.append(np.tile(color,(len(points),1)));counts[kind]+=1
    walk(d.modelspace(),Matrix44())
    p=np.concatenate(chunks);chunks.clear();cache.clear();c=np.concatenate(colors);colors.clear();low=p.min(axis=0);high=p.max(axis=0)
    print('SOURCE BOUNDS',low.tolist(),high.tolist(),flush=True)
    centre=(low+high)/2;scale=float(180/max(high[0]-low[0],high[1]-low[1]))
    old_y=p[:,1].copy()
    p[:,0]=(p[:,0]-centre[0])*scale
    p[:,1]=(p[:,2]-low[2])*scale
    p[:,2]=-(old_y-centre[1])*scale
    del old_y
    out=p
    assert np.isfinite(out).all()
    dest=Path(__file__).resolve().parents[1]/'web/assets'
    with (dest/'campus.bin').open('wb') as output:
        out.tofile(output);c.astype('<f4',copy=False).tofile(output)
    cells=np.unique(np.floor(out[:,[0,2]]/3).astype(int),axis=0)
    minimap=[[round(90+x*3*.65,2),round(72+z*3*.65,2),1.95,1.95] for x,z in cells]
    meta={'vertices':len(out),'triangles':len(out)//3,'byteLength':out.nbytes+c.nbytes,'map':minimap,'source':'Campus model.dxf.zip','sourceBounds':[low.tolist(),high.tolist()],'normalizationScale':scale,'geometryEntities':dict(counts),'omittedEntities':dict(skipped),'notes':'Explicit meshes, transformed block instances and CAD colors. Proxy graphics, annotation, linework and texture mapping omitted.'}
    (dest/'campus.json').write_text(json.dumps(meta,separators=(',',':')),encoding='utf-8')
    print(json.dumps({k:v for k,v in meta.items() if k!='map'},indent=2),flush=True)
if __name__=='__main__':convert(sys.argv[1])




