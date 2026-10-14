import importlib.util
from pathlib import Path
import time
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('openvault_companion',ROOT/'companion/openvault.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class FragmentedSerial:
    def __init__(self,chunks): self.chunks=iter(chunks)
    def read(self,size): return next(self.chunks,b'')
class CompanionTests(unittest.TestCase):
    def test_long_signed_response_is_not_truncated_by_serial_fragments(self):
        message=b'SIGNED '+b'A'*24000+b'\n'
        stream=FragmentedSerial([b'REVIEW_',b'READY\n'+message[:500],message[500:10000],message[10000:]])
        result=module.serial_lines(stream,time.monotonic()+1)
        self.assertEqual(next(result),'REVIEW_READY')
        self.assertEqual(next(result),message[:-1].decode())
    def test_sign_payload_file_validated_before_usb(self):
        path=ROOT/'build/unsigned-test.psbt'
        result=module.load_psbt(path,[])
        self.assertTrue(result.startswith('cHNidP'))
if __name__=='__main__': unittest.main()
