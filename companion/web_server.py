#!/usr/bin/env python3
"""Loopback USB bridge. Seed stays on device; web PIN is volatile and physically authorized."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse,base64,json,secrets,threading,time,queue,io,re
import serial
from serial.tools import list_ports

WEB=Path(__file__).resolve().parent/'web'
TOKEN=secrets.token_hex(32)

def ports():
    return [{'path':p.device,'label':p.description} for p in list_ports.comports() if p.vid==0x1a86 and ('ttyACM' in p.device or 'ttyUSB' in p.device or p.device.startswith('COM'))]

def public_wallet_export(state):
    xpub=state.get('xpub');slot=state.get('wallet');network=state.get('network')
    if not state.get('saved') or not xpub or type(slot) is not int or not 0<=slot<5:raise ValueError('Autorize primeiro a exportacao da xpub na placa')
    expected='84h/0h/0h' if network=='BTC' else '84h/1h/0h'
    if network not in ('BTC','TEST') or xpub['origin']!=expected or xpub['fingerprint']!=state.get('fingerprint'):raise ValueError('Dados publicos inconsistentes; exporte novamente')
    if not xpub['value'].startswith('xpub' if network=='BTC' else 'tpub'):raise ValueError('Rede da chave publica divergente')
    names=state.get('names') or [f'Carteira {i+1}' for i in range(5)]
    return {'format':'openvault-watch-only','version':1,'name':names[slot],'wallet_slot':slot+1,'network':'bitcoin' if network=='BTC' else 'testnet','fingerprint':xpub['fingerprint'],'derivation':'m/'+expected,'xpub':xpub['value'],'address_type':'p2wpkh','receive_descriptor':f"wpkh([{xpub['fingerprint']}/{expected}]{xpub['value']}/0/*)",'change_descriptor':f"wpkh([{xpub['fingerprint']}/{expected}]{xpub['value']}/1/*)",'confirmed_address':state.get('address'),'can_sign':False,'contains_private_keys':False,'recovery_backup':False}

class Bridge:
    def __init__(self,auto=True):
        self.lock=threading.RLock();self.commands=queue.Queue();self.port=None;self.active_port=None
        self.connection=None;self.running=True
        self.data={'connected':False,'port':None,'saved':None,'unlocked':False,'network':None,'fingerprint':None,'address':None,'xpub':None,'signed':None,'pending':None,'message':'Conecte o dispositivo pela USB.','brightness':100,'version':None,'flow':'idle','wallet':None,'slots':'00000','names':[f'Carteira {i+1}' for i in range(5)],'word':0,'words':0}
        found=ports()
        if auto and len(found)==1:self.port=found[0]['path']
        self.worker=threading.Thread(target=self.loop,daemon=True);self.worker.start()
    def snapshot(self):
        with self.lock:return dict(self.data)
    def connect(self,path):
        if path not in [p['path'] for p in ports()]:raise ValueError('Porta USB nao identificada como dispositivo compativel.')
        with self.lock:self.port=path
    def disconnect(self):
        with self.lock:self.port=None
    def send(self,command,pending):
        with self.lock:
            if not self.data['connected']:raise ValueError('Dispositivo desconectado.')
            if self.data['pending']:raise ValueError('Conclua ou cancele a solicitacao atual na placa.')
            self.data.update(pending=pending,message='Aguardando confirmacao nos botoes do dispositivo.')
            if pending=='sign':self.data['signed']=None
            self.commands.put(command)
    def request_delete(self,slot):
        with self.lock:
            if type(slot) is not int or not 0<=slot<5 or slot!=self.data['wallet']:raise ValueError('A carteira ativa mudou. Abra a confirmacao novamente.')
            if not self.data['saved'] or not self.data['unlocked'] or self.data['flow']!='idle':raise ValueError('Selecione e desbloqueie a carteira antes de excluir.')
            self.send('DELETE '+str(slot),'delete')
    def continue_flow(self,command,allowed_flows):
        with self.lock:
            if not self.data['connected'] or self.data['flow'] not in allowed_flows:raise ValueError('Esta etapa nao esta disponivel. Confira o progresso no painel.')
            self.commands.put(command)
            if command.startswith(('SET_PIN ','UNLOCK_PIN ')):self.data['flow']='confirm-web-pin'
            self.data['message']='Pedido enviado. Confira a placa para continuar.'
    def line(self,line):
        with self.lock:
            if line.startswith('STATUS '):
                parts=line.split();saved=parts[1]=='saved';unlocked=parts[2]=='unlocked'
                fp=parts[4] if len(parts)>4 and parts[3] in ('BTC','TEST') else None
                if (self.data['fingerprint'] and fp and fp!=self.data['fingerprint']) or (self.data['saved'] and not saved):
                    self.data.update(address=None,xpub=None,signed=None)
                new_wallet=next((int(part[7:]) for part in parts if part.startswith('wallet:')),self.data['wallet'])
                first_status=self.data['saved'] is None or new_wallet!=self.data['wallet']
                if new_wallet!=self.data['wallet']:self.data.update(address=None,xpub=None,signed=None,fingerprint=None,network=None)
                self.data.update(saved=saved,unlocked=unlocked,wallet=new_wallet)
                if first_status and not self.data['pending']:self.data['message']='Carteira vazia. Clique em Criar carteira no painel.' if not saved else ('Carteira pronta. Escolha uma operacao no painel.' if unlocked else 'Carteira salva. Clique em Desbloquear no painel.')
                if not saved:self.data.update(fingerprint=None,network=None)
                if fp:self.data.update(fingerprint=fp,network=parts[3])
                for part in parts:
                    if part.startswith('word:'):self.data['word']=int(part.split(':',1)[1])
                    if part.startswith('words:'):self.data['words']=int(part.split(':',1)[1])
                    if part.startswith('names:'):
                        encoded=part.split(':',1)[1].split(',')
                        if len(encoded)==5:
                            labels=[bytes.fromhex(item).decode('ascii') for item in encoded]
                            if all(re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9 _-]{0,17}',item) for item in labels):self.data['names']=labels
                    if part.startswith('slots:'):self.data['slots']=part.split(':',1)[1]
                    if part.startswith('flow:'):self.data['flow']=part.split(':',1)[1]
                    if part.startswith('brightness:'):
                        self.data['brightness']=int(part.split(':',1)[1])
            elif line.startswith('OPENVAULT '):
                self.data['version']=line.split()[1]
                self.data.update(pending=None,address=None,xpub=None,signed=None)
            elif line.startswith('ADDRESS '):
                parts=line.split()
                if len(parts)==4 and parts[1] in ('BTC','TEST') and parts[2].isdigit() and parts[3].startswith(('bc1','tb1')):
                    self.data.update(address={'network':parts[1],'index':int(parts[2]),'value':parts[3]},pending=None,message='Endereco confirmado no dispositivo.')
            elif line.startswith('XPUB '):
                parts=line.split()
                if len(parts)==4 and parts[3].startswith(('xpub','tpub')):
                    self.data.update(xpub={'fingerprint':parts[1],'origin':parts[2],'value':parts[3]},pending=None,message='Chave publica exportada. A seed permanece na placa.')
            elif line.startswith('SIGNED '):
                text=line[7:]
                try:ok=base64.b64decode(text,validate=True).startswith(b'psbt\xff')
                except ValueError:ok=False
                if ok:self.data.update(signed=text,pending=None,message='PSBT assinada. Baixe o arquivo; nada foi transmitido a rede.')
                else:self.data.update(pending=None,message='Resposta de assinatura invalida.')
            elif line.startswith('WALLET_DELETED '):
                slot=line.split()[1]
                if slot.isdigit() and int(slot)==self.data['wallet']:
                    self.data.update(pending=None,saved=False,unlocked=False,address=None,xpub=None,signed=None,network=None,fingerprint=None,flow='idle',message='Carteira excluida do dispositivo. O espaco esta disponivel.')
                    index=int(slot);names=list(self.data['names']);names[index]=f'Carteira {index+1}';self.data['names']=names
                    slots=list(self.data['slots']);slots[index]='0';self.data['slots']=''.join(slots)
            elif line=='WALLET_SELECTED':self.data.update(pending=None,address=None,xpub=None,signed=None,fingerprint=None,network=None,unlocked=False,saved=None,message='Carteira selecionada. Desbloqueie para operar ou crie se estiver vazia.')
            elif line=='WALLET_SAVED':self.data.update(saved=True,unlocked=True,pending=None,message='Carteira salva na placa. Guarde as palavras de recuperacao.')
            elif line=='UNLOCKED':self.data.update(unlocked=True,pending=None,message='Carteira desbloqueada. Escolha uma operacao no painel.')
            elif line=='BACKUP_DONE':self.data.update(pending=None,message='Consulta do backup concluida na placa.')
            elif line=='REVIEW_READY':self.data['message']='Revise cada destino, valor e taxa na placa. Segure A somente se tudo conferir.'
            elif line.startswith('ERROR '):self.data.update(pending=None,message=line[6:].replace('_',' '))
            elif line.startswith(('CANCELED','LOCKED ')):
                self.data.update(pending=None,message='Solicitacao cancelada ou carteira bloqueada na placa.')
            elif line.startswith('CONFIG_OK '):self.data.update(pending=None,message='Configuracao confirmada e salva no dispositivo.')
    def reset_connection(self):
        if self.connection:
            try:self.connection.close()
            except serial.SerialException:pass
        self.connection=None;self.active_port=None
        while not self.commands.empty():
            try:self.commands.get_nowait()
            except queue.Empty:break
        with self.lock:self.data.update(connected=False,port=None,saved=None,unlocked=False,pending=None,address=None,xpub=None,signed=None,fingerprint=None,network=None,version=None,flow="idle",wallet=None,slots="00000",names=[f"Carteira {i+1}" for i in range(5)],word=0,words=0)
    def loop(self):
        pending=bytearray();last_poll=0
        while self.running:
            with self.lock:port=self.port
            if port!=self.active_port:
                self.reset_connection();pending.clear()
            if not port:time.sleep(.2);continue
            try:
                if self.connection is None:
                    s=serial.Serial(port=None,baudrate=115200,timeout=.1,write_timeout=5)
                    s.dtr=False;s.rts=False;s.port=port;s.open();self.connection=s;self.active_port=port
                    with self.lock:self.data.update(connected=True,port=port,message='USB conectada. Desbloqueie a carteira na placa.')
                    s.write(b'PING\nSTATUS\n');last_poll=time.monotonic()
                while not self.commands.empty():self.connection.write((self.commands.get_nowait()+'\nSTATUS\n').encode('ascii'))
                if time.monotonic()-last_poll>=2:self.connection.write(b'STATUS\n');last_poll=time.monotonic()
                data=self.connection.read(2048)
                if data:
                    pending.extend(data)
                    if len(pending)>40000:pending.clear();raise ValueError('Resposta serial muito grande')
                    while b'\n' in pending:
                        line,_,rest=pending.partition(b'\n');pending=bytearray(rest)
                        self.line(line.decode('ascii',errors='replace').strip())
            except (serial.SerialException,OSError,ValueError) as exc:
                self.reset_connection()
                with self.lock:self.data['message']='Sem acesso a USB. Confira o cabo, a porta e a permissao do usuario.'
                time.sleep(1)

class Handler(BaseHTTPRequestHandler):
    server_version='OpenVaultLocal/0.6'
    def log_message(self,fmt,*args):pass
    def allowed_origin(self):
        host=self.headers.get('Host','')
        allowed={f'127.0.0.1:{self.server.server_port}',f'localhost:{self.server.server_port}'}
        if host not in allowed:return False
        origin=self.headers.get('Origin')
        return (not origin or origin in {'http://'+h for h in allowed}) and self.headers.get('Sec-Fetch-Site','none') not in ('cross-site',)
    def authorized(self):
        return self.allowed_origin() and secrets.compare_digest(self.headers.get('X-OpenVault-Token',''),TOKEN)
    def reply(self,code,data,content_type='application/json; charset=utf-8'):
        body=json.dumps(data,ensure_ascii=False).encode() if isinstance(data,(dict,list)) else data
        self.send_response(code);self.send_header('Content-Type',content_type);self.send_header('Content-Length',str(len(body)))
        self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff')
        self.send_header('X-Frame-Options','DENY');self.send_header('Referrer-Policy','no-referrer')
        self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' blob: data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'none'")
        self.end_headers();self.wfile.write(body)
    def do_GET(self):
        if not self.allowed_origin():return self.reply(403,{'error':'Origem nao permitida'})
        if self.path.startswith('/api/'):
            if not self.authorized():return self.reply(403,{'error':'Sessao local invalida'})
            if self.path=='/api/state':return self.reply(200,self.server.bridge.snapshot())
            if self.path=='/api/export-wallet':
                try:return self.reply(200,public_wallet_export(self.server.bridge.snapshot()))
                except ValueError as exc:return self.reply(400,{'error':str(exc)})
            if self.path=='/api/ports':return self.reply(200,ports())
            if self.path=='/api/qr':
                address=self.server.bridge.snapshot().get('address')
                if not address:return self.reply(404,{'error':'Nenhum endereco confirmado'})
                import qrcode
                from qrcode.image.svg import SvgPathImage
                image=qrcode.make(address['value'],image_factory=SvgPathImage,box_size=8,border=4)
                output=io.BytesIO();image.save(output)
                return self.reply(200,output.getvalue(),'image/svg+xml')
            return self.reply(404,{'error':'Rota inexistente'})
        assets={'/':('index.html','text/html; charset=utf-8'),'/app.js':('app.js','text/javascript; charset=utf-8'),'/style.css':('style.css','text/css; charset=utf-8')}
        if self.path not in assets:return self.reply(404,{'error':'Pagina inexistente'})
        file,mime=assets[self.path];body=(WEB/file).read_bytes()
        if file=='index.html':body=body.replace(b'__LOCAL_TOKEN__',TOKEN.encode())
        self.reply(200,body,mime)
    def do_POST(self):
        if not self.authorized():return self.reply(403,{'error':'Origem ou sessao local invalida'})
        if self.headers.get('Content-Type','').split(';')[0]!='application/json':return self.reply(415,{'error':'Use JSON'})
        try:
            length=int(self.headers.get('Content-Length','0'))
            if length<0 or length>262144:raise ValueError('Mensagem muito grande')
            body=json.loads(self.rfile.read(length) or b'{}')
            if not isinstance(body,dict):raise ValueError('Objeto JSON esperado')
            bridge=self.server.bridge
            if self.path=='/api/connect':bridge.connect(str(body.get('port','')))
            elif self.path=='/api/disconnect':bridge.disconnect()
            elif self.path=='/api/select-wallet':
                slot=body.get('slot')
                if type(slot) is not int or not 0<=slot<5:raise ValueError('Escolha uma das cinco carteiras')
                bridge.send('SELECT '+str(slot),'switch')
            elif self.path=='/api/next-word':bridge.continue_flow('NEXT_WORD',{'backup'})
            elif self.path=='/api/backup-ready':bridge.continue_flow('BACKUP_READY',{'backup'})
            elif self.path=='/api/set-pin' or self.path=='/api/unlock-pin':
                pin=body.get('pin');unlocking=self.path=='/api/unlock-pin'
                if not isinstance(pin,str) or not re.fullmatch(r'[0-9]{8}',pin):raise ValueError('O PIN deve ter exatamente 8 numeros')
                if not unlocking and pin!=body.get('confirmation'):raise ValueError('Os PINs nao conferem')
                bridge.continue_flow(('UNLOCK_PIN ' if unlocking else 'SET_PIN ')+pin,{'unlock'} if unlocking else {'set-pin'})
                pin=None;body.clear()
            elif self.path=='/api/delete-wallet':
                if body.get('confirmation')!='DELETE' or body.get('backup_acknowledged') is not True:raise ValueError('Confirme a exclusao e o aviso de recuperacao no modal.')
                bridge.request_delete(body.get('slot'))
            elif self.path=='/api/rename-wallet':
                name=body.get('name')
                if not isinstance(name,str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9 _-]{0,17}',name) or name.endswith(' '):raise ValueError('Use 1 a 18 letras sem acento, numeros, espacos, - ou _')
                bridge.send('NAME '+name,'rename')
            elif self.path=='/api/create':
                network=body.get('network');words=body.get('words')
                if network not in ('BTC','TEST') or type(words) is not int or words not in (12,24):raise ValueError('Escolha Bitcoin/testnet e 12 ou 24 palavras')
                if bridge.snapshot().get('saved') is not False:raise ValueError('A criacao exige dispositivo conectado sem cofre salvo')
                bridge.send(f'CREATE {network} {words}','create')
            elif self.path in ('/api/unlock','/api/lock','/api/backup','/api/change-pin'):
                operation=self.path.rsplit('/',1)[1]
                bridge.send({'unlock':'UNLOCK','lock':'LOCK','backup':'BACKUP','change-pin':'CHANGE_PIN'}[operation],operation)
            elif self.path=='/api/address':bridge.send('ADDRESS','address')
            elif self.path=='/api/xpub':bridge.send('REQUEST_XPUB','xpub')
            elif self.path=='/api/brightness':
                level=body.get('value')
                if type(level) is not int or not 40<=level<=200:raise ValueError('Brilho deve estar entre 40 e 200')
                bridge.send('BRIGHTNESS '+str(level),'brightness')
            elif self.path=='/api/sign':
                encoded=body.get('psbt','')
                if not isinstance(encoded,str) or len(encoded)>24000:raise ValueError('PSBT muito grande')
                data=base64.b64decode(encoded,validate=True)
                if not data.startswith(b'psbt\xff'):raise ValueError('PSBT invalida')
                utxos=body.get('utxos',[])
                if not isinstance(utxos,list) or len(utxos)>8:raise ValueError('Maximo oito transacoes anteriores')
                if utxos:
                    import wallycore as w
                    p=w.psbt_from_base64(encoded,0)
                    for rawhex in utxos:
                        if not isinstance(rawhex,str) or len(rawhex)>131072:raise ValueError('UTXO muito grande')
                        tx=w.tx_from_hex(rawhex,0)
                        for i in range(w.psbt_get_num_inputs(p)):
                            if bytes(w.psbt_get_input_previous_txid(p,i))==bytes(w.tx_get_txid(tx)):w.psbt_set_input_utxo(p,i,tx)
                    encoded=w.psbt_to_base64(p,0)
                if len(encoded)>24000:raise ValueError('PSBT com UTXOs excede 24.000 caracteres')
                bridge.send('SIGN '+encoded,'sign')
            else:return self.reply(404,{'error':'Rota inexistente'})
            self.reply(200,{'ok':True})
        except (ValueError,TypeError,KeyError) as exc:self.reply(400,{'error':str(exc)})


def main():
    parser=argparse.ArgumentParser(description='Painel OpenVault local; seed na placa e PIN web autorizado fisicamente.')
    parser.add_argument('--port',type=int,default=8765);parser.add_argument('--no-auto-connect',action='store_true')
    args=parser.parse_args()
    bridge=Bridge(auto=not args.no_auto_connect)
    server=ThreadingHTTPServer(('127.0.0.1',args.port),Handler);server.bridge=bridge
    print(f'OpenVault local: http://127.0.0.1:{args.port}',flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:bridge.running=False;bridge.worker.join(timeout=2);bridge.reset_connection();server.server_close()
if __name__=='__main__':main()
