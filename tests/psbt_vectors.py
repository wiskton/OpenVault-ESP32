"""Known-public test seed only. Build fixtures and independently validate C++ signatures."""
from pathlib import Path
import subprocess,sys
import wallycore as w
root=Path(__file__).resolve().parents[1]
H=0x80000000
mnemonic=' '.join(['abandon']*11+['about'])
seed=w.bip39_mnemonic_to_seed512(mnemonic,'')
master=w.bip32_key_from_seed(seed,w.BIP32_VER_TEST_PRIVATE,0)
path=[84|H,1|H,H,0,0]
child=w.bip32_key_from_parent_path(master,path,0)
pub=w.bip32_key_get_pub_key(child)
script=b'\0\x14'+bytes(w.hash160(pub))
prev=w.tx_init(2,0,1,1)
w.tx_add_raw_input(prev,bytes(32),0xffffffff,0xffffffff,b'\x01\x00',None,0)
w.tx_add_raw_output(prev,100000,script,0)
tx=w.tx_init(2,0,1,1)
w.tx_add_raw_input(tx,w.tx_get_txid(prev),0,0xfffffffd,None,None,0)
# Public deterministic destination, different derivation from the wallet.
dest=w.bip32_key_from_parent_path(master,[84|H,1|H,H,0,10],0)
dest_script=b'\0\x14'+bytes(w.hash160(w.bip32_key_get_pub_key(dest)))
w.tx_add_raw_output(tx,99000,dest_script,0)
psbt=w.psbt_from_tx(tx,0,0)
w.psbt_set_input_utxo(psbt,0,prev)
paths=w.map_init(1,None)
value=bytes(w.bip32_key_get_fingerprint(master))+b''.join(x.to_bytes(4,'little') for x in path)
w.map_add(paths,pub,value)
w.psbt_set_input_keypaths(psbt,0,paths)
(root/'build/unsigned-test.psbt').write_text(w.psbt_to_base64(psbt,0))
# Rejection corpus: signature policy, ownership, tampering and output accounting.
fixtures=[]
def reject(name, obj=None, encoded=None):
    file=root/('build/reject-'+name+'.psbt')
    file.write_text(encoded if encoded is not None else w.psbt_to_base64(obj,0))
    fixtures.append(str(file))
def clone(): return w.psbt_from_base64(w.psbt_to_base64(psbt,0),0)
bad=clone();w.psbt_set_input_sighash(bad,0,2);reject('sighash-none',bad)
bad=clone();w.psbt_set_input_utxo(bad,0,None);reject('missing-utxo',bad)
bad=clone();wrong=w.map_init(1,None);w.map_add(wrong,pub,bytes(4)+value[4:]);w.psbt_set_input_keypaths(bad,0,wrong);reject('wrong-owner',bad)
bad=clone();wrong=w.map_init(1,None);w.map_add(wrong,pub,value[:-4]+(10).to_bytes(4,'little'));w.psbt_set_input_keypaths(bad,0,wrong);reject('wrong-derivation',bad)
bad=clone();w.psbt_set_input_keypaths(bad,0,w.map_init(0,None));reject('missing-keypath',bad)
prev_bad=w.tx_from_hex(w.tx_to_hex(prev,0),0)
w.tx_set_output_satoshi(prev_bad,0,100001)
bad=clone();w.psbt_set_input_utxo(bad,0,prev_bad);reject('wrong-prevtx',bad)
bad=clone();w.psbt_set_input_witness_utxo(bad,0,w.tx_output_init(100001,script));reject('wrong-witness',bad)
bad_tx=w.tx_from_hex(w.tx_to_hex(tx,0),0);w.tx_set_output_satoshi(bad_tx,0,1)
bad=w.psbt_from_tx(bad_tx,0,0);w.psbt_set_input_utxo(bad,0,prev);w.psbt_set_input_keypaths(bad,0,paths);reject('excessive-fee',bad)
import base64
reject('trailing-data',encoded=base64.b64encode(base64.b64decode(w.psbt_to_base64(psbt,0))+b'garbage').decode())
subprocess.run([str(root/'build/wallet_test'),str(root/'build/unsigned-test.psbt'),*fixtures],cwd=root,check=True)
signed=w.psbt_from_base64((root/'build/signed-test.psbt').read_text(),0)
# libwally verifies signatures while finalizing/extracting; also verify ECDSA directly below.
assert w.psbt_get_input_signatures_size(signed,0)==1
sig=w.psbt_get_input_signature(signed,0,0)
compact=w.ec_sig_from_der(sig[:-1])
sighash=w.tx_get_btc_signature_hash(tx,0,b'\x76\xa9\x14'+bytes(w.hash160(pub))+b'\x88\xac',100000,w.WALLY_SIGHASH_ALL,w.WALLY_TX_FLAG_USE_WITNESS)
w.ec_sig_verify(pub,sighash,w.EC_FLAG_ECDSA,compact)
# Verify with a separate implementation (OpenSSL), using the known-public test key.
spki=bytes.fromhex('3036301006072a8648ce3d020106052b8104000a032200')+bytes(pub)
(root/'build/test-public.der').write_bytes(spki)
(root/'build/test-signature.der').write_bytes(bytes(sig[:-1]))
(root/'build/test-sighash.bin').write_bytes(bytes(sighash))
subprocess.run(['openssl','pkeyutl','-verify','-pubin','-keyform','DER','-inkey',str(root/'build/test-public.der'),'-in',str(root/'build/test-sighash.bin'),'-sigfile',str(root/'build/test-signature.der'),'-pkeyopt','digest:sha256'],check=True,stdout=subprocess.DEVNULL)
print('OK: PSBT assinada no core C++; assinatura conferida por libwally Python e OpenSSL; nove PSBTs maliciosas rejeitadas.')
