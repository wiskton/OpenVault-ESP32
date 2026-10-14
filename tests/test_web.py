import importlib.util,json,http.client,threading,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('web',Path(__file__).resolve().parents[1]/'companion/web_server.py')
web=importlib.util.module_from_spec(spec);spec.loader.exec_module(web)
class FakeBridge:
    def __init__(self):self.calls=[];self.saved=False
    def continue_flow(self,*args):self.calls.append(args)
    def request_delete(self,slot):self.calls.append(("DELETE",slot))
    def snapshot(self):return {'connected':True,'address':None,'saved':self.saved}
    def send(self,*args):self.calls.append(args)
class WebTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server=web.ThreadingHTTPServer(('127.0.0.1',0),web.Handler);cls.server.bridge=FakeBridge()
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True);cls.thread.start()
    @classmethod
    def tearDownClass(cls):cls.server.shutdown();cls.server.server_close();cls.thread.join()
    def request(self,path,method='GET',data=None,headers=None):
        h={'X-OpenVault-Token':web.TOKEN,'Content-Type':'application/json'};h.update(headers or {})
        c=http.client.HTTPConnection('127.0.0.1',self.server.server_port);c.request(method,path,body=json.dumps(data) if data is not None else None,headers=h)
        r=c.getresponse();result=(r.status,r.read(),dict(r.getheaders()));c.close();return result
    def test_static_and_private_files(self):
        status,body,headers=self.request('/');self.assertEqual(status,200);self.assertIn(web.TOKEN.encode(),body);self.assertIn("frame-ancestors 'none'",headers['Content-Security-Policy'])
        for path in ['/../README.md','/backups/flash-original.bin','/api/seed','/api/pin']:
            self.assertEqual(self.request(path)[0],404)
    def test_token_origin_host(self):
        self.assertEqual(self.request('/api/state')[0],200)
        for headers in [{'X-OpenVault-Token':''},{'Origin':'https://evil.example'},{'Host':'evil.example'},{'Sec-Fetch-Site':'cross-site'}]:
            self.assertEqual(self.request('/api/state',headers=headers)[0],403)
    def test_request_validation_and_physical_command(self):
        for value in [[],{'value':True},{'value':201},{'value':0}]:self.assertEqual(self.request('/api/brightness','POST',value)[0],400)
        self.assertEqual(self.request('/api/brightness','POST',{'value':100})[0],200)
        self.assertEqual(self.server.bridge.calls[-1],('BRIGHTNESS 100','brightness'))
        self.assertEqual(self.request('/api/sign','POST',{'psbt':'not a psbt'})[0],400)
        self.assertEqual(self.request('/api/create','POST',{})[0],400)
    def test_creation_and_management(self):
        bridge=self.server.bridge;bridge.saved=False
        for body in [{'network':'BTC','words':True},{'network':'BTC','words':13},{'network':'BTC\nLOCK','words':12}]:
            self.assertEqual(self.request('/api/create','POST',body)[0],400)
        self.assertEqual(self.request('/api/create','POST',{'network':'TEST','words':24})[0],200)
        self.assertEqual(bridge.calls[-1],('CREATE TEST 24','create'))
        bridge.saved=True
        try:self.assertEqual(self.request('/api/create','POST',{'network':'BTC','words':12})[0],400)
        finally:bridge.saved=False
        for route,command in [('unlock','UNLOCK'),('lock','LOCK'),('backup','BACKUP'),('change-pin','CHANGE_PIN')]:
            self.assertEqual(self.request('/api/'+route,'POST',{})[0],200)
            self.assertEqual(bridge.calls[-1],(command,route))
    def test_multiple_wallet_selection(self):
        for slot in [True,-1,5,'1']:
            self.assertEqual(self.request('/api/select-wallet','POST',{'slot':slot})[0],400)
        for slot in range(5):
            self.assertEqual(self.request('/api/select-wallet','POST',{'slot':slot})[0],200)
            self.assertEqual(self.server.bridge.calls[-1],('SELECT '+str(slot),'switch'))
        b=web.Bridge(auto=False)
        try:
            b.line('STATUS saved unlocked BTC 73c5da0a wallet:0 slots:11000 flow:idle brightness:100')
            b.data.update(address={'value':'old'},xpub={'value':'old'},signed='old')
            b.line('STATUS saved locked wallet:1 slots:11000 flow:idle brightness:100')
            self.assertEqual(b.snapshot()['wallet'],1);self.assertEqual(b.snapshot()['slots'],'11000')
            for key in ('address','xpub','signed','fingerprint','network'):self.assertIsNone(b.snapshot()[key])
            b.line('WALLET_SELECTED');self.assertFalse(b.snapshot()['unlocked'])
        finally:b.running=False;b.worker.join(timeout=2)
    def test_guided_pin_and_backup_routes(self):
        for value in ['','123','abcdefgh',12345678,'123456789','1234567\n']:
            self.assertEqual(self.request('/api/set-pin','POST',{'pin':value,'confirmation':value})[0],400)
        self.assertEqual(self.request('/api/set-pin','POST',{'pin':'42687359','confirmation':'58764291'})[0],400)
        self.assertEqual(self.request('/api/set-pin','POST',{'pin':'42687359','confirmation':'42687359'})[0],200)
        self.assertEqual(self.server.bridge.calls[-1],('SET_PIN 42687359',{'set-pin'}))
        self.assertEqual(self.request('/api/unlock-pin','POST',{'pin':'42687359'})[0],200)
        self.assertEqual(self.server.bridge.calls[-1],('UNLOCK_PIN 42687359',{'unlock'}))
        for route,command in [('next-word','NEXT_WORD'),('backup-ready','BACKUP_READY')]:
            self.assertEqual(self.request('/api/'+route,'POST',{})[0],200)
            self.assertEqual(self.server.bridge.calls[-1],(command,{'backup'}))
    def test_continuation_guards_and_no_pin_state(self):
        b=web.Bridge(auto=False)
        try:
            with self.assertRaises(ValueError):b.continue_flow('SET_PIN 42687359',{'set-pin'})
            b.data.update(connected=True,flow='backup',pending='create')
            with self.assertRaises(ValueError):b.continue_flow('SET_PIN 42687359',{'set-pin'})
            b.data['flow']='set-pin';b.continue_flow('SET_PIN 42687359',{'set-pin'})
            self.assertEqual(b.snapshot()['flow'],'confirm-web-pin');self.assertNotIn('42687359',json.dumps(b.snapshot()))
            with self.assertRaises(ValueError):b.continue_flow('SET_PIN 58764291',{'set-pin'})
        finally:b.running=False;b.worker.join(timeout=2)
    def test_delete_confirmation_and_slot_guards(self):
        for body in [{},{'slot':0,'confirmation':'DELETE'},{'slot':0,'backup_acknowledged':True}]:
            self.assertEqual(self.request('/api/delete-wallet','POST',body)[0],400)
        self.assertEqual(self.request('/api/delete-wallet','POST',{'slot':0,'confirmation':'DELETE','backup_acknowledged':True})[0],200)
        self.assertEqual(self.server.bridge.calls[-1],('DELETE',0))
        b=web.Bridge(auto=False)
        try:
            b.data.update(connected=True,saved=True,unlocked=True,flow='idle',wallet=1,slots='11000',fingerprint='public-fixture',address={'value':'old'},xpub={'value':'old'},signed='old')
            for invalid in [True,-1,0,5,'1']:
                with self.assertRaises(ValueError):b.request_delete(invalid)
            b.data['unlocked']=False
            with self.assertRaises(ValueError):b.request_delete(1)
            b.data['unlocked']=True;b.request_delete(1)
            self.assertEqual(b.commands.get_nowait(),'DELETE 1');self.assertTrue(b.snapshot()['saved'])
            b.line('CANCELED');self.assertTrue(b.snapshot()['saved']);self.assertIsNone(b.snapshot()['pending'])
            b.line('WALLET_DELETED 0');self.assertTrue(b.snapshot()['saved'])
            b.line('WALLET_DELETED 1');self.assertFalse(b.snapshot()['saved']);self.assertEqual(b.snapshot()['slots'],'10000')
            for key in ('address','xpub','signed','fingerprint','network'):self.assertIsNone(b.snapshot()[key])
        finally:b.running=False;b.worker.join(timeout=2)
    def test_wallet_names(self):
        for name in ['', ' Name','Name ', 'Nome\nLOCK','<script>','a'*19,'João']:
            self.assertEqual(self.request('/api/rename-wallet','POST',{'name':name})[0],400)
        self.assertEqual(self.request('/api/rename-wallet','POST',{'name':'Reserva BTC'})[0],200)
        self.assertEqual(self.server.bridge.calls[-1],('NAME Reserva BTC','rename'))
        b=web.Bridge(auto=False)
        try:
            names=['Pessoal','Reserva BTC','Empresa','Viagem','Familia']
            encoded=','.join(name.encode().hex() for name in names)
            b.line('STATUS saved locked wallet:1 slots:11000 names:'+encoded+' flow:idle brightness:100')
            self.assertEqual(b.snapshot()['names'],names)
            b.reset_connection();self.assertEqual(b.snapshot()['names'][1],'Carteira 2')
        finally:b.running=False;b.worker.join(timeout=2)
    def test_watch_only_export(self):
        self.assertEqual(self.request('/api/export-wallet')[0],400)
        state={'saved':True,'wallet':1,'network':'BTC','fingerprint':'73c5da0a','names':['Pessoal','Reserva BTC','C3','C4','C5'],'xpub':{'fingerprint':'73c5da0a','origin':'84h/0h/0h','value':'xpub-public-test-fixture'}}
        data=web.public_wallet_export(state)
        self.assertEqual(data['name'],'Reserva BTC');self.assertEqual(data['wallet_slot'],2)
        self.assertFalse(data['contains_private_keys']);self.assertFalse(data['can_sign']);self.assertFalse(data['recovery_backup'])
        self.assertFalse(any(key in data for key in ('seed','pin','mnemonic','entropy','private_key')))
        self.assertIn('/0/*)',data['receive_descriptor']);self.assertIn('/1/*)',data['change_descriptor'])
        state['fingerprint']='different'
        with self.assertRaises(ValueError):web.public_wallet_export(state)
    def test_setup_progress_contains_no_secrets(self):
        b=web.Bridge(auto=False)
        try:
            b.data['pending']='create';b.line('STATUS empty unlocked TEST 73c5da0a flow:backup brightness:100')
            self.assertEqual(b.snapshot()['flow'],'backup');self.assertEqual(b.snapshot()['pending'],'create')
            b.line('WALLET_SAVED');self.assertTrue(b.snapshot()['saved']);self.assertIsNone(b.snapshot()['pending'])
            b.line('CANCELED');self.assertIsNone(b.snapshot()['pending'])
            b.data['pending']='unlock';b.line('UNLOCKED');self.assertIsNone(b.snapshot()['pending'])
            self.assertFalse(any(k in b.snapshot() for k in ('seed','pin','mnemonic','entropy')))
        finally:b.running=False;b.worker.join(timeout=2)
    def test_public_cache_erased_on_wallet_removal(self):
        b=web.Bridge(auto=False)
        try:
            b.data.update(saved=True,fingerprint='old',network='BTC',address={'value':'old'},xpub={'value':'old'},signed='old')
            b.line('STATUS empty locked brightness:100')
            for key in ['address','xpub','signed','fingerprint','network']:self.assertIsNone(b.snapshot()[key])
            b.line('STATUS saved unlocked TEST 73c5da0a brightness:120')
            self.assertEqual(b.snapshot()['network'],'TEST');self.assertEqual(b.snapshot()['brightness'],120)
            b.reset_connection();self.assertIsNone(b.snapshot()['network'])
        finally:b.running=False;b.worker.join(timeout=2)
if __name__=='__main__':unittest.main()
