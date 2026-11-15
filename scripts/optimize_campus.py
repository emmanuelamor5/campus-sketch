"""Cluster subpixel CAD detail, index shared vertices and gzip for static hosting."""
import json,gzip,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'.tools'))
import numpy as np

def optimize(root):
    root=Path(root);meta=json.loads((root/'campus.json').read_text());n=meta['vertices']
    p=np.memmap(root/'campus.bin',dtype='<f4',mode='r',shape=(n,3))
    c=np.memmap(root/'campus.bin',dtype='<f4',mode='r',offset=n*12,shape=(n,3))
    step=.03;parts=[]
    for start in range(0,n,300000):
        q=np.rint(p[start:start+300000]/step).astype('<i2')
        local=c[start:start+300000]
        vegetation=(local[:,1]>local[:,0]*1.05)&(local[:,1]>local[:,2]*1.05)
        q[vegetation]=(np.rint(p[start:start+300000][vegetation]/.3)*10).astype('<i2')
        q=q.reshape(-1,3,3)
        good=np.any(np.cross(q[:,1].astype(float)-q[:,0],q[:,2].astype(float)-q[:,0])!=0,axis=1)
        q=q[good].reshape(-1,3)
        col=np.rint(np.clip(c[start:start+300000],0,1)*255).astype('<i2').reshape(-1,3,3)[good].reshape(-1,3)
        parts.append(np.column_stack((q,col)))
    records=np.concatenate(parts);del parts
    unique,index=np.unique(records,axis=0,return_inverse=True);del records
    faces=index.reshape(-1,3).astype('<u4');canon=np.sort(faces,axis=1);_,keep=np.unique(canon,axis=0,return_index=True);faces=faces[np.sort(keep)]
    position=(unique[:,:3].astype('<f4')*step);color=unique[:,3:].astype('u1');indices=faces.astype('<u4')
    binary=position.tobytes()+color.tobytes();padding=(-len(binary))%4;binary+=b'\0'*padding;indexOffset=len(binary);binary+=indices.tobytes()
    compressed=gzip.compress(binary,compresslevel=9,mtime=0);(root/'campus.bin.gz').write_bytes(compressed)
    meta.update({'originalTriangles':meta['triangles'],'vertices':len(unique),'triangles':len(faces),'byteLength':len(binary),'compressedBytes':len(compressed),'indexOffset':indexOffset,'indices':indices.size,'detailGrid':step,'vegetationGrid':.3,'compression':'gzip','format':'float32 positions, normalized uint8 linear colors, uint32 indices'})
    (root/'campus.json').write_text(json.dumps(meta,separators=(',',':')),encoding='utf-8')
    print({k:v for k,v in meta.items() if k not in ('map',)},flush=True)
if __name__=='__main__':optimize(Path(__file__).resolve().parents[1]/'web/assets')


