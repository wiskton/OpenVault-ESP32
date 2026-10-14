#include "openvault/wallet.hpp"
#include "openvault/menu.hpp"
#include "openvault/control.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
using namespace openvault;
int main(int argc,char**argv){
 const auto fresh=walletMenu(false,false),locked=walletMenu(true,false),unlocked=walletMenu(true,true);
 assert(fresh.size()==2&&fresh[0]==0&&fresh[1]==1);
 assert(locked.size()==1&&locked[0]==2);
 for(auto action:unlocked)assert(action!=0&&action!=1);
 for(const auto& name:{"Pessoal","Reserva BTC","Viagem-2026","Empresa_2"})assert(validWalletName(name));
 for(const auto& name:{""," Nome","Nome ","Nome\nLOCK","<script>","abcdefghijklmnopqrs"})assert(!validWalletName(name));
 assert(walletStorageKey("vault",0)=="vault");assert(walletStorageKey("vault",1)=="vault1");assert(walletStorageKey("attempts",4)=="attempts4");
 for(unsigned i=0;i<walletSlots;++i)assert(parseSlot("SELECT "+std::to_string(i))==int(i));
 for(const auto& bad:{"SELECT 5","SELECT -1","SELECT 0 extra","SELECT 00","SELECT 0\nLOCK"})assert(parseSlot(bad)==-1);
 assert(parseCreate("CREATE BTC 24").valid&&parseCreate("CREATE BTC 24").mainnet);
 assert(parseCreate("CREATE TEST 12").valid&&!parseCreate("CREATE TEST 12").mainnet);
 for(const auto& bad:{"CREATE BTC 0","CREATE TEST 13","CREATE BTC 12 extra","CREATE MAIN 12","CREATE BTC 12\nLOCK"})assert(!parseCreate(bad).valid);
 assert(allowDelete(2,2,true,true,true));
 assert(!allowDelete(1,2,true,true,true));assert(!allowDelete(5,5,true,true,true));
 assert(!allowDelete(2,2,false,true,true));assert(!allowDelete(2,2,true,false,true));assert(!allowDelete(2,2,true,true,false));
 assert(allowCreate(false,false,true));
 assert(!allowCreate(true,false,true));assert(!allowCreate(false,true,true));assert(!allowCreate(false,false,false));
 assert(wally_init(0)==WALLY_OK);
 assert(Wallet::selfTest());
 Wallet wallet;
 assert(!wallet.importWords("abandon abandon",false));
 uint8_t data[32]{};
 assert(wallet.setEntropy(data,16,true));
 assert(wallet.wordCount()==12);
 auto mainAddress=wallet.address();
 assert(mainAddress=="bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");
 std::array<uint8_t,32> random{};for(size_t i=0;i<32;++i)random[i]=uint8_t(i);
 std::vector<uint8_t> sealed;
 assert(wallet.seal("42687359",random,sealed));
 assert(sealed.size()==120);
 const auto saved=sealed;
 Wallet second;uint8_t otherEntropy[16];for(size_t i=0;i<16;++i)otherEntropy[i]=uint8_t(i+1);
 assert(second.setEntropy(otherEntropy,16,true));auto secondAddress=second.address();assert(secondAddress!=mainAddress);
 std::vector<uint8_t> secondBlob;assert(second.seal("58764291",random,secondBlob));second.lock();
 assert(!second.unlock(secondBlob,"42687359"));assert(second.unlock(secondBlob,"58764291"));assert(second.address()==secondAddress);
 assert(!second.unlock(saved,"58764291"));assert(second.unlock(saved,"42687359"));assert(second.address()==mainAddress);
 for(unsigned i=0;i<walletSlots;++i)for(unsigned j=i+1;j<walletSlots;++j)assert(walletStorageKey("vault",i)!=walletStorageKey("vault",j));
 wallet.lock();assert(!wallet.ready());
 assert(!wallet.unlock(sealed,"12345678"));assert(!wallet.ready());
 assert(wallet.unlock(sealed,"42687359"));assert(wallet.address()==mainAddress);
 for(auto index:{24,40,88,119}){sealed=saved;sealed[index]^=1;assert(!wallet.unlock(sealed,"42687359"));assert(!wallet.ready());}
 sealed=saved;sealed[4]^=1;assert(!wallet.unlock(sealed,"42687359"));
 assert(wallet.setEntropy(data,32,false));assert(wallet.wordCount()==24);
 assert(wallet.mnemonicWord(23)=="art");
 assert(wallet.importWords("abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",false));
 assert(wallet.address().find("tb1")==0);
 assert(!wallet.accountXpub().empty());
 std::vector<Payment> outputs;uint64_t fee;std::string error,signedPsbt;
 assert(!wallet.review("not a psbt",outputs,fee,error));
 assert(!wallet.signReviewed(signedPsbt));
 if(argc>1){std::ifstream in(argv[1]);std::string line;std::getline(in,line);assert(wallet.review(line,outputs,fee,error));assert(outputs.size()==1);assert(fee==1000);assert(wallet.signReviewed(signedPsbt));std::ofstream out("build/signed-test.psbt");out<<signedPsbt;assert(!wallet.signReviewed(signedPsbt));}
 for(int i=2;i<argc;++i){std::ifstream input(argv[i]);std::string bad;std::getline(input,bad);assert(!wallet.review(bad,outputs,fee,error));assert(!wallet.signReviewed(signedPsbt));}
 std::cout<<"OK: BIP39/BIP84, 12/24 palavras, PIN, cofre autenticado, adulteracao e politica PSBT.\n";
}
