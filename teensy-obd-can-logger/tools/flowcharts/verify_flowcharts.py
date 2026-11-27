"""Check code fingerprints, PDF/PPTX content and optional Canva extraction."""
import hashlib,json,pathlib,re,zipfile,xml.etree.ElementTree as ET
from pypdf import PdfReader
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/flowcharts/dev9'
BUILD=ROOT/'build/flowcharts-dev9'
spec=json.loads((OUT/'flowcharts.source.json').read_text())
norm=lambda s:re.sub(r'\s+','',s)
items=[(p['number'],kind,v) for p in spec['pages'] for kind,values in
       [('title',[p['title']]),('node',[n['label'] for n in p['nodes']]),('note',p['notes'])] for v in values]
for f in spec['source_files']:
    assert hashlib.sha256((ROOT/f['file']).read_bytes()).hexdigest()==f['sha256'],f['file']
reader=PdfReader(OUT/'SkiDrive_Code_Flowcharts.pdf')
assert len(reader.pages)==33
pdftext=norm('\n'.join(p.extract_text() for p in reader.pages))
missing=[x for x in items if norm(x[2]) not in pdftext]
assert not missing,('PDF missing',missing)
with zipfile.ZipFile(OUT/'SkiDrive_Code_Flowcharts.pptx') as z:
    slides=[n for n in z.namelist() if re.fullmatch(r'ppt/slides/slide\d+.xml',n)]
    assert len(slides)==33
    pptxtext=''.join(''.join(ET.fromstring(z.read(n)).itertext()) for n in slides)
    pictures=sum(len(ET.fromstring(z.read(n)).findall('.//{http://schemas.openxmlformats.org/presentationml/2006/main}pic')) for n in slides)
assert pictures==0,'Flowcharts must not be flattened slide images'
assert all(norm(v) in norm(pptxtext) for _,_,v in items)
canva=BUILD/'canva-content.txt'
canva_checked=0
if canva.exists():
    text=norm(canva.read_text());missing=[x for x in items if norm(x[2]) not in text]
    assert not missing,('Canva missing',missing)
    canva_checked=len(items)
report=dict(firmware=spec['version'],source_commit=spec['source_commit'],flowchart_pages=33,
    source_files_verified=len(spec['source_files']),titles_nodes_notes_checked=len(items),
    pdf_missing_items=0,pptx_missing_items=0,pptx_flattened_pictures=0,
    canva_items_checked=canva_checked,canva_missing_items=0 if canva_checked else None,
    scope='Runtime module/control-flow coverage; not machine-instruction coverage or independent driver verification.')
(OUT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
