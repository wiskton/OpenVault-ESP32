#!/usr/bin/env python3
"""Local serial companion. No secrets, network, signing approval or broadcast on the host."""
import argparse
import base64
from pathlib import Path
import time
import serial
from serial.tools import list_ports


def choose_port(explicit=None):
    if explicit:
        return explicit
    ports=[p.device for p in list_ports.comports() if p.vid==0x1a86]
    if len(ports)!=1:
        raise SystemExit('Informe --port. Nenhum dispositivo unico identificado.')
    return ports[0]


def load_psbt(path, utxos):
    data=Path(path).read_bytes()
    if data.startswith(b'psbt\xff'):
        encoded=base64.b64encode(data).decode('ascii')
    else:
        encoded=b''.join(data.split()).decode('ascii')
        if not base64.b64decode(encoded,validate=True).startswith(b'psbt\xff'):
            raise ValueError('Arquivo PSBT invalido')
    if utxos:
        import wallycore as w
        psbt=w.psbt_from_base64(encoded,0)
        for utxo in utxos:
            raw=Path(utxo).read_bytes()
            try: tx=w.tx_from_hex(b''.join(raw.split()).decode('ascii'),0)
            except (ValueError,UnicodeDecodeError): tx=w.tx_from_bytes(raw,0)
            for i in range(w.psbt_get_num_inputs(psbt)):
                if bytes(w.psbt_get_input_previous_txid(psbt,i))==bytes(w.tx_get_txid(tx)):
                    w.psbt_set_input_utxo(psbt,i,tx)
        encoded=w.psbt_to_base64(psbt,0)
    if len(encoded)>24000:
        raise ValueError('PSBT excede o limite de 24.000 caracteres')
    return encoded


def connect(port):
    s=serial.Serial(port=None,baudrate=115200,timeout=0.4,write_timeout=10)
    s.dtr=False;s.rts=False;s.port=port;s.open()
    return s


def serial_lines(connection, deadline):
    pending=bytearray()
    while time.monotonic()<deadline:
        data=connection.read(1024)
        if not data:
            continue
        pending.extend(data)
        if len(pending)>40000:
            raise ValueError('Resposta serial excede o limite')
        while b'\n' in pending:
            line,_,rest=pending.partition(b'\n')
            pending=bytearray(rest)
            yield line.decode('utf-8',errors='replace').strip()


def main():
    parser=argparse.ArgumentParser(description='OpenVault USB: enderecos publicos e PSBTs; nunca seeds ou PINs.')
    parser.add_argument('--port')
    sub=parser.add_subparsers(dest='command',required=True)
    sub.add_parser('status')
    final=sub.add_parser('finalize');final.add_argument('psbt',type=Path);final.add_argument('--output',required=True,type=Path)
    xpub=sub.add_parser('xpub');xpub.add_argument('--output',type=Path)
    sign=sub.add_parser('sign');sign.add_argument('psbt',type=Path);sign.add_argument('--output',required=True,type=Path);sign.add_argument('--utxo',action='append',default=[],help='Transacao anterior completa em hex ou binario; repetivel')
    args=parser.parse_args()
    if getattr(args,'output',None) and args.output.exists():
        raise SystemExit('Arquivo de saida ja existe. Escolha outro nome para preservar os dados.')
    if args.command=='finalize':
        import wallycore as w
        p=w.psbt_from_base64(load_psbt(args.psbt,[]),0)
        w.psbt_finalize(p,0)
        if not w.psbt_is_finalized(p): raise SystemExit('PSBT ainda nao possui todas as assinaturas')
        tx=w.psbt_extract(p,0)
        args.output.write_text(w.tx_to_hex(tx,w.WALLY_TX_FLAG_USE_WITNESS)+'\n')
        print('Transacao finalizada salva em',args.output,'; nao transmitida a rede.')
        return
    encoded=load_psbt(args.psbt,args.utxo) if args.command=='sign' else None
    if args.command=='sign': print('Na placa: desbloqueie e selecione Assinar PSBT antes de continuar.')
    if args.command=='xpub': print('Na placa: desbloqueie, selecione Exportar xpub e segure A para autorizar.')
    if args.command=='sign': input('Pressione Enter quando a placa estiver em Assinar PSBT: ')
    with connect(choose_port(args.port)) as s:
        if args.command=='status': s.write(b'STATUS\n')
        elif args.command=='sign': s.write(('SIGN '+encoded+'\n').encode())
        deadline=time.monotonic()+(15 if args.command=='status' else 600)
        for line in serial_lines(s,deadline):
            if not line: continue
            if line.startswith('ERROR ') or line.startswith('CANCELED') or line.startswith('LOCKED '):
                raise SystemExit(line)
            if line=='REVIEW_READY': print('Revise todas as saidas e a taxa na placa. Somente depois segure A para assinar.')
            if args.command=='status' and line.startswith('STATUS '): print(line);return
            if args.command=='xpub' and line.startswith('XPUB '):
                # Public data only. The fingerprint and origin enable watch-only descriptors.
                if args.output: args.output.write_text(line+'\n')
                else: print(line)
                return
            if args.command=='sign' and line.startswith('SIGNED '):
                result=line[7:]
                if not base64.b64decode(result,validate=True).startswith(b'psbt\xff'):
                    raise SystemExit('Resposta de assinatura invalida')
                args.output.write_text(result+'\n')
                print('PSBT assinada salva em',args.output,'(nenhuma transmissao a rede foi feita).')
                return
        raise SystemExit('Tempo esgotado. Confira a tela e a conexao USB.')
if __name__=='__main__': main()
