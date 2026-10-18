"""Convert locally extracted stock glyph nodes to the accepted native geometry."""
import math
import re

def generate(font):
    font['glyphs']['/'] = {'box': {'x': 0, 'y': 0, 'width': 24, 'height': 56.692},
        'node': {'t': 'line', 'a': {'x1': 0, 'y1': 56.692, 'x2': 24, 'y2': 0}, 'children': []}}
    segments = []
    records = []


    def emit(a, b, box):
        if a != b:
            segments.append(tuple(round((v-box['x' if i % 2 == 0 else 'y'])*1024)
                for i,v in enumerate((*a,*b))))


    def curve(a, b, c, d, box):
        prev = a
        for i in range(1,17):
            t=i/16; q=1-t
            p=tuple(q**3*a[j]+3*q*q*t*b[j]+3*q*t*t*c[j]+t**3*d[j] for j in (0,1))
            emit(prev,p,box); prev=p


    def path(d, box):
        tokens=re.findall(r'[A-Za-z]|[-+]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][-+]?\d+)?',d)
        p=(0.,0.); start=p; old_control=None; old_cmd=None; i=0; cmd=None
        while i<len(tokens):
            if tokens[i].isalpha():cmd=tokens[i];i+=1
            assert cmd is not None
            relative=cmd.islower(); code=cmd.upper()
            if code=='Z':emit(p,start,box);p=start;old_control=None;old_cmd=code;cmd=None;continue
            count={'M':2,'L':2,'H':1,'V':1,'C':6,'S':4}[code]
            vals=list(map(float,tokens[i:i+count]));assert len(vals)==count;i+=count
            def point(x,y):return (x+p[0],y+p[1]) if relative else (x,y)
            if code in ('M','L'):
                q=point(*vals)
                if code=='M':start=q;cmd='l' if relative else 'L'
                else:emit(p,q,box)
                p=q;old_control=None
            elif code=='H':q=(p[0]+vals[0] if relative else vals[0],p[1]);emit(p,q,box);p=q;old_control=None
            elif code=='V':q=(p[0],p[1]+vals[0] if relative else vals[0]);emit(p,q,box);p=q;old_control=None
            elif code=='C':
                b,c,q=point(*vals[:2]),point(*vals[2:4]),point(*vals[4:6]);curve(p,b,c,q,box);p=q;old_control=c
            elif code=='S':
                b=tuple(2*p[j]-old_control[j] for j in (0,1)) if old_cmd in ('C','S') else p
                c,q=point(*vals[:2]),point(*vals[2:4]);curve(p,b,c,q,box);p=q;old_control=c
            old_cmd=code


    def node(n, box):
        t,a=n['t'],n['a']
        assert 'transform' not in a
        if t=='g':
            for c in n.get('children',[]):node(c,box)
        elif t=='line':emit((float(a['x1']),float(a['y1'])),(float(a['x2']),float(a['y2'])),box)
        elif t=='polyline':
            values=list(map(float,re.findall(r'[-+]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][-+]?\d+)?',a['points'])))
            assert len(values)%2==0;points=list(zip(values[::2],values[1::2]))
            for x,y in zip(points,points[1:]):emit(x,y,box)
        elif t=='path':path(a['d'],box)
        elif t=='circle':
            cx,cy,r=float(a['cx']),float(a['cy']),float(a['r'])
            points=[(cx+r*math.cos(i*math.pi/16),cy+r*math.sin(i*math.pi/16)) for i in range(33)]
            for x,y in zip(points,points[1:]):emit(x,y,box)
        else:raise ValueError(t)


    for char,g in sorted(font['glyphs'].items()):
        first=len(segments);node(g['node'],g['box']);count=len(segments)-first
        assert first<=65535 and 0<count<=512
        records.append((ord(char),first,count,round(g['box']['width']*1024)))
    assert len(segments)<65536
    lines=['/* PRIVATE TE-derived glyph geometry. Keep outside tracked source/publication. */',
        'static const gin_font_segment_t gin_font_segments[] = {']
    lines += ['    {'+', '.join(map(str,x))+'},' for x in segments]
    lines += ['};','static const gin_font_glyph_t gin_font_glyphs[] = {']
    lines += ['    {'+', '.join(map(str,x))+'},' for x in records]
    lines += ['};']
    return ('\n'.join(lines)+'\n').encode()
