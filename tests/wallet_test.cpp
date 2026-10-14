#include "openvault/wallet.hpp"
#include "openvault/menu.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
using namespace openvault;
int main(int argc,char**argv){
 const auto fresh=walletMenu(false,false),locked=walletMenu(true,false),unlocked=walletMenu(true,true);
 assert(fresh.size()==2&&fresh[0]==0&&fresh[1]==1);
 assert(locked.size()==1&&locked[0]==2);
 for(auto action:unlocked)assert(action!=0&&action!=1);
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
