// Editable shapes/text for Canva import; Graphviz only supplies layout coordinates.
// Usage: bundled-node build_flowcharts.mjs <runtime-node-modules> [page-limit]
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
import {createRequire} from 'node:module';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const modules=process.argv[2];
if(!modules)throw Error('Pass the bundled runtime node_modules directory');
const {instance}=await import(pathToFileURL(path.join(modules,'@viz-js/viz/dist/viz.js')));
const {Presentation,PresentationFile,registerPdfFontFace}=await import(pathToFileURL(path.join(modules,'@oai/artifact-tool/dist/artifact_tool.mjs')));
registerPdfFontFace({family:'Arial',weight:400,style:'normal',source:new Uint8Array(await fs.readFile('C:/Windows/Fonts/arial.ttf'))});
registerPdfFontFace({family:'Arial',weight:700,style:'normal',source:new Uint8Array(await fs.readFile('C:/Windows/Fonts/arialbd.ttf'))});
const require=createRequire(path.join(modules,'@oai/artifact-tool/package.json'));
const {Canvas}=require('skia-canvas');const measure=new Canvas(10,10).getContext('2d');
const out=path.join(root,'docs/flowcharts/dev9');
const tmp=path.join(root,'build/flowcharts-dev9');await fs.mkdir(tmp,{recursive:true});
const spec=JSON.parse(await fs.readFile(path.join(out,'flowcharts.source.json'),'utf8'));
const viz=await instance();
const deck=Presentation.create({slideSize:{width:1920,height:1440}});
const COLORS={ink:'#172F44',muted:'#526779',line:'#546D7F',process:'#F6F8FA',decision:'#FFF3D6',call:'#E5F1F5',fault:'#FBE5E2',start:'#DDECE8',end:'#DDECE8'};
const wrap=(s,n=32)=>{let lines=[],line='';for(const w of s.split(/\s+/)){if(line.length+w.length+1>n&&line){lines.push(line);line=w;}else line+=(line?' ':'')+w;}if(line)lines.push(line);return lines.join('\n');};
const q=s=>JSON.stringify(s);
function measuredWrap(value,width,size){
 measure.font=`${size}px Arial`;let lines=[],line='';
 for(const word of value.split(/\s+/)){const next=line?line+' '+word:word;
  if(line&&measure.measureText(next).width>width){lines.push(line);line=word;}else line=next;
 }
 if(line)lines.push(line);return lines.join('\n');
}
const shapes={'process':'rect','decision':'diamond','call':'roundRect','fault':'rect','start':'roundRect','end':'roundRect'};
function text(slide,name,value,x,y,w,h,size,color=COLORS.ink,bold=false,align='left'){
 const sh=slide.shapes.add({name,geometry:'textbox',position:{left:x,top:y,width:w,height:h},fill:'none',line:{fill:'none',width:0}});
 sh.text=value;sh.text.style={fontSize:size,typeface:'Arial',color,bold,alignment:align,verticalAlignment:'top',wrap:'none',autoFit:'none',insets:0,lineSpacing:1.04};return sh;
}
function graphDot(p){
 return 'digraph G { graph [rankdir=TB, splines=polyline, nodesep=0.38, ranksep=0.36, pad=0.12, bgcolor="transparent"]; node [fontname="Arial", fontsize=17, width=2.8, height=0.44, margin="0.12,0.10", style="filled", color="#546D7F"]; edge [fontname="Arial", fontsize=13, arrowsize=0.75, color="#546D7F"];\n'+
 p.nodes.map(n=>`${n.id} [shape=${n.kind==='decision'?'diamond':n.kind==='start'||n.kind==='end'?'ellipse':'box'},label=${q(wrap(n.label,n.kind==='decision'?27:33))}];`).join('\n')+'\n'+
 p.edges.map(e=>`${e.source} -> ${e.target} [label=${q(e.label)}];`).join('\n')+'\n}';
}
const renderData=[];
for(const p of spec.pages.slice(0,process.argv[3]?Number(process.argv[3]):undefined)){
 const dot=graphDot(p);const g=viz.renderJSON(dot);await fs.writeFile(path.join(tmp,`${String(p.number).padStart(2,'0')}.dot`),dot);
 const [, ,gw,gh]=g.bb.split(',').map(Number);
 const scale=Math.min(1415/gw,1160/gh,1.7);
 const ox=48+(1415-gw*scale)/2,oy=170+(1160-gh*scale)/2;
 const tr=([x,y])=>[ox+x*scale,oy+(gh-y)*scale];
 const slide=deck.slides.add();slide.background.fill='#FFFFFF';
 text(slide,'section',`SKIDRIVE  /  FIRMWARE FLOWCHART ${String(p.number).padStart(2,'0')}`,48,32,1600,32,22,COLORS.muted,true);
 text(slide,'title',p.title,48,75,1800,62,39,COLORS.ink,true);
 text(slide,'version',spec.version,1500,35,370,30,20,COLORS.muted,false,'right');
 slide.shapes.add({geometry:'line',position:{left:1490,top:168,width:0,height:1130},fill:'none',line:{fill:'#CFD8DE',width:1}});
 text(slide,'notes-heading','CODE DETAILS',1520,171,350,33,23,COLORS.ink,true);
 let ny=224;
 const noteItems=[];
 for(let i=0;i<p.notes.length;i++){
   const value=measuredWrap(p.notes[i],330,20);const lineCount=value.split('\n').length;const h=lineCount*24;
   text(slide,`note-${i+1}`,value,1520,ny,350,h,20,COLORS.muted);
   noteItems.push({text:value,x:1520,y:ny,w:350,h,size:20});ny+=h+22;
 }
 if(ny>1325)throw Error(`Notes overflow page ${p.number}: ${ny}`);
 const nodesByGvid=new Map(g.objects.map(n=>[n._gvid,n]));
 const primitives=[];
 // Preserve Graphviz edge routing as editable native freeform lines and arrowheads.
 for(let i=0;i<(g.edges||[]).length;i++){
  const e=g.edges[i];
  for(const d of [...(e._draw_||[]),...(e._hdraw_||[])]){
   if(!['b','B','L','P','p'].includes(d.op))continue;
   const pts=d.points.map(tr);let minx=Math.min(...pts.map(a=>a[0])),miny=Math.min(...pts.map(a=>a[1]));
   const w=Math.max(1,Math.max(...pts.map(a=>a[0]))-minx),h=Math.max(1,Math.max(...pts.map(a=>a[1]))-miny);
   const v=pts.map(([x,y])=>[x-minx,y-miny]);
   const commands=[{moveTo:{x:v[0][0],y:v[0][1]}}];
   if(d.op==='b'||d.op==='B')for(let j=1;j+2<v.length;j+=3)commands.push({cubicBezTo:{x1:v[j][0],y1:v[j][1],x2:v[j+1][0],y2:v[j+1][1],x:v[j+2][0],y:v[j+2][1]}});
   else for(const a of v.slice(1))commands.push({lineTo:{x:a[0],y:a[1]}});
   const closed=d.op==='P'||d.op==='p';if(closed)commands.push({close:{}});
   slide.shapes.add({name:`edge-${i}-${primitives.length}`,geometry:'custom',position:{left:minx,top:miny,width:w,height:h},fill:closed?COLORS.line:'none',line:{fill:COLORS.line,width:1.8},customPaths:[{width:w,height:h,commands}]});
   primitives.push({type:'path',points:pts,op:d.op});
  }
  for(const d of e._ldraw_||[]){if(d.op!=='T')continue;
   const [x,y]=tr(d.pt),w=d.width*scale+14,h=20*scale;
   const left=d.align==='l'?x:d.align==='r'?x-w:x-w/2;
   text(slide,`edge-label-${i}`,d.text,left,y-15*scale,w,h,13*scale,COLORS.muted);
   primitives.push({type:'label',text:d.text,x:left,y:y-15*scale,w,h,size:13*scale});
  }
 }
 for(const n of g.objects){
   const orig=p.nodes.find(x=>x.id===n.name);if(!orig)continue;
   const [cx,cy]=tr(n.pos.split(',').map(Number)),w=Number(n.width)*72*scale,h=Number(n.height)*72*scale;
   const sh=slide.shapes.add({geometry:shapes[orig.kind],name:`node-${n.name}`,position:{left:cx-w/2,top:cy-h/2,width:w,height:h},fill:COLORS[orig.kind],line:{fill:orig.kind==='fault'?'#AD4E45':COLORS.line,width:1.5}});
   const label=n.label.replace(/\\n/g,'\n');
   sh.text=label;
   sh.text.style={fontSize:17*scale,typeface:'Arial',color:COLORS.ink,alignment:'center',verticalAlignment:'middle',autoFit:'none',wrap:'none',insets:0,lineSpacing:1.03};
   primitives.push({type:'node',kind:orig.kind,label,x:cx-w/2,y:cy-h/2,w,h,size:17*scale});
 }
 text(slide,'source',p.refs.join('  |  '),48,1354,1800,40,16,COLORS.muted);
 text(slide,'footer',`${String(p.number).padStart(2,'0')} / ${spec.pages.length}   •   Branch labels are decisions; call boxes lead to detailed charts.   •   Code snapshot: ${spec.source_commit}`,48,1400,1800,25,16,COLORS.muted);
 slide.speakerNotes.text=`${p.title}\n\n${p.notes.join('\n\n')}\n\nSources: ${p.refs.join('; ')}.\nFirmware ${spec.version}; source commit ${spec.source_commit}.`;
 renderData.push({number:p.number,title:p.title,version:spec.version,refs:p.refs,notes:noteItems,primitives,scale,minFont:17*scale,source_commit:spec.source_commit});
 console.log(`${p.number}: ${p.title}; node font ${Math.round(17*scale)}px; ${p.nodes.length} nodes`);
}
await fs.writeFile(path.join(tmp,'render-data.json'),JSON.stringify(renderData));
const candidate=path.join(tmp,'skidrive-firmware-flowcharts-draft.pptx');
await (await PresentationFile.exportPptx(deck)).save(candidate);
console.log('PPTX draft: '+candidate);
// Render a native artifact preview of selected pages before finalizing the deck.
for(const index of deck.slides.items.map((_,i)=>i)){
 const png=await deck.export({slide:deck.slides.items[index],format:'png',scale:0.8});
 await fs.writeFile(path.join(tmp,`native-${String(index+1).padStart(2,'0')}.png`),new Uint8Array(await png.arrayBuffer()));
}
console.log('Native preview render completed');
const pdf=await deck.export({format:'pdf'});
await fs.writeFile(path.join(out,'SkiDrive_Code_Flowcharts.pdf'),new Uint8Array(await pdf.arrayBuffer()));
console.log('PDF exported');
