import importlib.util,unittest,tempfile,subprocess,shutil
from pathlib import Path
spec=importlib.util.spec_from_file_location('guard',Path(__file__).resolve().parents[1]/'scripts/check_secrets.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
class GuardTests(unittest.TestCase):
 def test_hook_blocks_forced_private_file(self):
  with tempfile.TemporaryDirectory(prefix='openvault-guard-') as folder:
   root=Path(folder);(root/'scripts/data').mkdir(parents=True);(root/'.githooks').mkdir()
   for source in ['scripts/check_secrets.py','scripts/data/bip39_english.txt','.githooks/pre-commit']:
    shutil.copy2(g.ROOT/source,root/source)
   def git(*args):return subprocess.run(['git',*args],cwd=root,capture_output=True)
   self.assertEqual(git('init','-q').returncode,0)
   self.assertEqual(git('config','core.hooksPath','.githooks').returncode,0)
   (root/'.env').write_text('PUBLIC_TEST_FIXTURE=not-a-real-secret\n')
   self.assertEqual(git('add','-f','.env').returncode,0)
   result=git('-c','user.name=Guard test','-c','user.email=guard@example.invalid','commit','-qm','Synthetic safety test')
   self.assertNotEqual(result.returncode,0);self.assertIn(b'BLOCKED',result.stdout+result.stderr)
   self.assertNotEqual(git('rev-parse','--verify','HEAD').returncode,0)
 def test_private_paths(self):
  for path in ['backups/device.bin','.env.local','private/vault.json','openvault-carteira-1-consulta.json','export.psbt','credentials.pem']:
   self.assertTrue(g.findings(path,b'private fixture'))
 def test_keys_and_literal_pin(self):
  for data in [('xprv'+'A'*100).encode(),b'{"pin":"42687359"}',b'-----BEGIN '+b'PRIVATE KEY-----']:
   self.assertTrue(g.findings('accidental.txt',data))
 def test_public_vector_only_in_tests(self):
  self.assertFalse(g.findings('tests/vector.cpp',g.PUBLIC_PHRASE.encode()))
  self.assertTrue(g.findings('accidental.txt',g.PUBLIC_PHRASE.encode()))
  self.assertFalse(g.findings('README.md',b'Use the local web panel.'))
if __name__=='__main__':unittest.main()
