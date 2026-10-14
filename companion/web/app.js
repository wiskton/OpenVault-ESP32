'use strict';
const $=id=>document.getElementById(id);
const token=document.querySelector('meta[name="openvault-token"]').content;
let current={},qrAddress='',qrUrl='',polling=false;
function notice(text){$('statusText').textContent=text;}
async function api(path,data){
 const options={headers:{'X-OpenVault-Token':token},cache:'no-store'};
 if(data!==undefined){options.method='POST';options.headers['Content-Type']='application/json';options.body=JSON.stringify(data);}
 const response=await fetch('/api/'+path,options);
 const payload=await response.json();
 if(!response.ok)throw new Error(payload.error||'Não foi possível concluir a solicitação.');
 return payload;
}
function show(view){
 if(!['overview','receive','transactions','device'].includes(view))view='overview';
 for(const section of document.querySelectorAll('[data-panel]'))section.hidden=section.dataset.panel!==view;
 for(const link of document.querySelectorAll('[data-view]'))link.classList.toggle('active',link.dataset.view===view);
}
for(const link of document.querySelectorAll('[data-view]'))link.addEventListener('click',()=>show(link.dataset.view));
for(const button of document.querySelectorAll('[data-open]'))button.addEventListener('click',()=>{location.hash=button.dataset.open;show(button.dataset.open);window.scrollTo(0,0);});
window.addEventListener('hashchange',()=>show(location.hash.slice(1)));show(location.hash.slice(1));
async function refreshPorts(){
 const list=await api('ports');const selected=$('portSelect').value;
 $('portSelect').replaceChildren();
 if(!list.length)$('portSelect').add(new Option('Nenhum aparelho identificado',''));
 for(const item of list)$('portSelect').add(new Option(item.label+' · '+item.path,item.path));
 if(list.some(item=>item.path===selected))$('portSelect').value=selected;
 else if(current.port)$('portSelect').value=current.port;
}
async function qr(address){
 if(address===qrAddress)return;
 const response=await fetch('/api/qr',{headers:{'X-OpenVault-Token':token},cache:'no-store'});
 if(!response.ok)throw new Error('Não foi possível gerar o QR local.');
 const image=await response.blob();if(qrUrl)URL.revokeObjectURL(qrUrl);
 qrUrl=URL.createObjectURL(image);qrAddress=address;$('qrImage').src=qrUrl;
}
function networkName(network){return network==='BTC'?'Bitcoin mainnet':network==='TEST'?'Bitcoin testnet':'Confirme na placa';}
async function render(state){
 current=state;
 $('usbBadge').textContent=state.connected?'● USB conectada':'USB desconectada';$('usbBadge').classList.toggle('connected',state.connected);
 $('walletStatePill').textContent=!state.connected?'Aguardando USB':state.saved?(state.unlocked?'Desbloqueada':'Bloqueada'):'Sem cofre salvo';
 $('walletHeading').textContent=!state.connected?'Vamos conectar.':state.saved?(state.unlocked?'Carteira desbloqueada.':'Sua carteira está salva.'):'Crie sua carteira na placa.';
 $('connectionHint').textContent=!state.connected?'Conecte o M5StickC PLUS2 pelo cabo USB-C de dados para começar.':state.saved?'Seu cofre permanece na placa. Use os botões para desbloquear e confirmar cada operação.':'Selecione Criar carteira no aparelho. Anote as palavras e defina o PIN para salvar o cofre.';
 $('setupBanner').hidden=state.saved===true;
 $('disconnectButton').hidden=!state.connected;
 $('connectButton').textContent=state.connected?'Reconectar dispositivo ↗':'Conectar dispositivo ↗';
 $('detailConnection').textContent=state.connected?(state.port||'USB conectada'):'Desconectado';
 $('detailVersion').textContent=state.version||'—';
 $('detailSaved').textContent=state.saved===null?'Aguardando conexão':state.saved?'Salvo localmente no dispositivo':'Ainda não criado';
 $('detailNetwork').textContent=networkName(state.network);
 if(document.activeElement!==$('brightness')){$('brightness').value=state.brightness||100;$('brightnessValue').textContent=$('brightness').value+' / 255';}
 const canUse=state.connected&&state.saved&&state.unlocked&&!state.pending;
 for(const id of ['requestAddress','requestXpub','overviewXpub','signButton'])$(id).disabled=!canUse;
 $('brightnessButton').disabled=!state.connected||!!state.pending;
 if(state.address){
  $('addressValue').textContent=state.address.value;$('addressIndex').textContent='Endereço #'+state.address.index+' · derivado e confirmado no dispositivo';
  $('addressNetwork').textContent=networkName(state.address.network);$('addressNetwork').hidden=false;
  $('copyAddress').disabled=false;
  await qr(state.address.value);$('qrPlaceholder').hidden=true;$('qrImage').hidden=false;
 }else{
  $('addressValue').textContent='Nenhum endereço confirmado.';$('addressIndex').textContent='';$('addressNetwork').hidden=true;
  $('copyAddress').disabled=true;$('qrImage').hidden=true;$('qrPlaceholder').hidden=false;qrAddress='';
 }
 $('xpubValue').hidden=!state.xpub;$('downloadXpub').hidden=!state.xpub;
 if(state.xpub)$('xpubValue').textContent='['+state.xpub.fingerprint+'/'+state.xpub.origin+'] '+state.xpub.value;
 $('downloadSigned').disabled=!state.signed;
 notice(state.message||'Confira o estado do dispositivo.');
}
async function refresh(){
 if(polling)return;polling=true;
 try{await render(await api('state'));}catch(error){notice(error.message);}finally{polling=false;}
}
function action(id,handler){$(id).addEventListener('click',async()=>{try{await handler();await refresh();}catch(error){notice(error.message);}});}
action('refreshPorts',refreshPorts);
action('connectButton',async()=>{await api('connect',{port:$('portSelect').value});notice('Conectando pela USB…');});
action('disconnectButton',()=>api('disconnect',{}));
action('requestAddress',()=>api('address',{}));
action('requestXpub',()=>api('xpub',{}));
action('overviewXpub',async()=>{location.hash='device';show('device');await api('xpub',{});});
action('brightnessButton',()=>api('brightness',{value:Number($('brightness').value)}));
$('brightness').addEventListener('input',()=>{$('brightnessValue').textContent=$('brightness').value+' / 255';});
action('copyAddress',async()=>{if(current.address){await navigator.clipboard.writeText(current.address.value);notice('Endereço copiado. Confira a rede antes de receber.');}});
function download(name,text,type='text/plain'){
 const url=URL.createObjectURL(new Blob([text],{type}));const a=document.createElement('a');a.href=url;a.download=name;document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);
}
action('downloadXpub',async()=>{if(current.xpub)download('openvault-watch-only.json',JSON.stringify(current.xpub,null,2),'application/json');});
action('downloadSigned',async()=>{if(current.signed)download('openvault-signed.psbt',current.signed+'\n');});
$('psbtFile').addEventListener('change',()=>{$('psbtFileName').textContent=$('psbtFile').files[0]?.name||'Escolher arquivo .psbt';});
function asBase64(bytes){let binary='';for(const byte of bytes)binary+=String.fromCharCode(byte);return btoa(binary);}
function hex(bytes){return Array.from(bytes,value=>value.toString(16).padStart(2,'0')).join('');}
action('signButton',async()=>{
 const file=$('psbtFile').files[0];if(!file)throw new Error('Escolha um arquivo PSBT.');if(file.size>32000)throw new Error('Arquivo PSBT muito grande.');
 const bytes=new Uint8Array(await file.arrayBuffer());
 const binary=bytes[0]===112&&bytes[1]===115&&bytes[2]===98&&bytes[3]===116&&bytes[4]===255;
 const psbt=binary?asBase64(bytes):new TextDecoder().decode(bytes).replace(/\s/g,'');
 if(psbt.length>24000)throw new Error('A PSBT excede 24.000 caracteres Base64.');
 const files=Array.from($('utxoFiles').files);if(files.length>8)throw new Error('Use no máximo oito transações anteriores.');
 const utxos=[];
 for(const item of files){if(item.size>65536)throw new Error('Transação anterior muito grande.');const raw=new Uint8Array(await item.arrayBuffer());const text=new TextDecoder().decode(raw).replace(/\s/g,'');utxos.push(/^[0-9a-f]+$/i.test(text)&&text.length%2===0?text:hex(raw));}
 await api('sign',{psbt,utxos});
});
refreshPorts().catch(error=>notice(error.message));refresh();setInterval(refresh,1500);
