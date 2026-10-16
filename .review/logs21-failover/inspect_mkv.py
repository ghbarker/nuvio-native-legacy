"""Inspect real Matroska track encodings; never modify media or infer player success."""
from pathlib import Path
import argparse,hashlib,json,os

IDS={'ebml':0x1A45DFA3,'doctype':0x4282,'segment':0x18538067,'tracks':0x1654AE6B,'entry':0xAE,'number':0xD7,'type':0x83,'codec':0x86,'language':0x22B59C,'name':0x536E,'encodings':0x6D80,'encoding':0x6240,'order':0x5031,'scope':0x5032,'encoding_type':0x5033,'compression':0x5034,'algorithm':0x4254}

def vint(data,pos,identifier=False):
    if pos>=len(data) or not data[pos]: raise ValueError('Invalid/truncated EBML VINT')
    width=1
    while not(data[pos]&(1<<(8-width))):
        width+=1
        if width>8:raise ValueError('Oversize EBML VINT')
    if pos+width>len(data):raise ValueError('Truncated EBML VINT')
    raw=int.from_bytes(data[pos:pos+width],'big')
    if identifier:return raw,width
    value=raw&((1<<(7*width))-1)
    return (None if value==(1<<(7*width))-1 else value),width

def children(data,start,end):
    pos=start
    while pos<end:
        identity,iw=vint(data,pos,True);size,sw=vint(data,pos+iw)
        payload=pos+iw+sw;stop=end if size is None else payload+size
        if stop>end or payload>stop:raise ValueError('EBML child exceeds parent')
        yield identity,payload,stop,pos
        if stop<=pos:raise ValueError('Non-progressing EBML element')
        pos=stop
    if pos!=end:raise ValueError('EBML parent boundary mismatch')

def uint(data,a,b):return int.from_bytes(data[a:b],'big')
def text(data,a,b):return data[a:b].rstrip(b'\x00').decode('utf-8')

def inspect(path):
    data=Path(path).read_bytes();top=list(children(data,0,len(data)))
    ebml=next(x for x in top if x[0]==IDS['ebml']);segment=next(x for x in top if x[0]==IDS['segment'])
    assert any(i==IDS['doctype'] and text(data,a,b)=='matroska' for i,a,b,_ in children(data,ebml[1],ebml[2]))
    tracks=next(x for x in children(data,segment[1],segment[2])if x[0]==IDS['tracks']);rows=[]
    for identity,start,end,offset in children(data,tracks[1],tracks[2]):
        if identity!=IDS['entry']:continue
        row={'track_entry_offset':offset,'content_encodings':[],'language':'eng'}
        for identity,a,b,pos in children(data,start,end):
            for key in ['number','type']:
                if identity==IDS[key]:row[key]=uint(data,a,b)
            for key in ['codec','language','name']:
                if identity==IDS[key]:row[key]=text(data,a,b)
            if identity==IDS['encodings']:
                for eid,ea,eb,eo in children(data,a,b):
                    if eid!=IDS['encoding']:continue
                    encoding={'offset':eo,'order':0,'scope':1,'type':0,'compression':None}
                    for fid,fa,fb,fo in children(data,ea,eb):
                        for key,field in [('order','order'),('scope','scope'),('encoding_type','type')]:
                            if fid==IDS[key]:encoding[field]=uint(data,fa,fb)
                        if fid==IDS['compression']:
                            compression={'algorithm':0,'algorithm_explicit':False,'offset':fo}
                            for cid,ca,cb,co in children(data,fa,fb):
                                if cid==IDS['algorithm']:
                                    compression.update(algorithm=uint(data,ca,cb),algorithm_explicit=True,algorithm_element_offset=co)
                            encoding['compression']=compression
                    row['content_encodings'].append(encoding)
        rows.append(row)
    return {'file':Path(path).name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'doctype':'matroska','tracks':rows,'scope':'Structural EBML inspection only; not an execution of Media3.'}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('input',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if os.environ.get('GITHUB_ACTIONS')!='true':p.error('Media fixtures are inspected on the hosted runner only')
    a.output.write_text(json.dumps(inspect(a.input),indent=2)+'\n')

# Materialize the schema-default value only when mkvmerge omitted it. This is a
# valid metadata remux, not corruption: retain packet bytes and all file offsets
# beyond the reserved track-header Void, and remove optional stale CRC elements.
def explicit_zlib_algorithm(path):
    import zlib
    original=Path(path).read_bytes();before=inspect(path)
    subtitles=[t for t in before['tracks']if t['type']==17]
    assert len(subtitles)==1 and subtitles[0]['codec']=='S_TEXT/UTF8'and subtitles[0]['language']=='eng'
    enc=subtitles[0]['content_encodings'];assert len(enc)==1
    assert enc[0]['scope']==1 and enc[0]['type']==0 and enc[0]['compression']['algorithm']==0
    if enc[0]['compression']['algorithm_explicit']:
        return {'applied':False,'reason':'mkvmerge already wrote explicit zero','before_sha256':before['sha256'],'after_sha256':before['sha256']}
    target=subtitles[0]['number'];crc_removed=[]
    def size_bytes(value,preferred=1):
        for width in range(preferred,9):
            if value<(1<<(7*width))-1:return ((1<<(7*width))|value).to_bytes(width,'big')
        raise ValueError('EBML payload too large')
    def encode(identity,body,width):
        identity=identity.to_bytes((identity.bit_length()+7)//8,'big')
        return identity+size_bytes(len(body),width)+body
    next_mode={'tracks':(IDS['entry'],'entry'),'entry':(IDS['encodings'],'encodings'),'encodings':(IDS['encoding'],'encoding'),'encoding':(IDS['compression'],'compression')}
    def rewrite(identity,start,end,offset,mode):
        _,iw=vint(original,offset,True);_,sw=vint(original,offset+iw);body=bytearray();added=False
        for cid,a,b,pos in children(original,start,end):
            if cid==0xBF:
                assert b-a==4;body+=b'\xec'+original[pos+1:b];crc_removed.append(pos)
            elif mode=='compression'and cid==IDS['algorithm']:
                assert uint(original,a,b)==0;body+=original[pos:b];added=True
            elif mode in next_mode and cid==next_mode[mode][0]:
                eligible=True
                if mode=='tracks':
                    eligible=any(i==IDS['number']and uint(original,x,y)==target for i,x,y,_ in children(original,a,b))
                body+=rewrite(cid,a,b,pos,next_mode[mode][1])if eligible else original[pos:b]
            else:body+=original[pos:b]
        if mode=='compression'and not added:body+=b'\x42\x54\x81\x00'
        return encode(identity,bytes(body),sw)
    top=list(children(original,0,len(original)));seg=next(t for t in top if t[0]==IDS['segment']);nodes=list(children(original,seg[1],seg[2]));index=next(i for i,t in enumerate(nodes)if t[0]==IDS['tracks']);track=nodes[index];padding=nodes[index+1]
    assert padding[0]==0xEC,'Require the real mkvmerge reserved Void immediately after Tracks'
    new_tracks=rewrite(track[0],track[1],track[2],track[3],'tracks');growth=len(new_tracks)-(track[2]-track[3]);assert growth>=4
    remaining=(padding[2]-padding[3])-growth;assert remaining>=2
    for width in range(1,9):
        payload=remaining-1-width
        if payload>=0 and payload<(1<<(7*width))-1:
            new_padding=b'\xec'+size_bytes(payload,width)+bytes(payload);break
    else:raise ValueError('Cannot preserve reserved Void size')
    assert len(new_tracks)+len(new_padding)==padding[2]-track[3]
    patched=bytearray(original[:track[3]]+new_tracks+new_padding+original[padding[2]:])
    for identity,a,b,pos in nodes:
        if identity==0xBF:
            assert b-a==4;patched[pos]=0xEC;crc_removed.append(pos)
    assert len(patched)==len(original) and patched[padding[2]:]==original[padding[2]:]
    Path(path).write_bytes(patched);after=inspect(path)
    assert any(e['compression']and e['compression']['algorithm']==0 and e['compression']['algorithm_explicit']for t in after['tracks']for e in t['content_encodings'])
    return {'applied':True,'before_sha256':before['sha256'],'after_sha256':after['sha256'],'file_bytes_unchanged':len(patched),'explicit_default_bytes_added_to_track_headers':growth,'tracks_offset_unchanged':track[3],'first_byte_after_padding_unchanged':padding[2],'media_suffix_sha256':hashlib.sha256(original[padding[2]:]).hexdigest(),'media_suffix_byte_identical':True,'optional_crc32_replaced_with_equal_size_void':crc_removed,'scope':'Valid explicit-default metadata remux using mkvmerge track-header padding; original compressed packets and every later file offset are unchanged.'}
