'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync('companion/web/index.html','utf8'),elements=new Map();
class Element{
 constructor(){this.value='';this.hidden=false;this.disabled=false;this.required=false;this.dataset={};this.textContent='';this.handlers={};this.classList={toggle(){}};}
 addEventListener(event,handler){this.handlers[event]=handler;}
 replaceChildren(){}
 add(option){if(this.value==='')this.value=option.value;}
}
for(const match of html.matchAll(/id="([^"]+)"/g))elements.set(match[1],new Element());
const state={connected:true,saved:false,unlocked:true,wallet:0,slots:'00000',names:['Pessoal','C2','C3','C4','C5'],version:'0.7.0',flow:'backup',word:1,words:12,brightness:100,message:'Teste publico',pending:'create'};
const document={activeElement:null,getElementById(id){assert(elements.has(id),'Elemento ausente: '+id);return elements.get(id);},querySelector(){return {content:'test-token'};},querySelectorAll(){return [];}};
const sandbox={document,window:{addEventListener(){},scrollTo(){}},location:{hash:''},Option:class{constructor(text,value){this.text=text;this.value=value;}},setInterval(){},setTimeout(){},URL,Blob,TextDecoder,Uint8Array,fetch:async(path)=>({ok:true,json:async()=>path.includes('/ports')?[]:state}),navigator:{}};
vm.createContext(sandbox);vm.runInContext(fs.readFileSync('companion/web/app.js','utf8'),sandbox);
const el=id=>elements.get(id);
(async()=>{
 await sandbox.render(state);
 assert.equal(el('setupProgress').hidden,false);assert.equal(el('backupReady').disabled,true);assert.equal(el('webPinForm').hidden,true);
 state.word=12;await sandbox.render(state);assert.equal(el('backupReady').disabled,false);assert.equal(el('nextWord').disabled,true);
 state.flow='set-pin';await sandbox.render(state);assert.equal(el('webPinForm').hidden,false);assert.equal(el('webPinConfirm').required,true);
 el('webPin').value='42687359';el('webPinConfirm').value='58764291';
 await el('webPinForm').handlers.submit({preventDefault(){}});
 assert.match(el('pinFormError').textContent,/não conferem/);assert.equal(el('webPin').value,'');assert.equal(el('webPinConfirm').value,'');
 state.flow='unlock';await sandbox.render(state);assert.equal(el('pinConfirmationRow').hidden,true);assert.equal(el('webPinConfirm').required,false);
 el('webPin').value='42687359';state.flow='confirm-web-pin';await sandbox.render(state);
 assert.equal(el('webPinForm').hidden,true);assert.equal(el('webPin').value,'');assert.match(el('setupStepHelp').textContent,/Segure A/);
 console.log('OK: etapas de backup/PIN, confirmacao, erros visiveis e limpeza dos campos.');
})().catch(error=>{console.error(error);process.exitCode=1;});
