#!/usr/bin/env python3
"""Check Git blobs without printing matched secrets. Heuristic, not an audit."""
import argparse,fnmatch,re,subprocess,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
WORDS=set((ROOT/'scripts/data/bip39_english.txt').read_text().split())
PUBLIC_PHRASE=' '.join(['abandon']*11+['about'])
PRIVATE=['backups/*','secrets/*','private/*','local-data/*','build/*','.pio/*','.venv/*','.deps/*','*.bin','*.elf','*.dump','*.nvs','*.psbt','*.key','*.pem','*.p12','*.pfx','*.keystore','*.hex','*watch-only*.json','openvault-*-consulta.json','*wallet-backup*','*seed*.txt','*seed*.json','*mnemonic*.txt','*mnemonic*.json','*pin*.txt']
PATTERNS=[('private key',r'\b(?:xprv|tprv|yprv|zprv)[1-9A-HJ-NP-Za-km-z]{80,}\b'),('WIF',r'\b(?:[5KL][1-9A-HJ-NP-Za-km-z]{50,51}|[9c][1-9A-HJ-NP-Za-km-z]{50,51})\b'),('public wallet export',r'\b(?:xpub|tpub|ypub|zpub)[1-9A-HJ-NP-Za-km-z]{80,}\b'),('private key PEM',r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),('access token',r'\b(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,}|AKIA[A-Z0-9]{16}|sk-[A-Za-z0-9_-]{32,})\b')]
def findings(path,data):
    result=[];base=Path(path).name
    if any(part in {'backups','secrets','private','local-data','.pio','.venv','.deps','build'} for part in Path(path).parts[:-1]) or (base.startswith('.env') and base!='.env.example') or any(fnmatch.fnmatch(path,pat) or fnmatch.fnmatch(base,pat) for pat in PRIVATE):result.append('private file')
    if b'\0' in data:return result+['binary file']
    text=data.decode('utf8',errors='replace')
    for kind,pattern in PATTERNS:
        if re.search(pattern,text):result.append(kind)
    for match in re.finditer(r'["\x27](pin|password|mnemonic|seed|private_key|api_key)["\x27]\s*:\s*["\x27]([^"\x27\n]+)["\x27]',text,re.I):
        value=match[2]
        if path.startswith('tests/') and (value in ('42687359','58764291','12345678',PUBLIC_PHRASE)):continue
        if len(value)>=8 and not value.startswith(('Ex.', 'Use ', 'PIN ', 'O PIN ')):result.append('literal credential')
    for match in re.finditer(r'\b[a-z]+(?:\s+[a-z]+){11,}\b',text):
        run=[]
        for word in match[0].split()+['!']:
            if word in WORDS:run.append(word)
            else:
                if len(run)>=12 and not (path.startswith('tests/') and ' '.join(run)==PUBLIC_PHRASE):result.append('possible BIP39 words')
                run=[]
    return sorted(set(result))
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
def entries(history=False):
    rows=[]
    if history:
        for revision in git('rev-list','--all').decode().split():rows.extend(git('ls-tree','-rz',revision).split(b'\0'))
    else:rows=git('ls-files','--stage','-z').split(b'\0')
    seen=set()
    for row in rows:
        if not row:continue
        meta,path=row.split(b'\t',1);parts=meta.split();oid=parts[2] if history else parts[1]
        path=path.decode('utf8',errors='replace')
        if (oid,path) in seen:continue
        seen.add((oid,path));yield path,git('cat-file','blob',oid.decode())
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--history',action='store_true');args=parser.parse_args()
    failed=False;count=0
    for path,data in entries(args.history):
        count+=1
        # The public BIP39 dictionary is data, not a generated recovery phrase.
        if path=='scripts/data/bip39_english.txt' and hashlib.sha256(data).hexdigest()=='2f5eed53a4727b4bf8880d8f3f199efc90e58503646d9ff8eff3a2ed3b24dbda':continue
        for kind in findings(path,data):print(f'BLOCKED: {path}: {kind} (value hidden)');failed=True
    print(f'Checked {count} Git blobs; '+('review required.' if failed else 'no matches found.'))
    return int(failed)
if __name__=='__main__':sys.exit(main())
