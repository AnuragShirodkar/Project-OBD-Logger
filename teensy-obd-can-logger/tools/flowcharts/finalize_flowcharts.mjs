import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const skill=process.argv[2],python=process.argv[3];
process.env.RUNTIME_NODE_MODULES=path.resolve(path.dirname(python),'../node/node_modules');
const {finalizePresentation}=await import(pathToFileURL(path.join(skill,'container_tools/artifact_tool_utils.mjs')));
const result=await finalizePresentation({
 workspaceDir:root,
 candidatePath:path.join(root,'build/flowcharts-dev9/skidrive-firmware-flowcharts-draft.pptx'),
 finalPath:path.join(root,'docs/flowcharts/dev9/SkiDrive_Code_Flowcharts.pptx'),
 pythonExecutable:python,
 integrityValidatorPath:path.join(skill,'container_tools/inspect_presentation_package_integrity.py'),
 layoutValidatorPath:path.join(skill,'container_tools/inspect_presentation_layout_geometry.py'),
 layoutArgs:['--expected-slide-size-emu','18288000,13716000','--validate-bullet-geometry','--validate-heading-fit'],
 explicitTotalSlideCount:33,
 fontPolicy:{basis:'design',families:['Arial']},
 verifyArtifactToolImport:true,
 receiptPath:path.join(root,'build/flowcharts-dev9/final-validation.json')
});
console.log(JSON.stringify(result));
